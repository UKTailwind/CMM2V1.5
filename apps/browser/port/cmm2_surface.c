/* cmm2_surface.c: libnsfb's surface for the Colour Maximite 2 (apps/browser, docs/Web_Browser_Study.md), by the name
   "cmm2", so the framebuffer front end's -f cmm2 selects it. libnsfb's surface kinds are a fixed list, so it takes
   SDL's (not built here). Not the RAM surface's: the front end makes each image's bitmap a RAM surface
   (frontends/framebuffer/bitmap.c), and with the screen in that slot every image failed to the red placeholder.
   - NetSurf draws into a buffer in the program's memory, RGB565, and update copies each changed rectangle to the page
     shown by the DMA2D: the page is not cached, and drawing there a pixel at a time would be slow.
   - The screen's mode is used as it is (gfx_target: its page, size and depth), never changed (Peter, 27 Nov 2026: a
     mode change, two for each run, is a large event on the SDRAM's bus). It must be of 16-bit colour and the size
     asked for (cmm2_main.c asks for its own). The screen is cleared when NetSurf ends.
   - input pumps the network (services version 21's net_poll: nothing else does while NetSurf waits) and the USB
     devices, for up to its timeout. Ctrl-C asks NetSurf to quit. Keys and the mouse come in step 3.
   - A test run (cmm2_main.c's --shot) is finished here once the page is drawn: cmm2_shot_poll.
   MIT License, as Inc/cmm2img.h. */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libnsfb.h"
#include "libnsfb_plot.h"
#include "libnsfb_event.h"
#include "nsfb.h"
#include "surface.h"
#include "plot.h"
#include "cmm2.h"

int cmm2_shot_poll(void);                       /* cmm2_main.c: 1 once a test run has saved its picture */

static cmm2_video v;
uint8_t *volatile cmm2_page_buffer;            /* (NetSurf's buffer, for reading through the ST-Link beside the page) */

static int cmm2_defaults(nsfb_t *nsfb) {
    nsfb->width = 800;
    nsfb->height = 600;
    nsfb->format = NSFB_FMT_RGB565;
    select_plotters(nsfb);
    return 0;
}

static int cmm2_initialise(nsfb_t *nsfb) {
    cmm2_target t;
    uint8_t *p;
    if(nsfb->format != NSFB_FMT_RGB565 || cmm2->version < 21 || cmm2->gfx_target(&t) != 0) return -1;
    if(t.bpp != 16 || t.width != (uint32_t)nsfb->width || t.height != (uint32_t)nsfb->height || t.pitch != t.width * 2)
        return -1;
    v.fb = t.fb;
    v.width = t.width;
    v.height = t.height;
    v.pitch = t.pitch;
    if((p = realloc(nsfb->ptr, (size_t)nsfb->width * nsfb->height * 2)) == NULL) return -1;
    memset(p, 0xFF, (size_t)nsfb->width * nsfb->height * 2);
    nsfb->ptr = p;
    cmm2_page_buffer = p;
    nsfb->linelen = nsfb->width * 2;
    cmm2->con_screen(0);                        /* (the console to serial only: the page is NetSurf's) */
    return 0;
}

static int cmm2_geometry(nsfb_t *nsfb, int width, int height, enum nsfb_format_e format) {
    if(nsfb->ptr != NULL) return -1;            /* (set before initialise only: the mode is chosen there) */
    if(width > 0) nsfb->width = width;
    if(height > 0) nsfb->height = height;
    if(format != NSFB_FMT_ANY) nsfb->format = format;
    select_plotters(nsfb);
    nsfb->linelen = nsfb->width * nsfb->bpp / 8;
    return 0;
}

static int cmm2_finalise(nsfb_t *nsfb) {
    while(cmm2->dma2d_busy()) {}
    cmm2->gfx_cls(0);                           /* (the mode stays: the page cleared for the prompt) */
    free(nsfb->ptr);
    nsfb->ptr = NULL;
    return 0;
}

/* the rectangle from the buffer to the page, by the DMA2D (three copies may wait; more wait here) */
static int cmm2_update(nsfb_t *nsfb, nsfb_bbox_t *box) {
    int x0 = box->x0 < 0 ? 0 : box->x0, y0 = box->y0 < 0 ? 0 : box->y0;
    int x1 = box->x1 > nsfb->width ? nsfb->width : box->x1, y1 = box->y1 > nsfb->height ? nsfb->height : box->y1;
    if(x1 <= x0 || y1 <= y0) return 0;
    while(cmm2->dma2d_copy(nsfb->ptr + y0 * nsfb->linelen + x0 * 2, nsfb->linelen,
                           v.fb + y0 * v.pitch + x0 * 2, v.pitch, (x1 - x0) * 2, y1 - y0) < 0) {}
    return 0;
}

static bool cmm2_input(nsfb_t *nsfb, nsfb_event_t *event, int timeout) {
    static uint64_t last_usb;
    uint64_t end = cmm2->micros() + (timeout > 0 ? (uint64_t)timeout * 1000u : 0), now;
    int brk = 0;
    (void)nsfb;
    for(;;) {
        cmm2->net_poll();
        now = cmm2->micros();
        if(now - last_usb >= 20000) {           /* (the USB devices every 20 ms, as port/cmm2_net.c's pump: poll is
                                                   MMBasic's whole routinechecks) */
            last_usb = now;
            brk = cmm2->poll();
        }
        if(brk || cmm2_shot_poll()) {
            event->type = NSFB_EVENT_CONTROL;
            event->value.controlcode = NSFB_CONTROL_QUIT;
            return true;
        }
        if(timeout >= 0 && cmm2->micros() >= end) return false;
        cmm2->delay_us(500);
    }
}

static int cmm2_claim(nsfb_t *nsfb, nsfb_bbox_t *box) {
    (void)nsfb;
    (void)box;
    return 0;
}

static int cmm2_cursor(nsfb_t *nsfb, struct nsfb_cursor_s *cursor) {
    (void)nsfb;
    (void)cursor;
    return 0;
}

const nsfb_surface_rtns_t cmm2_rtns = {
    .defaults = cmm2_defaults,
    .initialise = cmm2_initialise,
    .finalise = cmm2_finalise,
    .input = cmm2_input,
    .geometry = cmm2_geometry,
    .update = cmm2_update,
    .claim = cmm2_claim,
    .cursor = cmm2_cursor,
};

NSFB_SURFACE_DEF(cmm2, NSFB_SURFACE_SDL, &cmm2_rtns)
