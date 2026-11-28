/* cmm2_main.c: browser.app's start (apps/browser, docs/Web_Browser_Study.md). NetSurf's framebuffer front end keeps its
   own main (frontends/framebuffer/gui.c, compiled with -Dmain=netsurf_main), called from here with the CMM2's surface
   unless the words say otherwise:
       RUN "browser.app", [--shot FILE] [NetSurf's options] [url]
   - The page is drawn in the mode the screen is in, which must be one of 16-bit colour: MODE 9,16 (1024x768) or
     MODE 11,16 (720p), say, set before the RUN. The browser does not change the mode (Peter, 27 Nov 2026: a mode change
     is a large event on the SDRAM's bus, two of them for every run), so -f cmm2 -w <its width> -h <its height> -b 16
     are put in front, from gfx_target (MMBasic's HRes and VRes).
   - --shot FILE: a test run. When the first page has finished loading (cmm2_page_done, from patches/netsurf-0001) and
     been drawn, the screen is saved as FILE (SAVE PNG's), the times and the memory are printed, and NetSurf quits.
     --then URL (or back, or wait for 10 s), up to eight times: steps taken first, each when the page before is done (cmm2_shot_poll).
   MIT License, as Inc/cmm2img.h. */
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "cmm2.h"
#include "utils/errors.h"
#include "utils/nsurl.h"
#include "netsurf/browser_window.h"
#include "desktop/browser_history.h"
#include "content/fetch.h"
#include "content/hlcache.h"

struct browser_window;
int netsurf_main(int argc, char **argv);
bool browser_window_stop_available(struct browser_window *bw);   /* (include/netsurf/browser_window.h) */

#define FONT_CHECK "/browser/fonts/DejaVuSans.ttf"   /* (the default face: NetSurf cannot start without it) */

static const char *shot;                        /* --shot's file, or NULL */
static struct browser_window *shot_bw;          /* the window the throbber stopped in */
static uint64_t t_start, t_done;
static int done_passes = -1;                    /* the surface's input calls since the page was done, or -1 */

/* the throbber stopped: NetSurf stops it in several places (the first, empty, page among them), so this only notes
   the window; cmm2_shot_poll waits until nothing in it is loading */
static unsigned stack_used_kb(void);
void cmm2_big_use(unsigned long *in_use, unsigned long *size);   /* (browser_malloc.c: the big blocks' pool) */

void cmm2_page_done(struct browser_window *bw) {
    shot_bw = bw;
    if(shot == NULL) {                          /* (in use: the memory on the serial console, the screen being the page's) */
        struct mallinfo mi = mallinfo();
        unsigned long bu, bs;
        cmm2_big_use(&bu, &bs);
        fprintf(stderr, "browser: a page done; heap %lu KB in use, %lu KB from the system; big blocks %lu KB of %lu "
                "(the display memory); stack %u KB at most\n", (unsigned long)(mi.uordblks / 1024),
                (unsigned long)(mi.arena / 1024), bu / 1024, bs / 1024, stack_used_kb());
    }
}

/* the stack, painted at the start from its bottom (the heap's end, as the start block gives it) to just under the
   running frame, so that its deepest use can be found: the stack sits on top of the heap, and a stack that outgrows
   its room writes over the newest heap blocks (27 Nov 2026: a style sheet's error message came out as garbage) */
#define STACK_PAINT 0xA5A5A5A5u
static uint32_t *stack_lo, *stack_hi;

static void stack_paint(void) {
    uint32_t here, *p;
    stack_lo = (uint32_t *)(((uintptr_t)cmm2_start->heap + cmm2_start->heap_size + 3) & ~3u);
    stack_hi = (uint32_t *)(((uintptr_t)&here - 256) & ~3u);
    for(p = stack_lo; p < stack_hi; p++) *p = STACK_PAINT;
}

static unsigned stack_used_kb(void) {
    uint32_t *p = stack_lo;
    while(p < stack_hi && *p == STACK_PAINT) p++;
    return (unsigned)(((uintptr_t)stack_hi - (uintptr_t)p) / 1024) + 1;
}

static void stats(void) {
    struct mallinfo mi = mallinfo();
    unsigned long bu, bs;
    cmm2_big_use(&bu, &bs);
    fprintf(stderr, "browser: the page done %.2f s after the start; heap %lu KB at most, %lu KB in use; big blocks "
            "%lu KB of %lu; stack %u KB of %u\n", (double)(t_done - t_start) / 1e6, (unsigned long)(mi.arena / 1024),
            (unsigned long)(mi.uordblks / 1024), bu / 1024, bs / 1024, stack_used_kb(),
            (unsigned)(((uintptr_t)stack_hi - (uintptr_t)stack_lo) / 1024));
}

/* --then's steps (28 Nov 2026: a heap damaged by following a link from one Wikipedia page to another, found without
   the mouse): each a URL to go to, as a link's click goes (into the history), or back for Back, taken once the page
   before is done and drawn; --shot's picture is of the last */
#define THEN_MAX 8
static const char *then[THEN_MAX];
static int nthen, then_next;

static uint64_t wait_until;                     /* --then wait: 10 s before the next step (a download's time) */

static void then_step(void) {
    const char *s = then[then_next++];
    nserror r;
    if(!strcmp(s, "wait")) {
        wait_until = cmm2->micros() + 10000000u;
        r = NSERROR_OK;
    } else if(!strcmp(s, "back")) {
        r = browser_window_history_back(shot_bw, false);
    } else {
        nsurl *u;
        if((r = nsurl_create(s, &u)) == NSERROR_OK) {
            r = browser_window_navigate(shot_bw, u, NULL, BW_NAVIGATE_HISTORY, NULL, NULL, NULL);
            nsurl_unref(u);
        }
    }
    fprintf(stderr, "browser: then %s: %s\n", s, r == NSERROR_OK ? "under way" : "failed");
}

/* A page that runs out of memory (28 Nov 2026, Peter: "display a message when the page is too complex rather than just
   a blank screen"): the first request malloc cannot meet stops its load, and NetSurf's own error page (about:query/
   fetcherror, as for a failed fetch) says why, with its Back and Try Again. It replaces the page, which frees its
   memory (patches/netsurf-0005), and the browser goes on. Until then a page that ran out came up blank or in part, and
   its load asked again and again for what it could not have (2,500 requests in 40 s). From the surface's input. */
unsigned long cmm2_failed_requests(void);       /* (browser_malloc.c) */
static unsigned long failed_seen;

static void too_large(struct browser_window *bw) {
    struct fetch_multipart_data *post = NULL;
    nsurl *err, *site = browser_window_access_url(bw);
    browser_window_stop(bw);
    hlcache_clean_soon();                       /* (the stopped page freed: a page stopped while loading counts as
                                                   loading, which the cache's cleaner otherwise leaves) */
    if(nsurl_create("about:query/fetcherror", &err) != NSERROR_OK) return;
    if(fetch_multipart_data_new_kv(&post, "siteurl", site != NULL ? nsurl_access(site) : "about:blank") == NSERROR_OK &&
       fetch_multipart_data_new_kv(&post, "reason", "This page needs more memory than the browser has on the Colour "
                                   "Maximite 2 (about 20 MB), so it was stopped.") == NSERROR_OK)
        browser_window_navigate(bw, err, NULL,
                                BW_NAVIGATE_HISTORY | BW_NAVIGATE_NO_TERMINAL_HISTORY_UPDATE | BW_NAVIGATE_INTERNAL,
                                NULL, post, NULL);
    fetch_multipart_data_destroy(post);
    nsurl_unref(err);
    fputs("browser: out of memory: the page stopped, and the error page shown\n", stderr);
}

void cmm2_memory_poll(void) {
    unsigned long n = cmm2_failed_requests();
    if(n == failed_seen) return;
    failed_seen = n;
    if(shot_bw != NULL) too_large(shot_bw);
}

/* from the surface's input, once a pass of the front end's loop: 1 when a test run is finished (NetSurf then quits).
   The page is done when its window has nothing loading; five passes later its redraw has been drawn and copied. A
   page that never finishes is saved as it is after 60 s. After the save the page stays on the screen for 3 s, to be
   seen (Peter, 28 Nov 2026), and then NetSurf quits. */
static uint64_t hold_until;                     /* the end of that 3 s, or 0 */

int cmm2_shot_poll(void) {
    int r;
    if(hold_until) return cmm2->micros() >= hold_until;
    if(shot == NULL) return 0;
    if(done_passes < 0) {
        int timeout = cmm2->micros() - t_start > 60000000u;
        if(wait_until != 0 && cmm2->micros() < wait_until) return 0;
        wait_until = 0;
        if(!timeout && (shot_bw == NULL || browser_window_stop_available(shot_bw))) return 0;
        t_done = cmm2->micros();
        done_passes = 0;
        if(timeout) fputs("browser: not done after 60 s\n", stderr);
    }
    if(++done_passes < 5) return 0;
    if(then_next < nthen && shot_bw != NULL) {  /* (--then: the next step, and its page waited for in turn) */
        stats();
        then_step();
        done_passes = -1;
        t_start = cmm2->micros();
        return 0;
    }
    while(cmm2->dma2d_busy()) {}
    stats();
    r = cmm2->gfx_save(shot);
    fprintf(stderr, "browser: %s %s\n", shot, r == 0 ? "saved" : cmm2->io_error());
    shot = NULL;
    hold_until = cmm2->micros() + 3000000u;
    return 0;
}

/* --cmm2-netlog: the connections on the serial console (28 Nov 2026: after Wikipedia no other site could be reached).
   curl's socket, connect, closesocket, getsockopt and getaddrinfo, as apps/curl's port gives them, are wrapped at the
   link (make.py: -Wl,--wrap), so curl.app's own build is unchanged. */
static int net_log, net_open;

int __real_socket(int domain, int type, int protocol);
int __real_connect(int fd, const struct sockaddr *addr, socklen_t len);
int __real_closesocket(int fd);
int __real_getsockopt(int fd, int level, int name, void *val, socklen_t *len);
int __real_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

int __wrap_socket(int domain, int type, int protocol) {
    int fd = __real_socket(domain, type, protocol), e = errno;
    if(fd >= 0) net_open++;
    if(net_log) fprintf(stderr, "net: socket %d%s%s, %d open\n", fd, fd < 0 ? ": " : "", fd < 0 ? strerror(e) : "",
                        net_open);
    errno = e;
    return fd;
}

int __wrap_connect(int fd, const struct sockaddr *addr, socklen_t len) {
    int r = __real_connect(fd, addr, len), e = errno;
    if(net_log) {
        const struct sockaddr_in *a = (const struct sockaddr_in *)addr;
        const uint8_t *ip = (const uint8_t *)&a->sin_addr.s_addr;
        fprintf(stderr, "net: connect %d to %u.%u.%u.%u:%u: %s\n", fd, ip[0], ip[1], ip[2], ip[3],
                (unsigned)ntohs(a->sin_port), r == 0 ? "made" : strerror(e));
    }
    errno = e;
    return r;
}

int __wrap_closesocket(int fd) {
    int r = __real_closesocket(fd), e = errno;
    if(r == 0) net_open--;
    if(net_log) fprintf(stderr, "net: close %d%s, %d open\n", fd, r == 0 ? "" : " (not a socket)", net_open);
    errno = e;
    return r;
}

int __wrap_getsockopt(int fd, int level, int name, void *val, socklen_t *len) {
    int r = __real_getsockopt(fd, level, name, val, len), e = errno;
    if(net_log && r == 0 && level == SOL_SOCKET && name == SO_ERROR && val && *(int *)val)
        fprintf(stderr, "net: socket %d's error: %s\n", fd, strerror(*(int *)val));
    errno = e;
    return r;
}

int __wrap_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    uint64_t t = cmm2->micros();
    int r = __real_getaddrinfo(node, service, hints, res);
    if(net_log) {
        if(r == 0 && *res && (*res)->ai_addr) {
            const uint8_t *ip = (const uint8_t *)&((const struct sockaddr_in *)(*res)->ai_addr)->sin_addr.s_addr;
            fprintf(stderr, "net: %s is %u.%u.%u.%u (%lu ms)\n", node ? node : "?", ip[0], ip[1], ip[2], ip[3],
                    (unsigned long)((cmm2->micros() - t) / 1000));
        } else fprintf(stderr, "net: %s not found: %s (%lu ms)\n", node ? node : "?", gai_strerror(r),
                       (unsigned long)((cmm2->micros() - t) / 1000));
    }
    return r;
}

/* newlib declares uname, and the system gives it: NetSurf's user agent names the machine */
int uname(struct utsname *u) {
    memset(u, 0, sizeof(*u));
    strcpy(u->sysname, "CMM2");
    strcpy(u->nodename, "cmm2");
    strcpy(u->release, "8");
    strcpy(u->version, "MMBasic");
    strcpy(u->machine, "armv7em");
    return 0;
}

int main(int argc, char **argv) {
    static char w[12], h[12];
    static char *defaults[] = {"-f", "cmm2", "-w", w, "-h", h, "-b", "16"};   /* the screen's own mode (see above) */
    cmm2_target t;
    char **nargv;
    int i, n = 0;
    t_start = cmm2->micros();
    stack_paint();
    if(cmm2->gfx_target(&t) != 0 || t.bpp != 16 || t.pitch != t.width * 2) {
        puts("The browser needs a mode of 16-bit colour: MODE 9,16 or MODE 11,16, say, before RUN");
        return 1;
    }
    {                                           /* the fonts, said here while the screen is still the console's: */
        FILE *f = fopen(FONT_CHECK, "rb");      /* NetSurf's own message would go to the serial port only */
        if(f == NULL) {
            puts("The browser's fonts are missing: the fonts folder from its zip goes in /browser/fonts");
            return 1;
        }
        fclose(f);
    }
    snprintf(w, sizeof(w), "%lu", (unsigned long)t.width);
    snprintf(h, sizeof(h), "%lu", (unsigned long)t.height);
    if((nargv = malloc((argc + 9) * sizeof(char *))) == NULL) return 1;
    nargv[n++] = argv[0];
    for(i = 1; i < argc; i++)                   /* (-v, NetSurf's log on the console, counts only as the first word) */
        if(!strcmp(argv[i], "-v")) nargv[n++] = argv[i];
    for(i = 0; i < 8; i++) nargv[n++] = defaults[i];
    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
        else if(!strcmp(argv[i], "--cmm2-nobig")) {
            extern int cmm2_no_big;             /* (browser_malloc.c: the big blocks' pool left unused, for a test) */
            cmm2_no_big = 1;
        }
        else if(!strcmp(argv[i], "--cmm2-netlog")) net_log = 1;
        else if(!strcmp(argv[i], "--then") && i + 1 < argc) {
            if(nthen < THEN_MAX) then[nthen++] = argv[i + 1];
            i++;
        }
        else if(strcmp(argv[i], "-v")) nargv[n++] = argv[i];
    }
    nargv[n] = NULL;
    if(argc > 1 && !strcmp(argv[1], "--cmm2-alloc")) {     /* (a check: what malloc and the alloc service give) */
        void *b[64];
        int k, nb = 0;
        size_t total = 0;
        char *p;
        struct mallinfo mi;
        for(k = 0; k < 64 && (b[k] = cmm2->alloc(1u << 20)) != NULL; k++) nb++;
        printf("alloc: %d blocks of 1 MB from MMBasic's heap\n", nb);
        for(k = 0; k < nb; k++) cmm2->release(b[k]);
        while((p = malloc(256 * 1024)) != NULL) total += 256 * 1024;
        mi = mallinfo();
        printf("malloc: %u KB in 256 KB pieces; footprint %u KB\n", (unsigned)(total / 1024), (unsigned)(mi.arena / 1024));
        return 0;
    }
    return netsurf_main(n, nargv);
}
