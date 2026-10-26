# Loop-back tests

Tests that need wires on the rear connector, so they are not part of tests/cmm2 or tests/picomite. Each says at its
head what it needs, and stops with a message when the wiring is not there.

Never wire pin 40 (GP10): it is the factory reset pin. Held low at power-up (a wire to an output that is low, or to
the ground on pin 39 next to it) it erases the options and the program in flash.

The wiring they all use: GP0, GP1, GP3-GP7 to GP8, GP9, GP11-GP15, that is pin 8 to 15, 10 to 24, 22 to 38, 36 to 35,
31 to 18, 7 to 27 and 13 to 28 (GP2, pin 12, and GP10, pin 40, are not used).

| Test | What it checks | Run with |
|---|---|---|
| ioserial.bas | SETPIN ARAW (pin 8 to 15); BITSTREAM's modes and two-pin forms timed by TIMER (22 to 38, 36 to 35); DEVICE SERIALTX read by COM1 (24 to 10); DEVICE SERIALRX from PWM 1B (22 to 38). Not with OPTION PICOMITE: its MODE 1 adds about 3 ms to each stream's time | |
| portgp.bas | PORT and PORT() through the 64-bit GPIO masks: every value, each bit, pin numbers, the errors | |
| stepcount.bas | STEPPER's step pulses counted on COUNT3/COUNT4 (pins 15, 24): lines, the PicoMite's 9600-pulse arcs, DIR, positions, G91 rounding | |
| stepio.bas | STEPPER's pulls and first output levels (read from the GPIO registers), the spindle, an E-STOP (pin 7 to 27) and a limit (pin 13 to 28) tripped part way, G28 with no switch | |
| steptime.bas | the 200 kHz tick: a 5 us step pulse (PULSIN), the low time and the 100 kHz top rate, the CPU it takes | |
| tmc.bas | TMC22xx's bit-banged UART on pin 24, read by COM1 on pin 10: every byte and CRC | |
| stepaudio.bas | a listening test, not checked: a 440 Hz tone alone, with the tick idle, stepping at 100 kHz and planning short blended moves (Peter, 25 Oct: "perfect") | |

    python -I tools/goldens.py COM4 OUTDIR --corpus tests/loopback --dir A:/l --nomodemap
