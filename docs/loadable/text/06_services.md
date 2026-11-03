# The Services

The services are the program's way to the firmware: a table of functions, declared in Inc/cmm2img.h, whose address the firmware passes to the program at its start and crt0.c keeps in the variable cmm2. A program includes cmm2.h and calls them through it:

```
#include "cmm2.h"
...
cmm2->con_puts("Hello\r\n");
```

This chapter describes the general services: the console, the clock, the files and directories, the system and the FM synthesiser. The chapters after it describe the screen, the keyboard and mouse, sound and music, and the I/O pins. Appendix A lists them all in the table's order, with the version that added each.

## Errors

A service never raises an MMBasic error: MMBasic's error handling would leave the program's stack and files in disorder. Each returns a status instead, negative for an error. For the file services it is the FatFs error code made negative:

{widths: 10,40,10,40}
| Code | Meaning | Code | Meaning |
|---|---|---|---|
| -1 | A disk error | -8 | The file is there already |
| -3 | The drive is not ready (no card or USB drive) | -9 | Not an open file or directory |
| -4 | No such file | -11 | No such drive |
| -5 | No such path | -13 | No file system on the drive |
| -6 | The name is not valid | -17 | Not enough memory |
| -7 | Access denied (read only, a directory, full) | -18 | Too many open files |

## The Console

```
void con_putc(int c);
void con_puts(const char *s);
int  con_getc(void);
void con_screen(int on);                       /* version 4 */
```

con_putc and con_puts write to the console, as printf() does but without the C library's buffering and without turning a newline into a carriage return and line feed ("\r\n" is wanted for a new line).

con_getc returns the next key typed, from the serial console or a USB keyboard, or -1 if there is none: it never waits. Ordinary keys are their ASCII codes, and the others MMBasic's codes (the arrows are 128 to 131, the function keys F1 to F12 145 to 156 and so on: see Appendix I of the user manual). A USB keyboard's keys arrive only when poll() is called.

con_screen(0) sends the console's output to the serial port only, so that a program's messages are not drawn over its display (and do not scroll it). con_screen(1), the setting at the start, sends it to both.

## The Clock

```
uint64_t micros(void);
void     delay_us(uint32_t us);
void     datetime(int t[6]);
```

micros returns the microseconds since the Colour Maximite 2 started, from a hardware timer. delay_us waits that many microseconds, without looking after the USB devices. datetime fills t with MMBasic's date and time: year, month, day, hour, minute and second.

## Files

```
int     file_open(const char *name, int mode);
int     file_close(int h);
int32_t file_read(int h, void *buf, uint32_t n);
int32_t file_write(int h, const void *buf, uint32_t n);
int     file_seek(int h, uint32_t pos);
int32_t file_tell(int h);
int32_t file_size(int h);
int     file_sync(int h);
int     file_remove(const char *name);
int     file_rename(const char *from, const char *to);
int     file_stat(const char *name, cmm2_stat *st);    /* version 5 */
```

file_open opens a file and returns its handle, 0 to 7, or an error. The mode is made of these bits, combined with C's | operator:

{widths: 26,74}
| Mode | Meaning |
|---|---|
| CMM2_O_READ | Read. |
| CMM2_O_WRITE | Write: the file must be there, unless CREATE or APPEND is added too. |
| CMM2_O_WRITE + CMM2_O_CREATE | The file is made, or emptied if it is there. |
| CMM2_O_WRITE + CMM2_O_APPEND | The file is made if it is not there, and written at its end. |

READ and WRITE can be used together. A file opened only to read is given a map of its clusters, so that seeking in it does not have to read the card's allocation table again (Doom seeks to each part of its 28MB data file).

file_read and file_write return the number of bytes read or written (fewer than asked at the end of a file, or when the drive is full) or an error. file_seek moves to a position from the start of the file. file_sync writes what is waiting to the card without closing the file. file_rename fails if a file of the new name is there (-8); the C library's rename() removes it first. Names are as described in the chapter The C Library.

file_stat gives a file's or directory's details:

```
typedef struct {
    uint32_t size;                              /* 0 for a directory */
    uint8_t  is_dir;
    uint8_t  month, day, hour, minute, second;  /* its last change */
    uint16_t year;
} cmm2_stat;
```

It returns 0, or an error (-4 if there is no such file).

## Directories

```
int dir_make(const char *name);
int dir_open(const char *path);
int dir_read(int h, cmm2_dirent *e);
int dir_close(int h);
```

dir_make makes a directory. dir_open opens a directory for listing and returns its handle, 0 to 3. dir_read fills in the next entry and returns 1, or 0 when there are no more:

```
typedef struct {
    char     name[256];
    uint32_t size;
    uint8_t  is_dir;
} cmm2_dirent;
```

For example, to list the root of the SD card:

```
cmm2_dirent e;
int d = cmm2->dir_open("A:/");
if(d >= 0) {
    while(cmm2->dir_read(d, &e) == 1)
        printf("%s%s  %lu\n", e.name, e.is_dir ? "/" : "", (unsigned long)e.size);
    cmm2->dir_close(d);
}
```

The files and directories a program leaves open are closed when it ends.

## The System

```
int      poll(void);
void     leave(int code);
uint32_t cpu_hz(void);
void    *alloc(uint32_t size);
void     release(void *p);
int      break_take(void);                      /* version 5 */
```

poll looks after the USB devices: the keyboard, the mouse, game controllers and the USB drive. It returns 1 once Ctrl-C has been pressed, as a request to end. It is quick when there is nothing to do, and a program should call it at least every 10 to 20ms: in its main loop, or once a frame.

leave ends the program from anywhere, with its exit code (see The End in the chapter How a Program Runs).

cpu_hz returns the processor's clock in Hz: normally 480000000 (400000000 with the earlier revision Y of the chip), and 504000000 under OPTION OVERCLOCK ON.

alloc takes a block of MMBasic's heap, outside the program's own memory, and returns it or NULL; release gives it back. Up to 64 blocks can be held, and they are freed when the program ends.

break_take returns 1 if Ctrl-C has been pressed since it was last called, and clears it, so that the program deals with Ctrl-C itself and the BASIC program that ran it carries on afterwards. poll() also reports a Ctrl-C until break_take takes it.

## The FM Synthesiser

```
void *opl_new(uint32_t rate);
void  opl_write(void *chip, uint32_t reg, uint32_t value);
void  opl_generate(void *chip, int16_t *buf, uint32_t frames);
void  opl_free(void *chip);
```

These give a program the firmware's emulation of the Yamaha YMF262 (the OPL3 of the Sound Blaster cards, by ymfm), for a program that drives an OPL chip itself: a DOS game's AdLib music, or an emulator.

opl_new makes a chip, reset, that produces 16-bit stereo sound at rate samples a second (44100 for the sound stream), and returns it, or NULL if there is no room (its state, about 18KB, is a block of MMBasic's heap). opl_write writes a register (0 to 0x1FF: 0x100 and above are the OPL3's second bank) at once. opl_generate makes frames samples of interleaved stereo (left, right) into buf, running the chip. opl_free frees it.

The emulation is exact and takes time: about 5,000 processor cycles a sample with all 18 voices sounding, so 1,024 frames take about 10ms on a Generation 2. A program generates the sound in its own code and hands it to the sound stream (see the chapter Sound and Music). A real chip took about 23 microseconds to act on a register write: a program that writes a key off and a key on to the same voice together should generate at least one sample between them, or the note is not struck again. For MIDI music the music_play service is simpler: it plays a MIDI file on the firmware's own chip, as PLAY MIDIFILE does.
