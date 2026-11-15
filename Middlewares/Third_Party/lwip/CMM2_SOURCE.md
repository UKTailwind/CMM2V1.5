# lwIP in the CMM2 firmware

lwIP 2.2.1 (COPYING: BSD-3-Clause), from https://github.com/lwip-tcpip/lwip at 77dcd25a72509eb83f72b033d219b1d40cd8eb95
(STABLE-2_2_1_RELEASE), the commit the PicoMite's pico-sdk (2.3.1) has. Unchanged.

Taken: src/include, src/core/*.c, src/core/ipv4/*.c, src/netif/ethernet.c, and src/apps/altcp_tls/'s
altcp_tls_mbedtls_mem.c and headers. altcp_tls_mbedtls.c is pico-sdk's copy for mbedTLS 3, as the PicoMite builds it
(Src/net). The settings are the PicoMite's (Inc/net/lwipopts.h, with the CMM2's section at its end).
