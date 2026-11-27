/* cmm2_main.c: browser.app's start (apps/browser, docs/Web_Browser_Study.md). NetSurf's framebuffer front end keeps its
   own main (frontends/framebuffer/gui.c, compiled with -Dmain=netsurf_main), called from here with the CMM2's surface
   unless the words say otherwise:
       RUN "browser.app", [--shot FILE] [NetSurf's options] [url]
   - -f cmm2 -w 1280 -h 720 -b 16 are put in front: 720p, MODE 11 (Peter, 27 Nov 2026). A later -w/-h/-b wins.
   - --shot FILE: a test run. When the first page has finished loading (cmm2_page_done, from patches/netsurf-0001) and
     been drawn, the screen is saved as FILE (SAVE PNG's), the times and the memory are printed, and NetSurf quits.
   MIT License, as Inc/cmm2img.h. */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/utsname.h>
#include "cmm2.h"

struct browser_window;
int netsurf_main(int argc, char **argv);
bool browser_window_stop_available(struct browser_window *bw);   /* (include/netsurf/browser_window.h) */

static const char *shot;                        /* --shot's file, or NULL */
static struct browser_window *shot_bw;          /* the window the throbber stopped in */
static uint64_t t_start, t_done;
static int done_passes = -1;                    /* the surface's input calls since the page was done, or -1 */

/* the throbber stopped: NetSurf stops it in several places (the first, empty, page among them), so this only notes
   the window; cmm2_shot_poll waits until nothing in it is loading */
void cmm2_page_done(struct browser_window *bw) {
    shot_bw = bw;
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
    fprintf(stderr, "browser: the page done %.2f s after the start; heap %lu KB at most, %lu KB in use; stack %u KB"
            " of %u\n", (double)(t_done - t_start) / 1e6, (unsigned long)(mi.arena / 1024),
            (unsigned long)(mi.uordblks / 1024), stack_used_kb(),
            (unsigned)(((uintptr_t)stack_hi - (uintptr_t)stack_lo) / 1024));
}

/* from the surface's input, once a pass of the front end's loop: 1 when a test run is finished (NetSurf then quits).
   The page is done when its window has nothing loading; five passes later its redraw has been drawn and copied. A
   page that never finishes is saved as it is after 60 s. */
int cmm2_shot_poll(void) {
    int r;
    if(shot == NULL) return 0;
    if(done_passes < 0) {
        int timeout = cmm2->micros() - t_start > 60000000u;
        if(!timeout && (shot_bw == NULL || browser_window_stop_available(shot_bw))) return 0;
        t_done = cmm2->micros();
        done_passes = 0;
        if(timeout) fputs("browser: not done after 60 s\n", stderr);
    }
    if(++done_passes < 5) return 0;
    while(cmm2->dma2d_busy()) {}
    stats();
    r = cmm2->gfx_save(shot);
    fprintf(stderr, "browser: %s %s\n", shot, r == 0 ? "saved" : cmm2->io_error());
    shot = NULL;
    return 1;
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
    static char *defaults[] = {"-f", "cmm2", "-w", "1280", "-h", "720", "-b", "16"};   /* 720p, MODE 11 (Peter) */
    char **nargv;
    int i, n = 0;
    t_start = cmm2->micros();
    stack_paint();
    if((nargv = malloc((argc + 9) * sizeof(char *))) == NULL) return 1;
    nargv[n++] = argv[0];
    for(i = 1; i < argc; i++)                   /* (-v, NetSurf's log on the console, counts only as the first word) */
        if(!strcmp(argv[i], "-v")) nargv[n++] = argv[i];
    for(i = 0; i < 8; i++) nargv[n++] = defaults[i];
    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
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
