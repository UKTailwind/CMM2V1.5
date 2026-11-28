# cyw43-driver in the CMM2 firmware

George Robotics' driver for the Infineon CYW43439 (LICENSE: George Robotics' licence of 2019-2022, which allows
redistribution for personal, non-commercial use only; the driver moved to the MIT licence upstream on 2 Sep 2026, at
1828b5a, ten commits later), from https://github.com/georgerobotics/cyw43-driver
at 055d64274b014dd7b1c2fc94d26e8a18face7124, the commit the PicoMite's pico-sdk (2.3.1) has.

Changes (27 Nov 2026), all in src/cyw43_ll.c, marked "CMM2":
- **A packet the status announces is never left in the chip.** In the SPI cyw43_ll_sdpcm_poll_device, when the read of
  a packet the status had announced gave nothing (an all-zero header: the chip had not taken the read; on 28 Nov 2026
  the cause was found to be the clock starting too soon after CS fell, now fixed in Src/net/cyw43_bus_cmm2.c, and the
  retry is kept for any read that still comes back empty), upstream cleared had_successful_packet, so that only the
  chip's interrupt would bring the next read. But the chip raises its packet interrupt once for a packet, and one left
  in its FIFO raises no other: the packet stayed, nothing more came, the chip's bus was put to sleep with it and would
  not wake (seen through a trace of the bus transfers). Now had_successful_packet stays set, so the status is read
  again at the next poll and the read tried again (up to eight times, then the frame is dropped); the same after an
  invalid length and after a header mismatch. cyw43_ll_has_work is also true while had_successful_packet is set, so
  that the poll comes without the interrupt. (pico-sdk's code is the same as upstream's here: a Pico W has the gap
  too, met less often.)
- **A header mismatch** (the length and its complement disagree: a read out of step with the chip's F2 FIFO) ends the
  frame (SPI_FRAME_CONTROL), as an invalid length just above it does. Upstream only returned, so the chip waited for
  the rest of that frame.
- **The gSPI's speed mode** from CYW43_SPI_HIGH_SPEED, HIGH_SPEED_MODE by default as upstream (and as
  Inc/net/cyw43_configport.h has it): the chip changes its data on SCK's rising edge, and the STM32's SPI2 (mode 0)
  reads it on the next rising edge, a whole clock later, which the add-on module's 470 R line needs above 16 MHz.
  Src/net/cyw43_bus_cmm2.c runs SPI2 at 8 MHz while the chip is in normal mode (its start) and at 32 MHz in
  high-speed mode.

Taken: src/*.h, and src/cyw43_ll.c, cyw43_ctrl.c, cyw43_stats.c and cyw43_lwip.c (the SPI bus's register code is
pico-sdk's, in Src/net/cyw43_bus_cmm2.c, so src/cyw43_spi.c is left out, with the SDIO and UART files); from firmware/,
the Wi-Fi blob w43439A0_7_95_49_00_combined.h, the NVRAM wifi_nvram_43439.h and README.md. The blob is Infineon's:
its terms are to be read before release (docs/Web_Plan.md).
