# Loop-back tests

Tests that need wires on the rear connector, so they are not part of tests/cmm2 or tests/picomite. Each says at its
head what it needs, and stops with a message when the wiring is not there.

Never wire pin 40 (GP10): it is the factory reset pin. Held low at power-up (a wire to an output that is low, or to
the ground on pin 39 next to it) it erases the options and the program in flash.

| Test | Wiring | Run with |
|---|---|---|
| portgp.bas | GP0, GP1, GP3-GP7 to GP8, GP9, GP11-GP15: pin 8 to 15, 10 to 24, 22 to 38, 36 to 35, 31 to 18, 7 to 27, 13 to 28 | OPTION PICOMITE (any device) |

    python -I tools/goldens.py COM4 OUTDIR --corpus tests/loopback --dir A:/l --nomodemap --pre "OPTION PICOMITE HDMI"
