/* WebMite_cmm2.c: the Colour Maximite 2's side of the PicoMite's network code (Src/net/picomite, docs/Web_Plan.md,
   step 2).
     - The globals the PicoMite defines in its PicoMite.c, misc/Custom.c, core/MM_Misc.c and misc/FileIO.c, with their
       values there.
     - Its file and time helpers on the CMM2's FatFS and clock: ExistsFile, FileSize, SDCardConfigured,
       set_time_epoch_us (WEB NTP's time: the PicoMite's uptime offset, and the CMM2's date and time and its RTC too).
     - The network block (NetBlock.h): on a G2 or G3, the 128 KB of MMBasic's SDRAM heap below the executable window,
       kept out of the heap at every wipe (Memory.c HeapWipe: WebBlockMark). A G1's comes with step 7 of the plan (the
       top of its display memory); until then Wi-Fi does not start on a G1.
     - Wi-Fi's start at power-up with OPTION WIFI (the PicoMite's main: cyw43_arch_init_with_country, then WebConnect),
       after a check that the module is there.
     - WebPoll: the network pumped from routinechecks' full checks, on its 1 ms tick (main.c). Never from the flash
       writer's loops, which call routinechecks(0): the network code is in bank 2, which stalls while a sector of it is
       erased or written. */
#include "WebMite_cmm2.h"
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include <time.h>
#include "NetBlock.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "picomite/WiFi.h"
#include "cyw43_fw_cmm2.h"                     // (cyw43_fw_release)
#define BT_HEAP_NO_MACROS
#include "bt_heap.h"                            // (Bluetooth's block: its size and BtHeapInit)

/* ---- the PicoMite's globals ---- */
volatile int WIFIconnected = 0;                 // (PicoMite.c)
volatile int LastWifiErr = 0;
volatile int scantimer = 0;
int startupcomplete = 0;
int WebScanActive = 0;
volatile bool TCPreceived = false;              // (misc/Custom.c)
char *TCPreceiveInterrupt = NULL;
static int NetHold;                                  // a G1's display memory lent (WebLend, WebGiveBack)
char *MQTTInterrupt = NULL;                     // (core/MM_Misc.c)
volatile bool MQTTComplete = false;
char *UDPinterrupt = NULL;
volatile bool UDPreceive = false;
int64_t TimeOffsetToUptime = 1704067200;        // (1 Jan 2024, the PicoMite's: WebStart sets it from the CMM2's clock)
uint32_t TimeSubSecondUs = 0;
int FatFSFileSystem = 1;                        // (misc/FileIO.c: 0 is the PicoMite's flash drive; the CMM2's are FatFS)
char id_out[12];                                // (PicoMite.c: the board's ID, for the default host name)
int clocktimer = 0;

_Static_assert(STRINGSIZE == 256, "WebMite_cmm2.h declares MQTT's buffers with the PicoMite's STRINGSIZE");

/* ---- files ---- */
// a name as BasicFileOpen (FileIO.c) makes it: the directory through fullpath, the current drive's root for "/x"
static void net_fullname(char *fname, char *out) {
    char pp[FF_MAX_LFN] = {0};
    char q[FF_MAX_LFN] = {0};
    int i, j;
    i = strlen(fname) - 1;
    while (i > 0 && !(fname[i] == 92 || fname[i] == 47)) i--;
    if (i > 0) {
        memcpy(q, fname, i);
        for (j = 0; j < strlen(q); j++) if (q[j] == '\\') q[j] = '/';
        i++;
    }
    strcpy(pp, &fname[i]);
    if ((pp[0] == 47 || pp[0] == 92) && i == 0) {
        strcpy(q, &pp[1]);
        strcpy(pp, q);
        q[0] = filepath[0]; q[1] = ':'; q[2] = '/'; q[3] = 0;
    }
    fullpath(q);
    strcpy(out, fullpathname);
    if (out[strlen(out) - 1] != '/') strcat(out, "/");
    strcat(out, pp);
}

static int net_stat(char *p, FILINFO *f) {
    char q[FF_MAX_LFN];
    if (!InitSDCard()) return -1;
    net_fullname(p, q);
    memset(f, 0, sizeof(FILINFO));
    FSerror = f_stat(q, f);
    return FSerror == FR_OK && !(f->fattrib & AM_DIR);
}

int ExistsFile(char *p) {
    FILINFO f;
    return net_stat(p, &f);
}

int FileSize(char *p) {
    FILINFO f;
    int r = net_stat(p, &f);
    return r < 0 ? -1 : r ? (int)f.fsize : 0;
}

int SDCardConfigured(void) {
    return 1;                                   // (the CMM2 always has its card slot)
}

/* ---- time ---- */
// the PicoMite's (core/MM_Misc.c), then the CMM2's date and time from it, and its RTC
void set_time_epoch_us(int64_t epoch_us) {
    const uint64_t now = time_us_64();
    const uint32_t sub = (uint32_t)(epoch_us % 1000000);
    TimeSubSecondUs = (uint32_t)((now % 1000000 + 1000000 - sub) % 1000000); // so that (now - phase) % 1000000 == sub
    TimeOffsetToUptime = epoch_us / 1000000 - (int64_t)((now - TimeSubSecondUs) / 1000000);
    time_t secs = (time_t)(epoch_us / 1000000);
    struct tm *t = gmtime(&secs);
    mT4IntEnable(0);
    second = t->tm_sec; minute = t->tm_min; hour = t->tm_hour;
    day = t->tm_mday; month = t->tm_mon + 1; year = t->tm_year + 1900;
    day_of_week = t->tm_wday ? t->tm_wday : 7;
    milliseconds = (int)(epoch_us / 1000 % 1000);
    mT4IntEnable(1);
    update_clock();
}

// the PicoMite's uptime offset from the CMM2's date and time (its RTC), so that mbedTLS's time is the real one
static void time_from_clock(void) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    mT4IntEnable(0);
    t.tm_sec = second; t.tm_min = minute; t.tm_hour = hour;
    t.tm_mday = day; t.tm_mon = month - 1; t.tm_year = year - 1900;
    mT4IntEnable(1);
    int64_t epoch = (int64_t)mktime(&t);        // (newlib's time zone is UTC unless set: the clock is taken as UTC)
    TimeSubSecondUs = 0;
    TimeOffsetToUptime = epoch - (int64_t)(time_us_64() / 1000000);
}

/* ---- the network block ---- */
// at start-up, before the first InitHeap: the block's place when OPTION WIFI is set (a G2 or G3)
void WebBlockSet(void) {
    uint32_t w = HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2();
    sprintf(id_out, "%08lX", (unsigned long)w);         // (OPTION WIFI's default host name ends with it)
    NetBlock = NULL;
    if (*Option.SSID && RadioWifiOn())                  // (a G1: the display memory's top 128 KB, Memory.h)
        NetBlock = (uint8_t *)(G1Hardware ? G1_NET_BLOCK : SDRAMEXEC - NET_BLOCK_SIZE);
    BtBlock = NULL;                                     // and Bluetooth's heap, below the network block's place
    if (RadioBtOn())
        BtBlock = (uint8_t *)(SDRAMEXEC - (G1Hardware ? 0 : NET_BLOCK_SIZE) - BT_BLOCK_SIZE);
}

// Bluetooth's block (Inc/net/bt_heap.h): BT_BLOCK_SIZE of MMBasic's SDRAM heap at a fixed place, kept out of it as the
// network block is, whose memory BTstack and the PicoMite's Bluetooth host take (Src/net/bt/bt_heap.c)
uint8_t *BtBlock;
int BtBlockInHeap(void **addr, int *size) {
    if (BtBlock == NULL) return 0;
    *addr = BtBlock;
    *size = BT_BLOCK_SIZE;
    return 1;
}

// OPTION RADIO: Bluetooth's half on (cyw43_arch_cmm2.c, which sets BTstack up with the chip as pico-sdk's does)
int RadioBt(void) {
    return RadioBtOn();
}

// the block's pages kept out of MMBasic's SDRAM heap (HeapWipe): its contents are lwIP's and outlive RUN and NEW
int WebBlockInHeap(void **addr, int *size) {
    if (NetBlock == NULL || G1Hardware) return 0;
    *addr = NetBlock;
    *size = NET_BLOCK_SIZE;
    return 1;
}

/* ---- start-up ---- */
static void reserve_pins(void);

// with no module answering: the network block given back to the heap, SPI2 off, and pins 32, 33, 38 and 40 as they are
// without OPTION WIFI (not configured), so that the header's passed-through pins work as before. (A G3 V1.2's module
// is on the board, on SPI6: the connector's pins and SPI2 were never Wi-Fi's there.)
static void release_all(void) {
    void *b = NetBlock;
    NetBlock = NULL;
    if (b && !G1Hardware) FreeMemory(b);                // (a G1's is display memory, not the heap's)
    b = BtBlock;                                        // (Bluetooth's is the heap's on every generation)
    BtBlock = NULL;
    if (b) FreeMemory(b);
    if (RM2OnBoard) {
        SPI6->CR1 = 0;
        GPIOF->BSRR = 1u << (8 + 16);                   // the module off (WL_ON, PF8), as pin 32 freed turns the
        return;                                         // add-on's off
    }
    SPI2->CR1 = 0;
    ExtCfg(32, EXT_NOT_CONFIG, 0);
    ExtCfg(33, EXT_NOT_CONFIG, 0);
    ExtCfg(38, EXT_NOT_CONFIG, 0);
    ExtCfg(40, EXT_NOT_CONFIG, 0);
}
// a G3 V1.2 built without its RM2 module (the straps, PF6 and PF9 low, say it is on the board, and nothing answered):
// OPTION RADIO NONE saved, as a board with no module has by default, so that later starts do not look for it again and
// the user is left at the prompt; OPTION WIFI or OPTION RADIO turn it on again. The sound comes off Bluetooth with it.
// (An add-on module's OPTION RADIO is kept: it may only be unplugged for now.) A save that finds no RAM for the
// sector's copy leaves the option set until the next start, which then tries again.
static void radio_none(void) {
    extern int OptionSaveSoft;
    Option.Radio = RADIO_WIFI_OFF | RADIO_BT_OFF;
    Option.audio_bluetooth = 0;
    OptionSaveSoft = 1;
    SaveOptions(1);
    MMPrintString("No module on this board: OPTION RADIO NONE\r\n");
}
// OPTION RADIO's halves (the PicoMite PC3's): the WiFi with OPTION WIFI set and its half on, Bluetooth with its own
// (docs/Bluetooth_Plan.md). Both on the one chip: one cyw43_arch_init (which also sets BTstack up with Bluetooth's half
// on: cyw43_arch_cmm2.c), then the WiFi's join and the Bluetooth host's start, as the PC3's main has them.
void WebStart(void) {
    int wifi = *Option.SSID && RadioWifiOn(), bt = RadioBtOn() && BtBlock != NULL, up;
    if (!wifi && !bt) return;
    if (wifi && NetBlock == NULL) return;
    if (bt) BtHeapInit(BtBlock, BT_BLOCK_SIZE);         // (its heap, before cyw43_arch_init sets BTstack up)
    time_from_clock();
    if (cyw43_arch_init_with_country(wifi_country_to_cyw43(Option.wifi_country_code)) != 0) return;
    if (wifi) {
        cyw43_arch_enable_sta_mode();           // (the chip's firmware loaded: a missing module fails here)
        up = (cyw43_state.itf_state & (1 << CYW43_ITF_STA)) != 0;   // (no answer to the gSPI test read: no module)
    } else {
        up = 1;
    }
    if (up && bt)                               // Bluetooth's patch, now (the chip's firmware too if the WiFi is off):
        up = cyw43_bluetooth_hci_init() == 0;   // what BTstack's HCI transport asks for when Bluetooth starts
    cyw43_fw_release();                         // (the firmware's inflated copy: Src/net/cyw43_fw_cmm2.c)
    if (!up) {
        MMPrintString("WiFi module not found\r\n");
        cyw43_arch_deinit();
        release_all();                          // the board as without OPTION WIFI: the block back in the heap,
        if (RM2OnBoard) radio_none();           // SPI2 off and the four pins free again; a G3 V1.2 without its module:
        return;                                 // OPTION RADIO NONE
    }
    reserve_pins();
    if (wifi) {
        startupcomplete = 1;
        WebConnect();
    }
    if (bt) {
        extern void bt_keyboard_init(void);     // (Src/net/bt/picomite/BTKeyboard.c: the PicoMite's Bluetooth host)
        extern int BtStarted;
        bt_keyboard_init();
        BtStarted = 1;
    }
}

/* ---- the Bluetooth poll ---- */
// from routinechecks' full checks and CheckAbort (main.c), as the PicoMite PC3's ProcessBT(): the radio, BTstack's work,
// the notices its callbacks posted, the pairings' save. Not again from inside itself, nor while a G1's display memory
// is lent (WebLend), nor (main.c) while a loadable program runs.
int BtStarted;
void BtPoll(void) {
    static int busy = 0;
    extern void bt_keyboard_poll(void);
    if (busy || NetHold || !BtStarted) return;
    busy = 1;
    bt_keyboard_poll();
    busy = 0;
}

/* ---- the poll ---- */
// from routinechecks' full checks (main.c), on its 1 ms tick: the PicoMite's CheckAbort's ProcessWeb(1). Not again
// from inside itself (an lwIP callback that prints, say).
void WebPoll(void) {
    static int busy = 0;
    if (busy || NetHold || !(WIFIconnected || WebScanActive)) return;
    busy = 1;
    ProcessWeb(1);
    busy = 0;
}

/* ---- OPTION, OPTION LIST and MM.INFO: the PicoMite's (core/MM_Misc.c, PICOMITEWEB), with the CMM2's PO, getargs,
   SaveOptions(1) and SoftReset(). Called by MiscSTM32.c's cmd_option and printoptions, and MM_Misc.c's fun_info. ---- */
extern void PO(char *s, int m);
extern void PO2Str(char *s1, const char *s2, int m);
extern void PO2Int(char *s1, int64_t n);
extern void PO3Int(char *s1, int64_t n1, int64_t n2);

// OPTION WIFI, RADIO, TCP SERVER PORT, UDP SERVER PORT, TELNET CONSOLE, TFTP, WEB MESSAGES: 1 if it was one of them
int WebOption(char *cmdline) {
    char *tp;
    /* OPTION RADIO BOTH | WEB | BLUETOOTH | NONE: the PicoMite PC3's, which halves of the radio the next start brings
       up. With WEB or NONE the Bluetooth host is not started; with BLUETOOTH or NONE the WiFi is left down (no join,
       no servers) and the WEB commands refuse. The stored WiFi network and the Bluetooth pairings are kept. Saved,
       then a restart. On the CMM2, whose module is optional, NONE is the default, and OPTION WIFI turns the WiFi's
       half on (docs/Bluetooth_Plan.md). */
    tp = checkstring(cmdline, "RADIO");
    if (tp) {
        uint8_t radio = 0;
        if (CurrentLinePtr) error("Invalid in a program");
        if (checkstring(tp, "BOTH")) radio = 0;
        else if (checkstring(tp, "WEB")) radio = RADIO_BT_OFF;
        else if (checkstring(tp, "BLUETOOTH")) radio = RADIO_WIFI_OFF;
        else if (checkstring(tp, "NONE")) radio = RADIO_WIFI_OFF | RADIO_BT_OFF;
        else error("Syntax: OPTION RADIO BOTH @ WEB @ BLUETOOTH @ NONE", '|', '|', '|');
        if ((radio & RADIO_BT_OFF) && Option.audio_bluetooth) error("Audio is on Bluetooth (OPTION AUDIO)");
        Option.Radio = radio;
        SaveOptions(1);
        SoftReset();
        return 1;
    }
    tp = checkstring(cmdline, "WEB MESSAGES");
    if (tp) {
        if (checkstring(tp, "OFF")) {
            optionsuppressstatus = 1;
            return 1;
        }
        if (checkstring(tp, "ON")) {
            optionsuppressstatus = 0;
            return 1;
        }
    }
    /* OPTION AUDIO BLUETOOTH [idle]: the sound to a Bluetooth speaker as well as the DACs (Src/net/bt/BTMite_cmm2.c,
       the PicoMite's BTAudio.c). idle is the seconds of silence before the stream is suspended, 0 for never (default
       60), kept as idle + 1 so that 0 means no Bluetooth. OPTION AUDIO DAC: the DACs only again. */
    tp = checkstring(cmdline, "AUDIO");
    if (tp) {
        char *p;
        if ((p = checkstring(tp, "BLUETOOTH"))) {
            int idle = 60;
            if (*p && *p != '\'') idle = getint((unsigned char *)p, 0, 250);
            if (!RadioBtOn()) error("Bluetooth is off (OPTION RADIO)");
            if (Option.audio_bluetooth) {                       // already the output: only the timeout changes
                Option.audio_bluetooth = idle + 1;
                SaveOptions(1);
                return 1;
            }
            if (CurrentLinePtr) error("Invalid in a program");
            Option.audio_bluetooth = idle + 1;
            SaveOptions(1);
            SoftReset();
            return 1;
        }
        if (checkstring(tp, "DAC")) {
            if (!Option.audio_bluetooth) return 1;
            if (CurrentLinePtr) error("Invalid in a program");
            Option.audio_bluetooth = 0;
            SaveOptions(1);
            SoftReset();
            return 1;
        }
        error("Syntax: OPTION AUDIO BLUETOOTH [idle] @ DAC", '|');
    }
    tp = checkstring(cmdline, "WIFI");
    if (tp) {
        setwifi((unsigned char *)tp);
        if (*Option.SSID && !RadioWifiOn()) {           // a network given: the WiFi's half of OPTION RADIO on with it
            Option.Radio &= ~RADIO_WIFI_OFF;
            SaveOptions(1);
        }
        SoftReset();
        return 1;
    }
    tp = checkstring(cmdline, "TCP SERVER PORT");
    if (tp) {
        getargs(&tp, 3, ",");
        if (CurrentLinePtr) error("Invalid in a program");
        Option.TCP_PORT = getint(argv[0], 0, 65535);
        Option.ServerResponceTime = 5000;
        if (argc == 3) Option.ServerResponceTime = getint(argv[2], 1000, 20000);
        SaveOptions(1);
        SoftReset();
        return 1;
    }
    tp = checkstring(cmdline, "UDP SERVER PORT");
    if (tp) {
        getargs(&tp, 3, ",");
        if (CurrentLinePtr) error("Invalid in a program");
        Option.UDP_PORT = getint(argv[0], 0, 65535);
        Option.UDPServerResponceTime = 5000;
        if (argc == 3) Option.UDPServerResponceTime = getint(argv[2], 1000, 20000);
        SaveOptions(1);
        SoftReset();
        return 1;
    }
    tp = checkstring(cmdline, "TELNET CONSOLE");
    if (tp) {
        if (CurrentLinePtr) error("Invalid in a program");
        if (checkstring(tp, "OFF")) Option.Telnet = 0;
        else if (checkstring(tp, "ON")) Option.Telnet = 1;
        else if (checkstring(tp, "ONLY")) Option.Telnet = -1;
        else error("Syntax");
        SaveOptions(1);
        SoftReset();
        return 1;
    }
    tp = checkstring(cmdline, "TFTP");
    if (tp) {
        if (CurrentLinePtr) error("Invalid in a program");
        if (checkstring(tp, "OFF") || checkstring(tp, "DISABLE")) Option.disabletftp = 1;
        else if (checkstring(tp, "ON") || checkstring(tp, "ENABLE")) Option.disabletftp = 0;
        else error("Syntax");
        SaveOptions(1);
        SoftReset();
        return 1;
    }
    return 0;
}

void WebOptionList(void) {
    if (*Option.SSID) {
        char password[] = "****************************************************************";
        password[strlen((char *)Option.PASSWORD)] = 0;
        PO("WIFI", 1);
        MMPrintString((char *)Option.SSID);
        MMPrintString(", ");
        MMPrintString(password);
        MMPrintString(", ");
        MMPrintString((char *)Option.hostname);
        if (*Option.ipaddress) {
            MMPrintString(", ");
            MMPrintString((char *)Option.ipaddress);
            MMPrintString(", ");
            MMPrintString((char *)Option.mask);
            MMPrintString(", ");
            MMPrintString((char *)Option.gateway);
        }
        if (Option.wifi_country_code) {
            MMPrintString(", ");
            MMPrintString((char *)wifi_country_to_string(Option.wifi_country_code));
        }
        MMPrintString("\r\n");
    }
    // OPTION RADIO, unless it is what OPTION WIFI alone gives (the WiFi's half on with a network, Bluetooth's off)
    if (Option.Radio != (RADIO_BT_OFF | (*Option.SSID ? 0 : RADIO_WIFI_OFF)))
        PO2Str("RADIO", Option.Radio == 0 ? "BOTH" : Option.Radio == RADIO_BT_OFF ? "WEB" : Option.Radio == RADIO_WIFI_OFF ? "BLUETOOTH" : "NONE", 1);
    if (Option.audio_bluetooth) PO2Int("AUDIO BLUETOOTH", Option.audio_bluetooth - 1);
    if (Option.TCP_PORT && Option.ServerResponceTime != 5000) PO3Int("TCP SERVER PORT", Option.TCP_PORT, Option.ServerResponceTime);
    if (Option.TCP_PORT && Option.ServerResponceTime == 5000) PO2Int("TCP SERVER PORT", Option.TCP_PORT);
    if (Option.UDP_PORT && Option.UDPServerResponceTime != 5000) PO3Int("UDP SERVER PORT", Option.UDP_PORT, Option.UDPServerResponceTime);
    if (Option.UDP_PORT && Option.UDPServerResponceTime == 5000) PO2Int("UDP SERVER PORT", Option.UDP_PORT);
    if (Option.Telnet == 1) PO2Str("TELNET", "CONSOLE ON", 1);
    if (Option.Telnet == -1) PO2Str("TELNET", "CONSOLE ONLY", 1);
    if (Option.disabletftp == 1) PO2Str("TFTP", "OFF", 1);
}

// MM.INFO(TCP REQUEST n), (TCP PORT), (UDP PORT), (IP ADDRESS), (MAX CONNECTIONS), (WIFI STATUS), (TCPIP STATUS), (TELNET) and
// MM.INFO(OPTION SSID): 1 if it was one of them (iret or sret and targ set)
int WebInfo(char *ep) {
    char *tp;
    if ((tp = checkstring(ep, "BLUETOOTH"))) {          // MM.INFO(BLUETOOTH SPEAKER | KEYBOARD | SERIAL): the PicoMite's
        extern void bt_info(unsigned char *tp, char *out);   // (Src/net/bt/picomite/BTAudio.c)
        bt_info((unsigned char *)tp, (char *)sret);
        CtoM(sret);
        targ = T_STR;
        return 1;
    }
    if ((tp = checkstring(ep, "TCP REQUEST"))) {
        int i = getint(tp, 1, MaxPcb) - 1;
        iret = TCPstate ? TCPstate->inttrig[i] : 0;
        targ = T_INT;
        return 1;
    }
    if (checkstring(ep, "TCP PORT")) {
        iret = Option.TCP_PORT;
        targ = T_INT;
        return 1;
    }
    if (checkstring(ep, "UDP PORT")) {
        iret = Option.UDP_PORT;
        targ = T_INT;
        return 1;
    }
    if (checkstring(ep, "IP ADDRESS")) {
        /* no interface at all if the radio didn't start */
        strcpy((char *)sret, netif_list ? ip4addr_ntoa(netif_ip4_addr(netif_list)) : "0.0.0.0");
        CtoM(sret);
        targ = T_STR;
        return 1;
    }
    if (checkstring(ep, "MAX CONNECTIONS")) {
        iret = MaxPcb;
        targ = T_INT;
        return 1;
    }
    if (checkstring(ep, "WIFI STATUS")) {
        int live = startupcomplete ? cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) : CYW43_LINK_DOWN;
        if (live < 0) iret = live;
        else if (WIFIconnected) iret = live;
        else iret = LastWifiErr ? LastWifiErr : live;
        targ = T_INT;
        return 1;
    }
    if (checkstring(ep, "TCPIP STATUS")) {
        iret = startupcomplete ? cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) : CYW43_LINK_DOWN;
        targ = T_INT;
        return 1;
    }
    if (checkstring(ep, "TELNET")) {
        /* the PicoMite's (core/MM_Misc.c): 1 while a telnet client is attached to the console, 0 otherwise.
           telnet_pcb_no is 99 when no client is connected, but a clean remote close can clear the pcb slot
           without resetting it, so require both to agree. TCPstate is NULL until the TCP server has been started. */
        iret = 0;
        if (TCPstate) {
            int pcb = TCPstate->telnet_pcb_no;
            if (pcb >= 0 && pcb < MaxPcb && TCPstate->client_pcb[pcb])
                iret = 1;
        }
        targ = T_INT;
        return 1;
    }
    if ((tp = checkstring(ep, "OPTION")) && checkstring(tp, "SSID")) {
        strcpy((char *)sret, (char *)Option.SSID);
        CtoM(sret);
        targ = T_STR;
        return 1;
    }
    return 0;
}

/* ---- a program's start and end, and its interrupts: the PicoMite's (core/MM_Misc.c check_interrupt, misc/External.c
   ClearExternalIO, core/Commands.c's RUN) ---- */
// check_interrupt (MM_Misc.c): WEB TCP INTERRUPT, MQTT's and WEB UDP INTERRUPT's, in the PicoMite's order; NULL if none
char *WebInterrupt(void) {
    char *a;
    if (TCPreceived && TCPreceiveInterrupt) {
        a = (char *)TCPreceiveInterrupt;
        TCPreceived = 0;
        return a;
    }
    if (MQTTComplete && MQTTInterrupt != NULL) {
        MQTTComplete = false;
        return (char *)MQTTInterrupt;
    }
    if (UDPreceive && UDPinterrupt != NULL) {
        UDPreceive = false;
        return (char *)UDPinterrupt;
    }
    return NULL;
}

// ClearExternalIO (External.c): MQTT and the TCP client closed, as the PicoMite's, and the interrupts the old program
// set forgotten (the PicoMite keeps TCPreceiveInterrupt and UDPinterrupt, which point into the old program)
void WebClearIO(void) {
    optionsuppressstatus = 0;                           // OPTION WEB MESSAGES OFF lasts until the program's end, as the
    MQTTInterrupt = NULL;                               // PicoMite's ClearRuntime has it (RUN and NEW)
    MQTTComplete = 0;
    TCPreceiveInterrupt = NULL;
    TCPreceived = false;
    UDPinterrupt = NULL;
    UDPreceive = false;
    if (!startupcomplete) return;
    closeMQTT();
    close_tcpclient();
}

// RUN, with a program to run (Commands.c): the web server's connections from before closed, as the PicoMite's
void WebRunStart(void) {
    if (startupcomplete) cleanserver();
}

// the four pins (32 WL_ON, 33 CS, 38 data, 40 clock) and SPI2 are Wi-Fi's: SETPIN, SPI2 OPEN and SPI2Close leave them.
// Not on a G3 V1.2, whose module is on the board, on SPI6: the pins and SPI2 stay BASIC's.
static void reserve_pins(void) {
    if (RM2OnBoard) return;
    ExtCfg(32, EXT_BOOT_RESERVED, 0);
    ExtCfg(33, EXT_BOOT_RESERVED, 0);
    ExtCfg(38, EXT_BOOT_RESERVED, 0);
    ExtCfg(40, EXT_BOOT_RESERVED, 0);
}

// the console's output (main.c putConsole and SerUSBPutC, never from an interrupt): the telnet console's copy and the
// network pumped, as the PicoMite's console output does (TelnetPutC, then ProcessWeb)
void WebConsolePutC(int c) {
    if (NetHold) return;
    TelnetPutC(c, 0);
    WebPoll();
}

/* A G1's display memory lent to a loadable program built --vram (Loader.c): the network block is in it. From WebLend,
   before the loader writes anything there, nothing of the network runs (the poll, telnet's output) until WebGiveBack,
   once the program has ended; then the network starts again from nothing, as at power up: the network code's data
   and bss set again (the linker script puts them first in .data and .bss, between _snetdata and _enetdata, _snetbss
   and _enetbss) and WebStart, which starts the radio and joins again (about 3 s). The interrupts a BASIC program set
   are kept. (docs/Web_Plan.md: Peter, 15 Nov 2026) */
void WebLend(void) {
    if (G1Hardware && NetBlock) NetHold = 1;
}
void WebGiveBack(void) {
    extern char _sinetdata[], _snetdata[], _enetdata[], _snetbss[], _enetbss[];
    char *t = TCPreceiveInterrupt, *u = UDPinterrupt, *m = MQTTInterrupt;
    if (!NetHold) return;
    memcpy(_snetdata, _sinetdata, _enetdata - _snetdata);   // (Bluetooth's heap too: WebStart starts it again)
    memset(_snetbss, 0, _enetbss - _snetbss);                       // (NetHold 0 again)
    TCPreceiveInterrupt = t;
    UDPinterrupt = u;
    MQTTInterrupt = m;
    ExtCurrentConfig[32] = ExtCurrentConfig[33] = ExtCurrentConfig[38] = ExtCurrentConfig[40] = EXT_NOT_CONFIG;   // as at
    WebBlockSet();                                  // power up: WebStart reserves them again (ExtCfg refuses a pin that
    WebStart();                                     // is boot-reserved already), or frees them with no module
}
