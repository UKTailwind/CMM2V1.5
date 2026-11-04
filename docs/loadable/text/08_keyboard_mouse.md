# The Keyboard and Mouse

A program reads the keyboard in two ways: as characters typed, for text, or as the keys held down at the moment, for games. The mouse is read as a position and buttons. A USB keyboard, mouse or game controller is looked after only when the program calls poll(), so a program that reads them calls it each time round its main loop.

## Characters Typed

```
int con_getc(void);
```

con_getc returns the next character typed on the serial console or a USB keyboard, or -1 if there is none (it never waits). Letters, digits and symbols are their ASCII codes, Enter is 13, Backspace 8, Tab 9 and Escape 27. The other keys have MMBasic's codes:

{widths: 30,20,30,20}
| Key | Code | Key | Code |
|---|---|---|---|
| Up arrow | 128 | Insert | 132 |
| Down arrow | 129 | Home | 134 |
| Left arrow | 130 | End | 135 |
| Right arrow | 131 | Page Up | 136 |
| Delete | 127 | Page Down | 137 |
| F1 to F12 | 145 to 156 | Ctrl with a letter | 1 to 26 |

The full list, with the keys combined with Shift, Ctrl and Alt, is in Appendix I of the user manual. getchar() and the other stdio input functions read the same keys, but wait for one and show it.

## Keys Held Down

```
int keys_held(int keys[7]);                              /* version 2 */
```

keys_held fills keys[0] to keys[5] with the keys held down on a USB keyboard at that moment, as MMBasic's KEYDOWN function gives them (0 for none), and keys[6] with the modifier keys held, as bits:

{widths: 15,35,15,35}
| Bit | Key | Bit | Key |
|---|---|---|---|
| 1 | Left Alt | 16 | Right Alt |
| 2 | Left Ctrl | 32 | Right Ctrl |
| 4 | Left GUI (Windows) | 64 | Right GUI |
| 8 | Left Shift | 128 | Right Shift |

It returns the number of keys held. The codes are as for con_getc: each is the character the key types, so Shift with a gives A (65) and Shift with 1 gives ! (33), and Ctrl with a letter gives its control code (Ctrl-D is 4). Up to six keys are reported at once, as the USB keyboard reports them.

A game reads the keys once a frame and compares them with the frame before, to find the keys pressed and released. In outline, Doom's layer works this way:

```
static int last[7];

static void keys(void) {
    int now[7], i, j;
    cmm2->poll();                             /* the keyboard read */
    cmm2->keys_held(now);
    for(i = 0; i < 6; i++) {                  /* pressed: in now, not in last */
        if(now[i] == 0) continue;
        for(j = 0; j < 6 && last[j] != now[i]; j++) {}
        if(j == 6) pressed(now[i]);
    }
    for(i = 0; i < 6; i++) {                  /* released: in last, not in now */
        if(last[i] == 0) continue;
        for(j = 0; j < 6 && now[j] != last[i]; j++) {}
        if(j == 6) released(last[i]);
    }
    memcpy(last, now, sizeof(last));
}
```

The keys held come from a USB keyboard only: the serial console sends characters, not key presses, so a program that should also work from a terminal reads con_getc as well.

## The Keyboard's Own Codes

```
int keys_raw(uint8_t report[8]);                         /* version 9 */
```

keys_raw gives the USB keyboard's last report as the keyboard sent it, before MMBasic's keyboard layout is applied. Each key is named by its place on the keyboard, not by the character it types, as a PC's scan codes name the keys. This is for a program that applies a layout of its own, or needs keys that type no character: the 386 PC emulator passes the keys to the PC this way, and DOS applies its own layout.

{widths: 15,85}
| Byte | Holds |
|---|---|
| 0 | The modifier keys held, as bits in the USB order: 1 left Ctrl, 2 left Shift, 4 left Alt, 8 left GUI, 16 right Ctrl, 32 right Shift, 64 right Alt, 128 right GUI. Note that this order is not keys_held's. |
| 1 | 0. |
| 2 to 7 | The USB HID usage codes of up to six keys held (4 for A, 5 for B, and so on to 0x65), or 0. |

It returns the number of keys held. With no keyboard, the report is all zero.

## The Mouse

```
int  mouse(int m[4]);                                    /* version 4 */
void mouse_set(int x, int y);                            /* version 4 */
```

mouse reads the mouse that MMBasic is using (as MM.MOUSE chooses it: a USB mouse, a PS/2 mouse or an I2C mouse adapter). It returns 1 and fills m with its position in the screen's pixels, x then y, its buttons (1 left, 2 right, 4 middle, added together) and the count of its wheel; or 0, with m all zero, if there is no mouse. The firmware keeps the position within the screen, and video_mode centres it. mouse_set moves it. A program draws its own pointer.

```
int m[4];
if(cmm2->mouse(m) && (m[2] & 1)) clicked(m[0], m[1]);
```

## Game Controllers

There is no service for USB game controllers yet. A program that wants one can read the keyboard instead, which every game can be played with.
