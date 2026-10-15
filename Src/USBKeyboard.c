/*
 * USBKeyboard.c -- the CMM2's USB keyboard, through the TinyUSB host stack.
 *
 * The keyboard layer is the PicoMite's input/USBKeyboard.c: the HID[] slot
 * table, FindFreeSlot(), the mount, unmount and report callbacks, and
 * hid_app_task(), which asks each slot for its next report every report_rate
 * milliseconds and auto-repeats the held key.  Keyboards only for now: the
 * PicoMite's mice, gamepads and touch panels can follow on the same slots.
 * Reports are decoded by KeyboardMap.c, also the PicoMite's.
 *
 * The STM32 side replaces the ST USB host library.  The H743's USB2_OTG_FS
 * core (PA11 DM, PA12 DP, on-chip full-speed PHY) is TinyUSB's root port 0;
 * its 48 MHz clock is HSI48 and the USB voltage detector is on, both from
 * SystemClock_Config().  VBUS is not switched by software.  Hubs are
 * enabled (tusb_config.h), so a keyboard with a built-in hub enumerates.
 *
 * The interrupt only queues events; CheckKeyboard(), called from
 * routinechecks() every OPTION USBPOLLING ms, runs tuh_task() and then
 * hid_app_task(), so every callback below runs in the main thread.
 */

#include <stdarg.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "tusb.h"
#include "KeyboardMap.h"

volatile struct s_HID HID[HID_SLOTS] = {0};
static uint8_t Current_USB_devices = 0;     /* slots in use */
static bool USBenabled = false;
extern const char *KBrdList[];

/* The callbacks run inside tuh_task(), so they do not print: printing can
   reach routinechecks() (a full console buffer, a screen scroll), and from
   there CheckSDCard() can longjmp out through TinyUSB.  They leave their
   message here and CheckKeyboard() prints it once tuh_task() has returned. */
static char usbmsg[160];
static void USBMessage(const char *m)
{
	if (CurrentLinePtr)
		return; /* not while a program runs */
	if (strlen(usbmsg) + strlen(m) < sizeof(usbmsg))
		strcat(usbmsg, m);
}

/* TinyUSB's millisecond clock (osal_none): the HAL tick, from SysTick. */
uint32_t tusb_time_millis_api(void)
{
	return HAL_GetTick();
}

/* TU_ASSERT would execute BKPT when the core's debug-enable bit is set (it
   can be, after an ST-Link session), halting the CMM2: never do that. */
void tusb_no_breakpoint(void)
{
}

/* TinyUSB's log (CFG_TUSB_DEBUG), for diagnosis: written into a RAM ring,
   from the interrupt as well, and sent to the serial console only (never
   the screen) by usblog_drain(), as far as the console's transmit buffer
   has room, so that logging never waits and never re-enters routinechecks(). */
#if CFG_TUSB_DEBUG
static char usblog[6144];
static volatile uint32_t usblog_head, usblog_tail;
extern volatile int ConsoleTxBufHead, ConsoleTxBufTail;
void SerialConsolePutC(int c);
int tusb_log_printf(const char *fmt, ...)
{
	char b[200];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(b, sizeof(b), fmt, ap);
	va_end(ap);
	if (n > (int)sizeof(b) - 1)
		n = sizeof(b) - 1;
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	for (int i = 0; i < n; i++)
	{
		uint32_t next = (usblog_head + 1) % sizeof(usblog);
		if (next == usblog_tail)
			break; /* full: drop the rest */
		usblog[usblog_head] = b[i];
		usblog_head = next;
	}
	__set_PRIMASK(primask);
	return n;
}
static void usblog_drain(void)
{
	while (usblog_tail != usblog_head)
	{
		int room = (ConsoleTxBufTail - ConsoleTxBufHead - 1 + CONSOLE_TX_BUF_SIZE) % CONSOLE_TX_BUF_SIZE;
		if (room < 2)
			return;
		char c = usblog[usblog_tail];
		usblog_tail = (usblog_tail + 1) % sizeof(usblog);
		SerialConsolePutC(c);
	}
}
#else
int tusb_log_printf(const char *fmt, ...)
{
	(void)fmt;
	return 0;
}
#define usblog_drain()
#endif

void OTG_FS_IRQHandler(void)
{
	tuh_int_handler(BOARD_TUH_RHPORT);
}

void clearrepeat(void)
{
	keytimer = 0;
	repeattime = Option.RepeatStart;
	memset(KeyDown, 0, sizeof(KeyDown));
}

/* Bring the host up.  The core is reset first, so a soft restart starts from
   a clean controller; TinyUSB then powers the port and resets whatever is
   attached (a hub included) before it enumerates. */
void USBKeyboardInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	clearrepeat();
	for (int i = 0; i < HID_SLOTS; i++)
	{
		memset((void *)&HID[i], 0, sizeof(struct s_HID));
		HID[i].report_requested = true;
	}
	Current_USB_devices = 0;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;            /* DM, DP */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	__HAL_RCC_USB_OTG_FS_CLK_ENABLE();
	__HAL_RCC_USB_OTG_FS_FORCE_RESET();
	__HAL_RCC_USB_OTG_FS_RELEASE_RESET();
	HAL_NVIC_SetPriority(OTG_FS_IRQn, 0, 0);                    /* TinyUSB enables it */

	const tusb_rhport_init_t host_init = {.role = TUSB_ROLE_HOST, .speed = TUSB_SPEED_FULL};
	tusb_init(BOARD_TUH_RHPORT, &host_init);
	USBenabled = true;
}

/* Every millisecond, from Timer1msHandler(): time since each slot's last report. */
void USBKeyboardTick(void)
{
	for (int i = 0; i < HID_SLOTS; i++)
	{
		if (HID[i].Device_type && HID[i].report_timer < 10000)
			HID[i].report_timer++;
	}
}

static void hid_app_task(void)
{
	if (KeyDown[0] && keytimer > (int)repeattime)
	{
		uint8_t c = KeyDown[0];
		if (!(c == 0 || c == PDOWN || c == PUP || c == CTRLKEY('P') || c == CTRLKEY('L') || c == 25 || c == F5 || c == F4 || c == CTRLKEY('T')))
		{
			USR_KEYBRD_ProcessData(c);
		}
		keytimer = 0;
		repeattime = Option.RepeatRate;
	}
	for (int i = 0; i < HID_SLOTS; i++)
	{
		if (HID[i].active == false)
			continue;
		if (HID[i].report_requested)
			continue;
		if (HID[i].report_timer >= HID[i].report_rate)
		{
			if (HID[i].Device_type == HID_ITF_PROTOCOL_KEYBOARD && HID[i].notfirsttime == 0)
			{
				HID[i].notfirsttime = 1;
				if (!Option.noLED)
					tuh_hid_set_report(HID[i].Device_address, HID[i].Device_instance, 0, HID_REPORT_TYPE_OUTPUT, (void *)&HID[i].sendlights, 1);
			}
			HID[i].report_requested = true;
			if (!tuh_hid_receive_report(HID[i].Device_address, HID[i].Device_instance))
			{
				// Allow retry on next poll cycle instead of permanent deactivation
				HID[i].report_requested = false;
				HID[i].report_timer = 0;
			}
		}
	}
}

/* tuh_task() must not run inside itself; the guard is a second line of
   defence behind the callbacks not printing (USBMessage). */
void CheckKeyboard(void)
{
	static volatile bool busy = false;
	if (!USBenabled || busy)
		return;
	busy = true;
	tuh_task();
	hid_app_task();
	usblog_drain();
	busy = false;
	if (usbmsg[0])
	{
		char m[sizeof(usbmsg)];
		strcpy(m, usbmsg);
		usbmsg[0] = 0;
		MMPrintString(m);
	}
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

/* The dwc2 driver refused a low-speed device behind a hub: on this core's
   full-speed port such a device hangs the controller (STM32 erratum, see
   CFG_TUH_DWC2_REFUSE_LS_BEHIND_FS_HUB in tusb_config.h). */
void tuh_dwc2_ls_hub_refused_cb(uint8_t hub_addr, uint8_t hub_port)
{
	(void)hub_addr;
	(void)hub_port;
	USBMessage("USB low-speed device behind a hub is not supported: plug it in directly\r\n> ");
}

void tuh_mount_cb(uint8_t dev_addr)
{
	(void)dev_addr;
}

void tuh_umount_cb(uint8_t dev_addr)
{
	(void)dev_addr;
}

static int FindFreeSlot(uint8_t itf_protocol)
{
	if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD && !HID[0].active)
		return 0;
	for (int i = HID_SLOTS - 1; i >= 0; i--)
	{
		if (!HID[i].active)
			return i;
	}
	return -1;
}

// Invoked when device with hid interface is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
{
	(void)desc_report;
	(void)desc_len;
	__DSB();
	uint16_t pid, vid;
	uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
	if (itf_protocol != HID_ITF_PROTOCOL_KEYBOARD)
		return; /* keyboards only, for now: leave other interfaces unclaimed */
	int slot = FindFreeSlot(itf_protocol);
	if (slot == -1 || Current_USB_devices == HID_SLOTS)
		return; /* all slots in use: skip quietly rather than error mid-enumeration */
	tuh_vid_pid_get(dev_addr, &vid, &pid);
	HID[slot].vid = vid;
	HID[slot].pid = pid;
	if (Option.USBKeyboard == CONFIG_US)
		keylayout = USkeyValue;
	else if (Option.USBKeyboard == CONFIG_DE)
		keylayout = DEkeyValue;
	else if (Option.USBKeyboard == CONFIG_FR)
		keylayout = FRkeyValue;
	else if (Option.USBKeyboard == CONFIG_ES)
		keylayout = ESkeyValue;
	else if (Option.USBKeyboard == CONFIG_BE)
		keylayout = BEkeyValue;
	else
		keylayout = UKkeyValue;
	HID[slot].Device_address = dev_addr;
	HID[slot].Device_instance = instance;
	HID[slot].Device_type = HID_ITF_PROTOCOL_KEYBOARD;
	caps_lock = 0; /* as the CMM2 always started: no OPTION CAPSLOCK/NUMLOCK yet */
	num_lock = 0;
	scroll_lock = 0;
	HID[slot].sendlights = kbd_lock_leds();
	HID[slot].report_rate = 20; // mSec between report
	HID[slot].report_timer = -(10 + (slot + 2) * 500);
	HID[slot].active = true;
	HID[slot].report_requested = false;
	{
		char m[64];
		strcpy(m, KBrdList[(int)Option.USBKeyboard]);
		strcat(m, " USB Keyboard Connected on channel ");
		IntToStr(m + strlen(m), slot + 1, 10);
		strcat(m, "\r\n> ");
		USBMessage(m);
	}
	keyboardseen = 1;
	SD_LED_GPIO_Port->BSRR = SD_LED_Pin; /* the CMM2 lights its LED while a keyboard is there */
	Current_USB_devices++;
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
	__DSB();
	int i;
	for (i = 0; i < HID_SLOTS; i++)
	{
		if (HID[i].active && instance == HID[i].Device_instance && dev_addr == HID[i].Device_address && HID[i].Device_type == HID_ITF_PROTOCOL_KEYBOARD)
		{
			USBMessage("USB Keyboard Disconnected\r\n> ");
			break;
		}
	}
	if (i < HID_SLOTS)
	{
		memset((void *)&HID[i], 0, sizeof(struct s_HID));
		HID[i].report_requested = true;
		Current_USB_devices--;
		clearrepeat(); /* a key held as it went must not go on repeating */
		int keyboards = 0;
		for (int j = 0; j < HID_SLOTS; j++)
			if (HID[j].active && HID[j].Device_type == HID_ITF_PROTOCOL_KEYBOARD)
				keyboards++;
		if (!keyboards)
		{
			keyboardseen = 0;
			SD_LED_GPIO_Port->BSRR = SD_LED_Pin << 16;
		}
	}
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
	__DSB();
	int n = -1;
	for (int i = 0; i < HID_SLOTS; i++)
		if (HID[i].active && instance == HID[i].Device_instance && dev_addr == HID[i].Device_address)
		{
			n = i;
			break;
		}
	if (n < 0)
		return; // no matching device found, discard report
	memcpy((void *)&HID[n].report[1], report, (len > 64 ? 64 : len));
	HID[n].report[0] = (len > 64 ? 64 : len);
	if (HID[n].Device_type == HID_ITF_PROTOCOL_KEYBOARD && len >= sizeof(hid_keyboard_report_t))
		process_kbd_report((hid_keyboard_report_t const *)report, n);
	HID[n].report_requested = false;
	HID[n].report_timer = 0;
}
