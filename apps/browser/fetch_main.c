/* fetch_main.c: fetch.app, the curl tool (curl 8.23.0) for the Colour Maximite 2, step 1 of docs/Web_Browser_Study.md.
   curl's own main (src/tool_main.c) is renamed curl_tool_main by port/curl_config.h, and called from here.
       RUN "fetch.app", [options] url ...
   as curl takes them ("--help all" lists them). The CA bundle is A:/cacert.pem unless --cacert names another.
       RUN "fetch.app", --cmm2-bench a.b.c.d port /path
   measures the network services themselves: what net_poll costs, and an HTTP download through the services directly
   and through port/cmm2_net.c's sockets.
       RUN "fetch.app", --cmm2-mem [curl's options] url ...
   runs curl as usual, then prints the heap's high-water mark. MIT License, as cmm2img.h. */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cmm2.h"

int curl_tool_main(int argc, char *argv[]);

static uint8_t buf[16384];

static void report(const char *what, uint64_t bytes, uint64_t us, uint32_t polls) {
    printf("%-34s %8lu bytes in %6lu ms: %5lu KB/s, %lu net_polls\n", what, (unsigned long)bytes,
           (unsigned long)(us / 1000), (unsigned long)(us ? bytes * 1000000 / us / 1024 : 0), (unsigned long)polls);
}

/* an HTTP GET through the services alone: mode 0 polls before every sock_recv, mode 1 only when nothing came */
static void bench_raw(const uint8_t ip[4], int port, const char *req, int mode) {
    int h = cmm2->sock_open(1), r, st;
    uint64_t t0, bytes = 0;
    uint32_t polls = 0;
    if(h < 0) { printf("sock_open %d\n", h); return; }
    cmm2->sock_connect(h, ip, port);
    while(!((st = cmm2->sock_state(h)) & 1)) {
        if(st < 0) { printf("connect %d\n", st); cmm2->sock_close(h); return; }
        cmm2->net_poll();
    }
    cmm2->sock_send(h, (const uint8_t *)req, strlen(req), NULL, 0);
    t0 = cmm2->micros();
    for(;;) {
        if(mode == 0) { cmm2->net_poll(); polls++; }
        r = cmm2->sock_recv(h, buf, sizeof(buf), NULL, NULL);
        if(r > 0) { bytes += r; continue; }
        if(r == 0) break;
        if(r != -11) { printf("recv %d\n", r); break; }
        if(mode == 1) { cmm2->net_poll(); polls++; }
    }
    report(mode ? "services, poll when empty" : "services, poll before each recv", bytes, cmm2->micros() - t0, polls);
    cmm2->sock_close(h);
}

/* the same through port/cmm2_net.c's blocking sockets */
static void bench_sock(const uint8_t ip[4], int port, const char *req) {
    struct sockaddr_in a;
    int fd = socket(AF_INET, SOCK_STREAM, 0), r;
    uint64_t t0, bytes = 0;
    if(fd < 0) { printf("socket errno %d\n", errno); return; }
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    memcpy(&a.sin_addr.s_addr, ip, 4);
    if(connect(fd, (struct sockaddr *)&a, sizeof(a)) < 0) { printf("connect errno %d\n", errno); closesocket(fd); return; }
    send(fd, req, strlen(req), 0);
    t0 = cmm2->micros();
    while((r = recv(fd, buf, sizeof(buf), 0)) > 0) bytes += r;
    report("cmm2_net.c sockets", bytes, cmm2->micros() - t0, 0);
    closesocket(fd);
}

static int bench(int argc, char **argv) {
    struct in_addr a;
    uint8_t ip[4];
    char req[300];
    uint64_t t0;
    int i, port;
    if(argc < 5 || !inet_aton(argv[2], &a)) {
        puts("RUN \"fetch.app\", --cmm2-bench a.b.c.d port /path");
        return 2;
    }
    memcpy(ip, &a.s_addr, 4);
    port = atoi(argv[3]);
    snprintf(req, sizeof(req), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", argv[4], argv[2]);
    if(cmm2->version < 21 || cmm2->net_poll() != 2) { puts("no network"); return 1; }
    t0 = cmm2->micros();
    for(i = 0; i < 1000; i++) cmm2->net_poll();
    printf("net_poll, idle: %lu us each\n", (unsigned long)((cmm2->micros() - t0) / 1000));
    bench_raw(ip, port, req, 0);
    bench_raw(ip, port, req, 1);
    bench_sock(ip, port, req);
    return 0;
}

int main(int argc, char **argv) {
    int r, mem = 0;
    if(argc > 1 && !strcmp(argv[1], "--cmm2-bench")) return bench(argc, argv);
    if(argc > 1 && !strcmp(argv[1], "--cmm2-mem")) {           /* curl as usual, then the heap's high-water mark */
        mem = 1;
        argv[1] = argv[0];
        argv++;
        argc--;
    }
    r = curl_tool_main(argc, argv);
    fflush(stdout);
    fflush(stderr);
    if(mem) {
        struct mallinfo mi = mallinfo();
        fprintf(stderr, "heap: %lu KB taken from the system at most, %lu KB in use at the end\n",
                (unsigned long)(mi.arena / 1024), (unsigned long)(mi.uordblks / 1024));
    }
    return r;
}
