# cyw43-driver in the CMM2 firmware

George Robotics' driver for the Infineon CYW43439 (LICENSE: George Robotics' licence of 2019-2022, which allows
redistribution for personal, non-commercial use only; the driver moved to the MIT licence upstream on 2 Sep 2026, at
1828b5a, ten commits later), from https://github.com/georgerobotics/cyw43-driver
at 055d64274b014dd7b1c2fc94d26e8a18face7124, the commit the PicoMite's pico-sdk (2.3.1) has.

One change (27 Nov 2026): in src/cyw43_ll.c's SPI cyw43_ll_sdpcm_poll_device, a packet whose header's length and
its complement disagree ("hdr mismatch", a read out of step with the chip's F2 FIFO) now ends the frame
(SPI_FRAME_CONTROL) and clears had_successful_packet, as an invalid length just above it does. Upstream only returned,
so the chip waited for the rest of that frame and never signalled another packet: the network dead until a restart.
On the CMM2 the wrong reads came from the gSPI's high-speed mode (see the next).

A second (27 Nov 2026): src/cyw43_ll.c's bus set-up takes the gSPI's speed mode from CYW43_SPI_HIGH_SPEED,
HIGH_SPEED_MODE by default as upstream, 0 (normal mode) in Inc/net/cyw43_configport.h. In high-speed mode the chip
changes its data on SCK's rising edge, and the STM32's SPI2 (mode 0) read it on that same edge, held only by the
chip's output delay: under load (the web browser in MODE 11,16) reads now and then came back wrong, the chip's
packet FIFO went out of step and the network died. In normal mode the data changes on the falling edge, half a
clock before it is read.

Taken: src/*.h, and src/cyw43_ll.c, cyw43_ctrl.c, cyw43_stats.c and cyw43_lwip.c (the SPI bus's register code is
pico-sdk's, in Src/net/cyw43_bus_cmm2.c, so src/cyw43_spi.c is left out, with the SDIO and UART files); from firmware/,
the Wi-Fi blob w43439A0_7_95_49_00_combined.h, the NVRAM wifi_nvram_43439.h and README.md. The blob is Infineon's:
its terms are to be read before release (docs/Web_Plan.md).
