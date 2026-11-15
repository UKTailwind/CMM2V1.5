# Mbed TLS in the CMM2 firmware

Mbed TLS 3.6.6 (LICENSE: Apache-2.0 or GPL-2.0-or-later), from https://github.com/Mbed-TLS/mbedtls at
0bebf8b8c7f07abe3571ded48a11aa907a1ffb20, the commit the PicoMite's pico-sdk (2.3.1) has. Unchanged.

Taken: include/ and library/ (every library/*.c is built; the configuration, the PicoMite's net/mbedtls_config.h in
Inc/net, leaves most of them empty, and the linker keeps only what is used).
