/***************************************************************************

CMM2 MMBasic
tusb_config.h

TinyUSB configuration for the Colour Maximite 2

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
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
#define CFG_TUH_MSC                 1                   // USB flash drives, as C: (USBDrive.c)
#define CFG_TUH_CDC                 4                   // USB serial devices, as COM4-COM7 (Serial.c)
#define CFG_TUH_CDC_FTDI            1                   // the vendor USB-UART bridges, through the same CDC calls
#define CFG_TUH_CDC_CP210X          1
#define CFG_TUH_CDC_CH34X           1
#define CFG_TUH_CDC_PL2303          1                   // Prolific, common in GPS receivers
#define CFG_TUH_CDC_TX_BUFSIZE      256                 // as the UARTs' TX_BUFFER_SIZE, so LOF() means the same on every COM port
#define CFG_TUH_CDC_LINE_CONTROL_ON_ENUM 0x00           // DTR and RTS are raised by OPEN, not on connection
#define CFG_TUH_CDC_LINE_CODING_ON_ENUM {115200, CDC_LINE_CODING_STOP_BITS_1, CDC_LINE_CODING_PARITY_NONE, 8}

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
