/*
 * tusb_config.h - TinyUSB configuration for the Colour Maximite 2 (STM32H743),
 * built against TinyUSB master efab607b8 (Middlewares/Third_Party/tinyusb)
 *
 * USB host only, on the H743's full-speed OTG core with its on-chip PHY
 * (USB2_OTG_FS, PA11/PA12), which TinyUSB numbers root port 0.  The FS core
 * has no internal DMA, so the driver runs in slave mode and the CPU moves
 * every packet through the FIFOs: no buffer needs to be in DMA-reachable or
 * uncached memory.
 *
 * Hubs are enabled so a keyboard with a built-in hub (the Raspberry Pi
 * keyboard: hub, then the keyboard behind it) enumerates.  The sizes follow
 * the PicoMite's usb_host_files/tusb_config.h.
 */
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU                OPT_MCU_STM32H7
#define CFG_TUSB_OS                 OPT_OS_NONE
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG              0                   // 2: TinyUSB's log, to the serial console (USBKeyboard.c)
#endif
#define CFG_TUSB_DEBUG_PRINTF       tusb_log_printf     // (only called when CFG_TUSB_DEBUG is set)
#define CFG_TUSB_DEBUG_BREAKPOINT   tusb_no_breakpoint  // a failed TU_ASSERT must never halt the CPU

#define BOARD_TUH_RHPORT            0                   // USB2_OTG_FS, PA11/PA12
#define CFG_TUH_ENABLED             1
#define CFG_TUH_MAX_SPEED           OPT_MODE_FULL_SPEED
#define CFG_TUH_DWC2_DMA_ENABLE     0                   // the FS core has no DMA: slave mode
#define CFG_TUH_DWC2_REFUSE_LS_BEHIND_FS_HUB 1          // STM32 erratum: a low-speed device behind a hub hangs the core (hcd_dwc2.c)

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN          __attribute__((aligned(4)))

#define CFG_TUH_ENUMERATION_BUFSIZE 1024
#define CFG_TUH_HUB                 2                   // a hub, and a keyboard hub behind it
#define CFG_TUH_DEVICE_MAX          (3 * CFG_TUH_HUB + 1)
#define CFG_TUH_HID                 8                   // the PicoMite has 4 * CFG_TUH_DEVICE_MAX; DTCM is tight here
#define CFG_TUH_HID_EPIN_BUFSIZE    64
#define CFG_TUH_HID_EPOUT_BUFSIZE   64
#define CFG_TUH_TASK_QUEUE_SZ       64

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
