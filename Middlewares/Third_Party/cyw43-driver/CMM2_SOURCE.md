# cyw43-driver in the CMM2 firmware

George Robotics' driver for the Infineon CYW43439 (LICENSE: MIT), from https://github.com/georgerobotics/cyw43-driver
at 055d64274b014dd7b1c2fc94d26e8a18face7124, the commit the PicoMite's pico-sdk (2.3.1) has. Unchanged.

Taken: src/*.h, and src/cyw43_ll.c, cyw43_ctrl.c, cyw43_stats.c and cyw43_lwip.c (the SPI bus's register code is
pico-sdk's, in Src/net/cyw43_bus_cmm2.c, so src/cyw43_spi.c is left out, with the SDIO and UART files); from firmware/,
the Wi-Fi blob w43439A0_7_95_49_00_combined.h, the NVRAM wifi_nvram_43439.h and README.md. The blob is Infineon's:
its terms are to be read before release (docs/Web_Plan.md).
