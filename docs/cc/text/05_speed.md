# Speed

All of these were measured on a Generation 2 at 480MHz, in MODE 1,8.

## Calculation

A 3,200-line program that predicts solar eclipses, run with the three ways of running a BASIC program:

| How it ran | Time | Compared with the interpreter |
|---|---|---|
| The interpreter | 6.16 seconds | 1 |
| OPTION COMPILE ON | 1.71 seconds | 3.6 times faster |
| CC | 0.93 seconds | 6.6 times faster |

The output of the three was the same, byte for byte, apart from the time the program printed.

The KnivD benchmark moves grains of sand, so a higher figure is better:

| How it ran | Grains |
|---|---|
| The interpreter | 18,768 |
| OPTION COMPILE ON | 42,070 |
| CC, the BASIC program | 69,100 |
| CC, the same program written in C by hand | 86,000 |
| The same C built with gcc on a PC, as a loadable program | 130,291 |

## Games

Games spend much of their time drawing. That takes the same time however the program runs, so the gain from compiling is smaller:

| Game | Interpreter | OPTION COMPILE ON | CC |
|---|---|---|---|
| Wolfenstein 3D (wolf3d_cmm2) | 28.5 frames a second | 79.7 | 90.2 |
| Into the Darkness | 17.8 frames a second | 37.5 | 37.9 |

Into the Darkness spends about 62% of its time in the firmware's drawing commands, so compiling the rest gains almost nothing over OPTION COMPILE ON. It is a good example of when OPTION COMPILE ON is all you need.

## Which to use

OPTION COMPILE ON needs no extra step and works on any program, so try it first. Use CC when the program is mostly calculation, or when you want the last of the speed and have checked that the result is the same.
