# I/O tests

Programs that drive I/O pins, so they are not in the G2's regression (Peter's card is on its connector): they run on
a board whose pins are free, the G1. Each checks what it set by reading the peripheral's registers back with PEEK, so
no instrument is needed.

- `pwm.bas`: PWM and SERVO (Src/PWM.c) on TIM3 (PWM 1: pins 12, 22, 29) and TIM8 (PWM 2: pins 36, 31), with OPTION
  PICOMITE NONE: frequency, centre or up counting, each output's duty and polarity, an empty output left as it is, an
  output started after the others, OFF and STOP, and the pins taken and given back.
- `pwmpico.bas`: the same with OPTION PICOMITE: the PicoMite's PWM phase and defer, and SERVO's positions.

    python -I tools/goldens.py COM33 OUT --baud 115200 --corpus tests/io --dir A:/i --only pwm
    python -I tools/goldens.py COM33 OUT --baud 115200 --corpus tests/io --dir A:/i --only pwmpico --nomodemap --pre "OPTION PICOMITE HDMI"

and OPTION PICOMITE NONE after the second. The G1's console was COM34 until 23 Oct 2026.
