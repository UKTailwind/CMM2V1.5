/* pdcutil.c: the CMM2's beep, wait and name for PDCursesMod (apps/pdcurses/cmm2), in place of the VT port's
   (vt/pdcutil.c, which the Linux framebuffer port uses, and whose napms this follows). Public domain, as PDCursesMod.

   The beep is PLAY TONE's 400 Hz for 0.2 seconds, as the others' (common/beep.c), or a flash if the sound is in use.
   A wait looks after the USB devices (poll) every 10 ms at most, and the cursor's and text's blinking. */
#include "curspriv.h"
#include "cmm2.h"

void PDC_check_for_blinking(void);              /* common/blink.c, in fb/pdcdisp.c */

static const long beep_interval = 700;

static void _raw_beep(void) {
    if(cmm2->version < 11 || cmm2->basic_cmd("PLAY TONE 400, 400, 200") != 0) flash();
}

void PDC_beep(void) {
    if(!SP->n_beeps_queued) {
        _raw_beep();
        SP->t_next_beep = PDC_millisecs() + beep_interval;
    }
    SP->n_beeps_queued++;
}

void PDC_napms(int ms) {
    long t = PDC_millisecs();
    const long end_t = t + ms;
    int remains;

    do {
        remains = (int)(end_t - t);
        if(SP->n_beeps_queued && (int)(t - SP->t_next_beep) > 0) {
            SP->n_beeps_queued--;
            SP->t_next_beep = t + beep_interval;
            if(SP->n_beeps_queued > 0) _raw_beep();
        }
        if(remains > 10) remains = 10;          /* (10 ms bits, the USB devices seen to between) */
        PDC_check_for_blinking();
        if(remains > 0) {
            cmm2->delay_us(1000 * remains);
            cmm2->poll();
            t = PDC_millisecs();
        }
    } while(remains > 0);
}

const char *PDC_sysname(void) { return "Colour Maximite 2"; }

enum PDC_port PDC_port_val = PDC_PORT_LINUX_FB; /* (the port it is built from: an 8-bit framebuffer) */
