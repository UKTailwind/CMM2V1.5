# The CMM2 Words

These words give a Forth program what a BASIC program has on the Colour Maximite 2: files, the screen, sound, the keyboard and mouse, the I/O pins, and MMBasic itself. They are in the module `cmm2`, so type this once at the prompt, or put it at the top of a program:

```
cmm2 import
```

This file describes every word this port adds to zeptoforth. The later sections, from Double Precision on, are outside the module `cmm2` and say what to import. For the rest of Forth here, such as `dup`, tasks, `fat32-tools` and `ansi-term`, see zeptoforth's own reference at https://github.com/tabemann/zeptoforth/tree/master/docs/words. It also covers words for its other boards, such as `pin` and `uart`, which are not here: this file's I/O section has their place.

## How They Work

**Arguments** come in MMBasic's order, left to right, so `LINE x1,y1,x2,y2,width,colour` is `x1 y1 x2 y2 width colour line`.

**Strings** are the usual Forth pair ( addr bytes ), as `s"` gives them. MMBasic's limit applies: 255 bytes.

**Decimals.** Only `circle` takes one, its aspect, as a double-precision value: two cells, written with `de` and an exponent (`1de0`, `0.5de0`). The double-precision words are zeptoforth's float32 words with `v` as `dv`: `dv+`, `dvsin`, `dv.`, `n>dv` and so on. A program written with the names `v+` and literals like `1.5e0` can `include float64_v.fs`, then `float64-v import`.

**Colours** are MMBasic's, as RGB() gives them, in every mode and depth. `255 128 0 rgb` makes one and `red`, `orange` and the rest are ready made (see Colours). A fill or background of `-1` is none.

**Errors** are exceptions: a word that fails does not return. At the prompt the message is printed; in a program `try` catches it, and the exception can be compared with the names below:

```
: load-it ( -- ) s" A:/level1.dat" o-read file-open file-close ;
: there? ( -- flag ) ['] load-it try dup ['] x-no-file = if drop false else ?raise true then ;
```

`there?` is false if the file is not there, true if it was read, and passes on any other error with `?raise`.

Things that are not really errors are answers: `exists?`, `mouse`'s flag, `gamepad`'s -1, `i2c-xfer`'s status.

**Text that comes back** (`cwd$`, `basic-eval`, `date$`, `time$`, `dir-read`'s name) is in a buffer of the word's own, and stays there until that word next runs. `dir-read` has one for each open directory, and `date$`, `time$` and `basic-eval` share theirs. A program with several tasks calling the same word should copy the text at once.

**The console** is a terminal on the screen, apps/vt100's, as well as the serial port, so ANSI sequences work: colours, `ansi-term`, the line editor (`enable-line`) and `zeptoed`. It draws on whatever the mode is, with its colours and without touching the palette, and what has been printed reaches the screen before a word here runs, so text and graphics come out in the order the program made them. Keys come as a terminal sends them: an arrow is ESC [ A, not the firmware's code for it, which `keydown` still gives for what is held. Questions like `get-cursor-position` are answered by the terminal, not sent to the serial port. In a mode of 1 or 4 bits the firmware draws the console, without colours, and the terminal takes it back when the mode changes. While `0 console-screen` is in force what is printed is not drawn, then or later. Shift with Page Up and Page Down, or the mouse wheel, shows what has scrolled off.

**Files for zeptoforth's own words.** `fat32-tools`, `zeptoed` and anything else written for zeptoforth's file system use the firmware's files, through the classes in `cmm2_fs.fs`, so MMBasic's names work there too. A listing's names are cut to 8.3 by the entry they come in: `files` gives them whole. `blocks.fs` keeps `block`, `load`, `list`, `edit` and the rest in blocks.dat in the current directory.

**Tasks.** The firmware's services cannot be run twice at once, so while one of these words is in the firmware no other task runs. Most take microseconds. A long one (a big `file-read`, `save-screen`, `delay-us`) holds the others up for as long as it takes. Any of these words can be used from any task.

**At `reboot` or Ctrl-C** Forth starts again, and what it had of the firmware is given back, as it is when a program ends: open files and directories are closed, sound stops, blocks are released, buses and COM ports are closed, PWM stops, pins are unconfigured, and fonts defined from memory go (the console goes back to font 1 if any were). The video mode stays as it was, as the console is using it. `bye` leaves for MMBasic, which puts back the mode, the font and the colours as well.

**A bad address** raises the exception `x-bad-address`, which the prompt reports like any other, keeping what is in RAM. In a firmware service, or if a second comes straight after the first, Forth starts again, as at Ctrl-C.

**Name clashes.** The colours `black`, `blue`, `cyan`, `green`, `magenta`, `red`, `white` and `yellow` are also in the module `ansi-term`: whichever is imported last wins.

## Files

Names are MMBasic's: `A:/dir/name`, `C:...`, or relative to the current directory. Up to 8 files can be open at once, and an `include` in progress takes one.

| Word | Stack | What it does |
|---|---|---|
| `o-read` `o-write` `o-create` `o-append` | ( -- mode ) | file-open's modes, combined with `or` |
| `file-open` | ( addr bytes mode -- handle ) | Opens a file. `o-read`: to read. `o-write`: the file must be there. `o-write o-create or`: made, or emptied if it is there. `o-write o-append or`: made if need be, and written at its end. `o-read o-write or`: to read and change in place |
| `file-close` | ( handle -- ) | CLOSE |
| `file-read` | ( addr bytes handle -- count ) | Reads up to bytes into addr, and gives how many: fewer at the end of the file, 0 past it |
| `file-write` | ( addr bytes handle -- count ) | Writes bytes from addr, and gives how many: fewer if the drive is full |
| `file-seek` | ( pos handle -- ) | SEEK: pos is from the start, and 0 is the first byte (SEEK's first is 1) |
| `file-tell` | ( handle -- pos ) | LOC: the position, 0 at the start |
| `file-size` | ( handle -- size ) | LOF |
| `file-sync` | ( handle -- ) | Writes what is waiting to the card without closing the file |
| `file-truncate` | ( handle -- handle' ) | Ends the file where the handle is. The file is copied to do it, so use the handle that comes back |
| `file-remove` | ( addr bytes -- ) | KILL |
| `file-rename` | ( from-addr from-bytes to-addr to-bytes -- ) | RENAME: fails with `x-exists` if the new name is there |
| `file-stat` | ( addr bytes -- size dir? year month day hour minute second ) | A file's or directory's details: dir? is true for a directory, whose size is 0. A drive's root has no date, so its date is all 0 |
| `exists?` | ( addr bytes -- flag ) | Is a file or directory there? |
| `file-load` | ( addr bytes -- block size ) | A whole file in a new block of memory (see `alloc`): `release` gives it back |

For example, to write a file and read it back:

```
create buf 16 allot
: save-it ( -- ) s" A:/hello.txt" o-write o-create or file-open >r s" Hello" r@ file-write drop r> file-close ;
: show-it ( -- ) s" A:/hello.txt" o-read file-open >r buf 16 r@ file-read r> file-close buf swap type ;
```

## Directories

| Word | Stack | What it does |
|---|---|---|
| `mkdir` | ( addr bytes -- ) | MKDIR |
| `rmdir` | ( addr bytes -- ) | RMDIR: the directory must be empty |
| `chdir` | ( addr bytes -- ) | CHDIR: which stays when the program ends, as CHDIR's does |
| `cwd$` | ( -- addr bytes ) | CWD$ |
| `files` | ( -- ) | Lists the current directory: each file with its size, and directories with a / |
| `dir-open` | ( addr bytes -- handle ) | Opens a directory for listing: up to 4 at once |
| `dir-read` | ( handle -- addr bytes size dir? true \| false ) | The next entry, or false after the last |
| `dir-close` | ( handle -- ) | Closes it |

```
: list-it ( -- )
  s" A:/" dir-open >r
  begin r@ dir-read while
    if drop type ." /" else -rot type space . then cr
  repeat
  r> dir-close
;
```

## The Screen

| Word | Stack | What it does |
|---|---|---|
| `mode` | ( mode depth -- ) | MODE: depth 8 or 16 (not 16 in modes 15 and 16). The page is cleared. The monitor takes a second or two to find the new picture |
| `mode-scaled` | ( mode depth scale -- ) | As `mode`, each pixel shown scale times each way (1 or 2), the display 1/scale of the mode's size. Refused in the modes that double their lines already (3, 5, 6, 7, 12, 13, 17) |
| `palette` | ( start count addr -- ) | Sets count entries of an 8-bit mode's palette from start, from R, G, B bytes at addr. It waits for the next frame, and the change is seen whole |
| `wait-frame` | ( -- ) | Waits for the display's next frame, the moment to draw without tearing, with other tasks running meanwhile. Not more than 50 ms |
| `gfx-target` | ( -- fb width height pitch bpp copies ) | The page drawing goes to, for a program that writes pixels itself: line y starts at fb + y * pitch, and where copies is 2 the mode shows each line twice and each must be written twice, the second width bytes (in depth 8) after the first. `PAGE WRITE` and `FRAMEBUFFER WRITE`, through `basic`, change it |
| `mm.hres` `mm.vres` | ( -- pixels ) | The size of that page |
| `console-screen` | ( flag -- ) | Whether the console is drawn on the screen as well as sent to the serial port. **A graphics program wants `0 console-screen`:** anything printed lands on the picture, and at the bottom of the screen scrolls it |
| `console-font` | ( index -- ) | The console's font: 0 to 5 are 5x8, 6x12, 8x14, 8x16, 12x24 and 16x32, and -1 the one that suits the mode |
| `console-size` | ( cols rows -- ) | The console's size in characters, 0 for all there is: a terminal on the serial port may want the same, as what Forth sees of the size is the screen's |

## Colours

| Word | Stack | What it does |
|---|---|---|
| `rgb` | ( r g b -- colour ) | RGB(r, g, b) |
| `black` `blue` `green` `cyan` `red` `magenta` `yellow` `brown` `gray` `lightgray` `white` `orange` `pink` `gold` `salmon` `beige` `lilac` `fuchsia` `rust` `cerulean` `midgreen` `cobalt` `myrtle` | ( -- colour ) | RGB(WHITE) and the rest |
| `colour` | ( colour bg -- ) | COLOUR: the console's colours, and the drawing's when none is given |

## Drawing

| Word | Stack | What it does |
|---|---|---|
| `cls` | ( colour -- ) | CLS |
| `pixel` | ( x y colour -- ) | PIXEL |
| `pixel@` | ( x y -- colour ) | PIXEL(x, y): 0 to $FFFFFF. In an 8-bit mode a colour reads back as the mode holds it, so red ($FF0000) reads $E00000 |
| `line` | ( x1 y1 x2 y2 width colour -- ) | LINE |
| `box` | ( x y w h width colour fill -- ) | BOX: w and h are its size |
| `rbox` | ( x y w h radius colour fill -- ) | RBOX |
| `circle` | ( x y r width aspect colour fill -- ) | CIRCLE: aspect is a double, `1de0` round, `2de0` twice as wide |
| `triangle` | ( x1 y1 x2 y2 x3 y3 colour fill -- ) | TRIANGLE |
| `text` | ( x y addr bytes just-addr just-bytes font scale colour bg -- ) | TEXT: just as TEXT's (`s" CM"` centred and middle, `s" "` the default), font 1-16 and scale 1-15, or 0 for the console's |
| `blit-read` | ( x y w h addr -- ) | BLIT READ: the area into memory, 3 bytes a pixel: blue, green, red. addr has w * h * 3 bytes |
| `blit-write` | ( x y w h addr -- ) | BLIT WRITE: and back |
| `save-screen` | ( addr bytes -- ) | SAVE PNG, or SAVE IMAGE if the name ends in .BMP |
| `image-resize` | ( x y w h new-x new-y new-w new-h page keep-black -- ) | IMAGE RESIZE_FAST: page is the page read (-1 the FRAMEBUFFER, -2 as it is), keep-black 1 to leave black alone (-2 as 0) |
| `dma2d-copy` | ( src src-pitch dst dst-pitch width height -- ) | Has the DMA2D copy width bytes from each of height rows, pitch bytes apart, while the program goes on. The addresses, pitches and width must be even. It waits if the queue is full. The source must not change until the copy is done |
| `dma2d-busy` | ( -- count ) | The copies not yet done |

```
: picture ( -- )
  0 console-screen
  1 8 mode
  black cls
  40 40 200 120 2 red blue box
  260 40 200 120 16 yellow midgreen rbox
  600 100 60 3 1de0 cyan orange circle
  300 260 s" zeptoforth" s" CM" 1 3 white black text
  s" A:/picture.png" save-screen
;
```

## Fonts

| Word | Stack | What it does |
|---|---|---|
| `font` | ( font scale -- ) | FONT: the console's font (font 0 leaves it as it is) |
| `mm.fontwidth` `mm.fontheight` | ( -- pixels ) | The console font's character size |
| `font-size` | ( font -- width height ) | Font 1-16's character size |
| `font-define` | ( font addr -- ) | Font 10-16 from memory, in DefineFont's layout: width, height, first character and count as bytes, then the glyphs. It must stay in memory. 0 for addr puts back the font it replaced |

## Sound

The sound stream is 16-bit stereo at about 44,100 samples a second. The firmware asks for it from an interrupt, where Forth cannot run, so a Forth program writes frames (a left and a right sample, four bytes) into a queue of 186 ms, and silence goes out when the queue is empty.

| Word | Stack | What it does |
|---|---|---|
| `audio-start` | ( -- rate ) | Starts the stream, and gives its rate. Again, while it runs, it only gives the rate |
| `audio-stop` | ( -- ) | Stops it, and any music |
| `audio-write` | ( addr frames -- taken ) | As many frames as the queue has room for, and how many that was |
| `audio-room` | ( -- frames ) | The room in the queue |
| `audio-play` | ( addr frames -- ) | All the frames, waiting for room, with other tasks running meanwhile |
| `tone` | ( left-hz right-hz ms -- ) | PLAY TONE |
| `sound-state` | ( -- state ) | What is playing: 0 nothing, 2 a tone, 23 the stream, and the others in Audio.h, with $100 added once the last PLAY EFFECT has ended |

The stream and MMBasic's own sounds exclude each other: `audio-start` under a `tone` raises `x-sound-busy`, and `tone` over the stream raises `x-firmware`. The rest of PLAY is there through `basic`:

```
: song ( -- ) s\" PLAY WAV \"A:/music/song.wav\"" basic ;
s" PLAY STOP" basic
```

## Music

| Word | Stack | What it does |
|---|---|---|
| `music-play` | ( addr bytes loop? bank -- ) | Plays a Standard MIDI File in memory on the firmware's FM synthesiser, starting the stream if it is not going. The file must stay in memory while it plays. bank is a GENMIDI bank (175 instruments of 36 bytes), or 0 for the firmware's |
| `music-stop` | ( -- ) | Stops it |
| `music-volume` | ( volume -- ) | 0 to 127, 127 at the start |
| `music-pause` | ( paused? -- ) | Pauses it, or goes on |
| `music-playing?` | ( -- flag ) | False once a file not looped has ended: which is two seconds after its last note, for the notes to die away |
| `music-rate` | ( divisor -- ) | The synthesiser at 1/1 to 1/4 of its rate: less of the processor and less treble, the pitch and timing kept |

```
s" A:/music/tune.mid" file-load 0 0 music-play
```

## The FM Chip Directly

For a program that drives an OPL3 itself. It makes the sound into memory, in its own time, and hands that to the stream: about 10 ms of the processor for each 23 ms of sound with all voices going.

| Word | Stack | What it does |
|---|---|---|
| `opl-new` | ( rate -- chip ) | A chip, reset, making rate samples a second. Its 18 KB comes from `alloc`'s blocks |
| `opl-write` | ( chip reg value -- ) | Writes register 0 to $1FF, at once |
| `opl-generate` | ( chip addr frames -- ) | Makes frames of stereo at addr, which `audio-play` takes as they are |
| `opl-free` | ( chip -- ) | Gives it back |

## The Keyboard, the Mouse and Gamepads

The keys typed come through Forth's own `key` and `key?`. These are for keys held down, as a game wants, which only a USB keyboard can report. Forth looks after the USB devices at each frame, so they need no calling.

| Word | Stack | What it does |
|---|---|---|
| `keydown` | ( n -- value ) | KEYDOWN(n): 0 how many keys are held, 1-6 each key's code, 7 the modifiers (1 left Alt, 2 left Ctrl, 4 left GUI, 8 left Shift, 16-128 the right ones) |
| `keys-raw` | ( addr -- count ) | The USB keyboard's last report at addr, 8 bytes: the modifiers in HID's order, 0, and six HID usage codes, before any layout. Gives how many keys are held |
| `mouse` | ( -- x y buttons wheel flag ) | The mouse MM.MOUSE chooses: its position in pixels, its buttons (1 left, 2 right, 4 middle) and its wheel's count. flag is false, and the rest 0, if there is no mouse |
| `mouse-set` | ( x y -- ) | Moves it, within the screen |
| `gamepad` | ( n -- lx ly buttons ) | The pad in USB slot n, 1 upward: its left stick, 0-255 with 128 centred, and DEVICE(GAMEPAD n, B)'s bitmap. buttons is -1 if there is no pad |

## I/O

Pins are numbered as the PicoMite's GP numbers, as in the loadable programs guide's table: COM1 on GP0/1, PWM on GP2-5, SPI2 on GP10-12, I2C2 on GP14/15, I2C on GP16/17, SPI on GP18-20, the analog inputs on GP26-28, COM2 on GP36/37. Pin 40 (GP10, SPI2's clock) is the reset link: nothing that holds it low at power up.

| Word | Stack | What it does |
|---|---|---|
| `pin-off` `pin-in` `pin-out` `pin-od` `pin-analog` | ( -- mode ) | setpin's modes: not configured, input, output, open drain output, analog input |
| `pull-none` `pull-up` `pull-down` | ( -- pull ) | setpin's pulls, for an input |
| `setpin` | ( gp mode pull -- ) | SETPIN |
| `pin@` | ( gp -- level ) | PIN(gp): 0 or 1, the level on the pin whatever is driving it |
| `pin!` | ( level gp -- ) | PIN(gp) = level, for an output. On a pin that is not an output it does nothing, and says nothing |
| `adc` | ( gp -- value microvolts ) | An analog input: 0-65535 for 0 V to the supply, and the voltage |
| `pwm` | ( gp freq duty -- ) | GP2-5: freq Hz, duty 0-65535. GP2 and GP3 share a frequency, and GP4 and GP5: setting one's changes the other's, which keeps its duty |
| `pwm-stop` | ( gp -- ) | Stops that output |
| `i2c-open` | ( bus khz timeout -- ) | Bus 0 is I2C (GP16/17), 1 is I2C2 (GP14/15). 100, 400 or 1000 kHz, timeout 0 (none) or 100 ms up |
| `i2c-close` | ( bus -- ) | |
| `i2c-xfer` | ( bus addr w wn r rn stop? -- status ) | Writes wn bytes from w to 7-bit address addr, then reads rn into r, with a repeated start between. stop? releases the bus at the end. With wn and rn 0 it only sends the address, to see if a device answers. Gives 0, 1 for no acknowledge or 2 for a timeout, as MM.I2C |
| `spi-open` | ( bus hz mode -- ) | Bus 0 is SPI (GP18-20), 1 is SPI2 (GP10-12). Mode 0-3, 8 bits |
| `spi-close` | ( bus -- ) | |
| `spi-xfer` | ( bus tx rx n -- ) | n bytes each way. tx 0 sends zeros, rx 0 drops what comes in. A chip select is an ordinary output |
| `com-open` | ( com baud bits parity stop rxbuf -- ) | OPEN "COMn:": com 1 (GP0/1), 2 (GP36/37), or 4-7 for a USB serial device on a firmware with version 14 of the services, bits 7 or 8, parity 0 none, 1 even, 2 odd, stop 1 or 2, rxbuf the receive buffer's bytes, or 0 for MMBasic's default |
| `com-close` | ( com -- ) | |
| `com-write` | ( addr bytes com -- ) | Queues them to send, waiting while the queue is full |
| `com-read` | ( addr bytes com -- count ) | What has come, up to bytes: it does not wait |
| `com-avail` | ( com -- count ) | LOC(#n): the bytes waiting |
| `io-error` | ( -- addr bytes ) | The firmware's message for its last error |

## The System

| Word | Stack | What it does |
|---|---|---|
| `date$` `time$` | ( -- addr bytes ) | DATE$ and TIME$ |
| `datetime` | ( -- year month day hour minute second ) | The same as numbers |
| `micros` | ( -- d ) | Microseconds since the board started, as a double cell |
| `delay-us` | ( us -- ) | Waits, with nothing else running: for short waits. `ms` lets other tasks run |
| `cpu-speed` | ( -- hz ) | MM.INFO(CPUSPEED) |
| `alloc` | ( bytes -- addr ) | A block of MMBasic's heap, outside Forth's memory. Up to 64 can be held, counting OPL chips |
| `release` | ( addr -- ) | Gives one back |
| `basic` | ( addr bytes -- ) | Runs one MMBasic statement, as the interpreter runs it. Constants only, as Forth's variables are not BASIC's, and it cannot call a SUB. It runs to its end with the other tasks waiting |
| `basic-eval` | ( addr bytes -- addr' bytes' ) | An MMBasic expression's value as text, for what only the firmware knows: `s" MM.INFO(SOUND)" basic-eval type` |
| `basic-prep` | ( addr bytes tokens size -- length ) | Tokenises a statement into size bytes at tokens, once: it takes far longer than most statements take to run. The character 1 in the statement marks a value to come |
| `basic-exec` | ( tokens -- ) | Runs what `basic-prep` made. A value to come is the program's to write in over the 1, as digits or a quoted string, in a copy of the tokens: a minus sign is a token, so a negative number needs `basic` |
| `bye-code` | ( code -- ) | As `bye`, with an exit code, which MMBasic prints if it is not 0 |

A statement is text, so a number in one is written in first. `format-integer` ( addr n -- addr bytes ) makes the digits:

```
create stmt 256 allot
: page-write ( n -- )
  s" PAGE WRITE " { addr u } addr stmt u move
  stmt u + swap format-integer nip u + stmt swap basic
;
```

A double quote in a string needs `s\"`:

```
: song ( -- ) s\" PLAY WAV \"A:/music/song.wav\"" basic ;
```

## Double Precision

These take the place of zeptoforth's single-precision `float32`, with its names, `v` being `dv`. A value is two cells, ( lo hi ) with the high word on top, so `2dup`, `2swap`, `2@`, `2!`, `2constant` and `D:` locals all carry it. In the stacks below, f is a value and takes two cells.

A number has `de` before its exponent, in either case: `1de0`, `1.5de0`, `-2.25de-3`. In `hex`, such a number is an integer, as it would be in any Forth. Angles are in radians. A comparison with a NaN is false, except `dv<>`, which is true.

The words marked * need `float64 import`. The rest are always there.

| Word | Stack | What it does |
|---|---|---|
| `dv+` `dv-` `dv*` `dv/` | ( f1 f2 -- f ) | Add, subtract, multiply and divide |
| `dvnegate` `dvabs` `dvsqrt` | ( f -- f' ) | Negate, absolute value and square root |
| `dvmin` `dvmax` | ( f1 f2 -- f ) | The smaller or larger |
| `dvmod` | ( f1 f2 -- f ) | f1 less the nearest whole multiple of f2: `7de0 4de0 dvmod` is -1 |
| `dvfract` | ( f -- f' ) | The fractional part |
| `dvnfract` | ( f -- f' ) | The whole part, towards zero |
| `dvfloor` `dvceil` | ( f -- f' ) | Down or up to a whole number |
| `dv<` `dv<=` `dv=` `dv<>` `dv>` `dv>=` | ( f1 f2 -- flag ) | Compare two values |
| `dv0<` `dv0<=` `dv0=` `dv0<>` `dv0>` `dv0>=` * | ( f -- flag ) | Compare a value with zero |
| `dv>nan?` `dv>infinite?` * | ( f -- flag ) | Is it not a number, or infinite? |
| `dvsin` `dvcos` `dvtan` | ( f -- f' ) | Sine, cosine and tangent |
| `dvasin` `dvacos` `dvatan` | ( f -- f' ) | Their inverses |
| `dvatan2` | ( y x -- f ) | The angle of the point x,y |
| `dvsinh` `dvcosh` `dvtanh` `dvasinh` `dvacosh` `dvatanh` | ( f -- f' ) | The hyperbolic functions and their inverses |
| `dvexp` `dvln` | ( f -- f' ) | e to the power f, and the natural logarithm |
| `dvexpm1` `dvlnp1` | ( f -- f' ) | e to the power f, less 1, and the natural logarithm of 1 + f: exact for a small f |
| `dvpow` | ( f1 f2 -- f ) | f1 to the power f2, as C's pow |
| `dv**` * | ( f1 f2 -- f ) | As `dvpow`, raising `x-domain-error` for 0 to the power 0, and for a negative number to a fractional power |
| `dvi**` * | ( f n -- f' ) | f to the power of an integer |
| `dvround-half-even` | ( f -- f' ) | To the nearest whole number, a half to the even one: 2.5 is 2, 3.5 is 4 |
| `dvround-half-away-zero` | ( f -- f' ) | A half away from zero: 2.5 is 3, -2.5 is -3 |
| `dvround-half-up` `dvround-half-down` * | ( f -- f' ) | A half up or down: 2.5 is 3 or 2, and -2.5 is -2 or -3 |
| `dvround-half-zero` * | ( f -- f' ) | A half towards zero: 2.5 is 2 |
| `dvround-half-odd` * | ( f -- f' ) | A half to the odd one: 2.5 is 3, 3.5 is 3 |
| `dvround-zero` `dvround-away-zero` * | ( f -- f' ) | Towards zero, or away from it: 2.1 is 2 or 3 |
| `n>dv` `u>dv` | ( n -- f ) | From a signed or unsigned integer |
| `dv>n` `dv>u` | ( f -- n ) | To a signed or unsigned integer, towards zero |
| `f64>dv` `dv>f64` | ( lo hi -- f ) ( f -- lo hi ) | From and to zeptoforth's S31.32 fixed point, numbers like `1,5` |
| `v>dv` `dv>v` | ( v -- f ) ( f -- v ) | From and to a single-precision value in one cell, as float32 has it |
| `dv.` * | ( f -- ) | Prints a value, to 16 significant digits, then a space |
| `(dv.)` * | ( f -- ) | Prints it with no space |
| `format-dv` | ( f addr -- count ) | The value as text at addr, which has 40 bytes |
| `parse-dv` | ( addr bytes -- f flag ) | Text as a value, 1.5de0 or 1.5e0: flag is true if all of it was a number |
| `dv1` `dv-1` `dv2` `dvhalf` `dv-half` `dvpi` * | ( -- f ) | 1, -1, 2, 0.5, -0.5 and pi |
| `dv+infinity` `dv-infinity` * | ( -- f ) | The two infinities |
| `x-domain-error` * | ( -- ) | What `dv**` raises |

`float64_v.fs` gives a program the names without the d: `include float64_v.fs`, then `float64-v import`, and `v+`, `vsin`, `v.`, `1.5e0` and the rest work as above.

## Blocks

Forth's blocks, in the module `block`: type `block import` for these, though `load`, `list`, `load-range`, `list-range` and `edit` are always there. They are records in blocks.dat in the current directory, up to 4096 of 1024 bytes each, and the file is opened when they are first wanted. An id is any number but -1. `edit` is zeptoforth's block editor: Ctrl-V leaves it.

| Word | Stack | What it does |
|---|---|---|
| `block-size` | ( -- bytes ) | 1024 |
| `block?` | ( id -- flag ) | Is there a block of that id? |
| `find-block` | ( id -- addr \| 0 ) | A block's contents, or 0 if there is none. They stay good for the next three finds: copy them to keep them. Writing to them changes nothing on the card |
| `block!` | ( addr id -- flag ) | Writes 1024 bytes from addr as that block. flag is false if there are 4096 blocks already |
| `delete-block` | ( id -- ) | Deletes a block, if it is there |
| `delete-blocks` | ( id count -- ) | Deletes count blocks from id up |
| `erase-all-blocks` | ( -- ) | Deletes them all, and blocks.dat |
| `free-block-count@` | ( -- count ) | How many more can be written |
| `copy-block` | ( src-id dest-id -- ) | Copies a block. If there is no src-id, dest-id is deleted |
| `copy-blocks` | ( src-id dest-id count -- ) | Copies count blocks, which may overlap |
| `insert-blocks` | ( id count -- ) | Moves blocks from id up by count, as far as the first gap that takes them, leaving count empty ids |
| `load` | ( id -- ) | Runs a block as Forth, a line of 64 characters at a time |
| `list` | ( id -- ) | Shows a block, with line numbers |
| `load-range` `list-range` | ( start-id end-id -- ) | The same for a range, leaving out ids that are not there |
| `edit` | ( id -- ) | Opens a block in the block editor |
| `x-block-not-found` | ( -- ) | Raised by `load` and `list` |
| `x-invalid-block-id` | ( -- ) | Raised by `block!` for an id of -1 |
| `x-block-write-fail` | ( -- ) | Raised by `copy-block` if the block cannot be written |

## Loading and Keeping Code

These are always there.

| Word | Stack | What it does |
|---|---|---|
| `include` | ( "name" -- ) | Runs the file named next as Forth. A file may include another, up to four deep |
| `included` | ( addr bytes -- ) | The same for a name on the stack |
| `read-lines` | ( handle -- ) | What `included` runs a file's lines with |
| `x-cannot-open` | ( -- ) | Raised for a file that cannot be opened |
| `save-image` | ( -- ) | Saves the flash dictionary to zeptoforth.img, or zeptoforth_g1.img on a Generation 1, in the current directory, for the next start. `compile-to-flash` puts words there, and `compile-to-ram` goes back |
| `bye` | ( -- ) | Back to MMBasic |
| `sync-code` | ( -- ) | Makes code just written fit to run, as the memory is cached. The compiler does this itself: it is for code a program writes by hand |

## The Date and Random Numbers

| Word | Stack | What it does |
|---|---|---|
| `date-time@` | ( date-time -- ) | In the module `rtc`: the date and time now, from the Colour Maximite 2's clock, into a structure of `date-time-size` bytes. `date-time.` prints it. `rtc` is otherwise zeptoforth's own |
| `random` | ( -- u ) | In the module `rng`: a random number from the hardware |

## Exceptions

| Exception | Raised for |
|---|---|
| `x-bad-address` | A bad address, in Forth's own code (this one is in the kernel, not the module `cmm2`) |
| `x-no-file` | No such file |
| `x-no-path` | No such path |
| `x-bad-name` | The name is not valid, or longer than 255 bytes |
| `x-denied` | Access denied: read only, a directory, full, or written to when open to read |
| `x-exists` | The file is there already |
| `x-not-open` | Not an open file or directory |
| `x-not-ready` | The drive is not ready: no card or USB drive |
| `x-no-drive` | No such drive |
| `x-no-filesystem` | No file system on the drive |
| `x-disk-error` | A disk error |
| `x-too-many-files` | 8 files or 4 directories are open already |
| `x-no-memory` | The memory or the 64 blocks have run out |
| `x-file-error` | Any other of FatFs's errors: the variable `file-error` holds the code |
| `x-no-such-io` | No such GP number, bus or port |
| `x-pin-in-use` | The pin is in use by a COM port, SPI, I2C or another function |
| `x-not-possible` | That mode on that pin, or a bus or port that is not open |
| `x-firmware` | What MMBasic's own command would have called an error: it prints the message, and `io-error` gives it |
| `x-too-long` | A statement or string of more than 255 bytes |
| `x-bad-mode` | That mode or depth cannot be set |
| `x-frame-buffer` | MMBasic has a FRAMEBUFFER of another depth |
| `x-no-display` | There is no page to draw on |
| `x-bad-palette` | Not depth 8, or the range is not in 0-255 |
| `x-bad-copy` | A copy's size, pitch or address is odd or too large |
| `x-no-font` | No such font, or not one that can be defined |
| `x-sound-busy` | Sound is playing already |
| `x-no-room` | No memory for the sound's buffers or the synthesiser |
| `x-no-sound` | The stream would not start |
| `x-not-midi` | Not a MIDI file |
| `x-midi-timing` | The MIDI file's timing, in SMPTE frames, is not supported |

## The Services Left Out

| Service | Why |
|---|---|
| `con_putc`, `con_puts`, `con_getc` | `emit`, `type`, `key` and the rest use them already, through the terminal |
| `poll`, `break_take` | Forth's tick calls them at each frame, and a second caller could swallow a Ctrl-C |
| `leave` | `bye` and `bye-code` |
| `frame_hook` | It is Forth's tick, and runs in an interrupt. `wait-frame` is the way to the frames |
| `audio_lock`, `audio_unlock` | They hold off a fill the program writes. Forth's sound goes through a queue that needs no lock |
| `mem_copy`, `mem_fill`, `mem_move`, `libm` | Forth has `move`, `fill` and its own maths |
