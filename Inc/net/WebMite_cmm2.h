/* WebMite_cmm2.h: what the PicoMite's network code (Src/net/picomite, copied by tools/port_net.py) has from the
   PicoMite's own headers and core, on the Colour Maximite 2 (docs/Web_Plan.md, step 2). CMakeLists.txt makes it the
   first include of those files only (-include); the CMM2's network glue (Src/net/WebMite_cmm2.c) includes it too.

   It comes before the CMM2's MMBasic_Includes.h (which has no include guard: MMBasic.c includes it again to build the
   command and token tables), so it needs none of MMBasic's types. Copied as they are: the PicoMite's misc/Custom.h
   (its PICOMITEWEB includes, types and declarations), Hardware_Includes.h (its PICOMITEWEB declarations), misc/FileIO.h
   (the WiFi country codes) and core/MM_Misc.h (WEB NTP's time). Then the RP2040's and the PicoMite core's ways, on the
   CMM2:
     __not_in_flash_func(f)      f (the network code is in bank 2: the flash here too)
     time_us_64()                MicrosNow()
     pico-sdk's absolute_time_t and alarm_id_t (the types NTP_T holds)
     getcsargs(x, y)             the CMM2's getargs(x, y, ",")
     GetTempMainMemory           GetTempMemory
     Timer4                      the CMM2's Timer4 (Timers.c: the 1 ms countdown, as the PicoMite's)
     calloc, malloc, free        lwIP's mem_calloc, mem_malloc and mem_free: the network block (NetBlock.h). The
                                 C library's heap grows up from the bottom of MMBasic's SDRAM heap, which a G1 lends to
                                 its loadable programs; the state the PicoMite keeps there (it must outlive a RUN) goes
                                 into the block, which is lwIP's heap. */
#ifndef WEBMITE_CMM2_H
#define WEBMITE_CMM2_H

#define PICOMITEWEB 1
#include <stdlib.h>                             // (declared before calloc and free become lwIP's below)
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "Loader.h"                             // (MicrosNow)

/* ---- the RP2040's ways, on the CMM2 ---- */
#define __not_in_flash_func(f) f
#define time_us_64() MicrosNow()
typedef uint64_t absolute_time_t;
typedef int32_t alarm_id_t;
static inline absolute_time_t get_absolute_time(void) { return MicrosNow(); }
static inline absolute_time_t make_timeout_time_ms(uint32_t ms) { return MicrosNow() + 1000ull * ms; }
static inline int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to) { return (int64_t)(to - from); }

/* ---- the PicoMite's configuration.h ---- */
#define MaxPcb 8
#define MAXERRMSG 64

/* ---- the PicoMite's misc/Custom.h, PICOMITEWEB ---- */
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/err.h"

#define TCP_READ_BUFFER_SIZE 2048
/* TCP Server structure */
typedef struct TCP_SERVER_T_
{
   struct tcp_pcb *server_pcb;
   struct tcp_pcb *telnet_pcb;
   struct tcp_pcb *client_pcb[MaxPcb];
   volatile int inttrig[MaxPcb];
   uint8_t *buffer_sent[MaxPcb];
   uint8_t *buffer_recv[MaxPcb];
   volatile int sent_len[MaxPcb];
   volatile int recv_len[MaxPcb];
   volatile int total_sent[MaxPcb];
   volatile int to_send[MaxPcb];
   volatile uint64_t pcbopentime[MaxPcb];
   volatile int keepalive[MaxPcb];
   volatile int telnet_pcb_no;
   volatile int telnet_init_sent;
} TCP_SERVER_T;

/* A server slot whose idle connection the housekeeping closed (WiFi.c) is kept from reuse for a while
   with this mark in it: it holds no connection. pcb_live() is true only for a real connection. */
#define PCB_TIMED_OUT ((struct tcp_pcb *)44)
#define pcb_live(p) ((p) != NULL && (p) != PCB_TIMED_OUT)

/* NTP structure */
typedef struct NTP_T_
{
   ip_addr_t ntp_server_address;
   bool dns_request_sent;
   struct udp_pcb *ntp_pcb;
   absolute_time_t ntp_test_time;
   alarm_id_t ntp_resend_alarm;
   volatile bool complete;
} NTP_T;

/* TCP Client structure.
   tcp_pcb is held as a void* because MMTCPclient.c uses the altcp API,
   which means the underlying pointer is a struct altcp_pcb* (not a
   struct tcp_pcb*). altcp_* functions transparently dispatch to either
   plain TCP or TLS depending on how the pcb was created. */
typedef struct TCP_CLIENT_T_
{
   void *volatile tcp_pcb;
   ip_addr_t remote_addr;
   volatile uint8_t *buffer;
   volatile int buffer_len;
   volatile bool complete;
   volatile bool connected;
   volatile bool tls;
   volatile int BUF_SIZE;
   volatile int TCP_PORT;
   volatile int *buffer_write;
   volatile int *buffer_read;
   volatile char *hostname;
} TCP_CLIENT_T;

/* ============================================================================
 * PICOMITEWEB - External variables
 * ============================================================================ */
extern TCP_SERVER_T *TCPstate;
extern TCP_CLIENT_T *TCP_CLIENT;

extern volatile bool TCPreceived;
extern char *TCPreceiveInterrupt;

extern char *UDPinterrupt;
extern volatile bool UDPreceive;

/* ============================================================================
 * PICOMITEWEB - Function declarations
 * ============================================================================ */

/* NTP functions */
void GetNTPTime(void);
void cmd_ntp(unsigned char *tp);

/* PING command */
void cmd_ping(unsigned char *tp);

/* TCP functions */
void checkTCPOptions(void);
void open_tcp_server(void);
int cmd_tcpclient(void);
int cmd_tcpserver(void);
void cleanserver(void);
err_t tcp_server_close(void *arg, int pcb);
void tcp_server_forget(TCP_SERVER_T *state, int pcb);
err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb, int pcb);
void checksent(void *arg, int fn, int pcb);
void starttelnet(struct tcp_pcb *client_pcb, int pcb, void *arg);

/* TCP Client functions */
TCP_CLIENT_T *tcp_client_init(void);
void tcp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

/* TLS sub-command dispatch (only present when PICOMITEWEB_TLS is enabled) */
#ifdef PICOMITEWEB_TLS
int cmd_tls(void);
#endif

/* UDP functions */
void open_udp_server(void);
void cmd_udp(unsigned char *tp);

/* TCP client functions */
void close_tcpclient(void);

/* MQTT functions */
int cmd_mqtt(void);
void closeMQTT(void);

/* TFTP functions */
int cmd_tftp_server_init(void);

/* TLS functions */
int cmd_tls(void);

/* Telnet functions */
void TelnetPutC(int c, int flush);

extern unsigned char topicbuff[256];            // (STRINGSIZE, as the PicoMite's MMMqtt.c has them)
extern unsigned char messagebuff[256];
extern unsigned char addressbuff[20];
extern char *MQTTInterrupt;
extern volatile bool MQTTComplete;

/* ---- the PicoMite's Hardware_Includes.h, PICOMITEWEB ---- */
extern volatile int WIFIconnected;
extern volatile int LastWifiErr;
extern volatile int scantimer;
extern int startupcomplete;
extern int WebScanActive; // WEB SCAN is waiting for results: ProcessWeb polls with no connection
extern volatile int WebInCallback; // inside an lwIP callback: WebHeartbeat leaves the chip alone
void WebHeartbeat(uint64_t timenow);
void ProcessWeb(int mode);
void WebConnect(void);
void web_async_set_error(const char *msg);
void web_async_check_error(void);
struct altcp_tls_config;
void picomite_tls_init(void);
struct altcp_tls_config *picomite_tls_get_client_config(void);
int picomite_tls_set_ca(const unsigned char *ca_buf, size_t ca_len);
void picomite_tls_clear_ca(void);
bool picomite_tls_verify_is_required(void);
void setwifi(unsigned char *tp);

/* ---- the PicoMite's misc/FileIO.h: the WiFi regulatory domain, an index in Option.wifi_country_code ---- */
enum wifi_country_e
{
        WIFI_COUNTRY_WORLDWIDE = 0,
        WIFI_COUNTRY_AUSTRALIA,
        WIFI_COUNTRY_AUSTRIA,
        WIFI_COUNTRY_BELGIUM,
        WIFI_COUNTRY_BRAZIL,
        WIFI_COUNTRY_CANADA,
        WIFI_COUNTRY_CHILE,
        WIFI_COUNTRY_CHINA,
        WIFI_COUNTRY_COLOMBIA,
        WIFI_COUNTRY_CZECH_REPUBLIC,
        WIFI_COUNTRY_DENMARK,
        WIFI_COUNTRY_ESTONIA,
        WIFI_COUNTRY_FINLAND,
        WIFI_COUNTRY_FRANCE,
        WIFI_COUNTRY_GERMANY,
        WIFI_COUNTRY_GREECE,
        WIFI_COUNTRY_HONG_KONG,
        WIFI_COUNTRY_HUNGARY,
        WIFI_COUNTRY_ICELAND,
        WIFI_COUNTRY_INDIA,
        WIFI_COUNTRY_ISRAEL,
        WIFI_COUNTRY_ITALY,
        WIFI_COUNTRY_JAPAN,
        WIFI_COUNTRY_KENYA,
        WIFI_COUNTRY_LATVIA,
        WIFI_COUNTRY_LIECHTENSTEIN,
        WIFI_COUNTRY_LITHUANIA,
        WIFI_COUNTRY_LUXEMBOURG,
        WIFI_COUNTRY_MALAYSIA,
        WIFI_COUNTRY_MALTA,
        WIFI_COUNTRY_MEXICO,
        WIFI_COUNTRY_NETHERLANDS,
        WIFI_COUNTRY_NEW_ZEALAND,
        WIFI_COUNTRY_NIGERIA,
        WIFI_COUNTRY_NORWAY,
        WIFI_COUNTRY_PERU,
        WIFI_COUNTRY_PHILIPPINES,
        WIFI_COUNTRY_POLAND,
        WIFI_COUNTRY_PORTUGAL,
        WIFI_COUNTRY_SINGAPORE,
        WIFI_COUNTRY_SLOVAKIA,
        WIFI_COUNTRY_SLOVENIA,
        WIFI_COUNTRY_SOUTH_AFRICA,
        WIFI_COUNTRY_SOUTH_KOREA,
        WIFI_COUNTRY_SPAIN,
        WIFI_COUNTRY_SWEDEN,
        WIFI_COUNTRY_SWITZERLAND,
        WIFI_COUNTRY_TAIWAN,
        WIFI_COUNTRY_THAILAND,
        WIFI_COUNTRY_TURKEY,
        WIFI_COUNTRY_UK,
        WIFI_COUNTRY_USA,
        WIFI_COUNTRY_COUNT
};
uint32_t wifi_country_to_cyw43(unsigned char idx);
const char *wifi_country_to_string(unsigned char idx);
int wifi_country_from_string(const char *iso);
extern int FatFSFileSystem;                     // (TFTP's drive: the CMM2's are all FatFS, so 1)
extern char id_out[12];                         // (the board's ID, the default host name's end)
int SDCardConfigured(void);

/* ---- the PicoMite's core/MM_Misc.h: WEB NTP's time ---- */
extern int64_t TimeOffsetToUptime;
extern uint32_t TimeSubSecondUs;
extern bool optionsuppressstatus;

/* ---- the PicoMite core's ways, on the CMM2 ---- */
#define getcsargs(x, y) getargs((char **)(x), y, ",")
#define GetTempMainMemory(n) GetTempMemory(n)
static inline __attribute__((always_inline)) int mytoupper(int c)
{
    if ((unsigned)(c - 'a') < 26u)
        c &= ~0x20;
    return c;
}
extern volatile unsigned int Timer4;
#define g_VarIndex VarIndex
#define g_OptionBase OptionBase
#define heap_memory_size ((int)(SDRAMEXEC - SDRAMBASE))   // (the PicoMite's MMBasic heap size: the CMM2's SDRAM heap)
extern volatile int Keycomplete;
extern volatile char ConsoleRxBuf[];
extern volatile int ConsoleRxBufHead, ConsoleRxBufTail;
int ExistsFile(char *p);
int FileSize(char *p);
void set_time_epoch_us(int64_t epoch_us);
extern int clocktimer;                          // (OPTION RTC AUTO's day; the CMM2 keeps its own RTC: Src/net/WebMite_cmm2.c)
#define RTC_REREAD_MS (24 * 3600 * 1000)

/* ---- the C library's heap, the network block's here ---- */
#define calloc(n, s) mem_calloc((mem_size_t)(n), (mem_size_t)(s))
#define malloc(s) mem_malloc((mem_size_t)(s))
#define free(p) mem_free(p)

#endif
