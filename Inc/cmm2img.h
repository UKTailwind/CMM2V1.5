/*
 * cmm2img.h: loadable programs for the Colour Maximite 2 G2. The image header, the start block a program is called
 * with, and the table of firmware services it uses. The firmware (Src/Loader.c, Src/OPL.c) and the SDK (tools/sdk)
 * are both built from this file.
 *
 * MIT License. Copyright (c) 2026 Peter Mather.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this file, to deal in it without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of it, and to permit persons to whom it is furnished to do so, subject to the following condition:
 * the above copyright notice and this permission notice shall be included in all copies or substantial portions.
 * THE FILE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE FILE OR THE USE OR OTHER DEALINGS IN IT.
 *
 * The file, as RUN "name.app" reads it:
 *   the header (64 bytes, below);
 *   image_size bytes: the program as linked, from link_base: its code, constants, init/fini arrays and data;
 *   reloc_count words: what to adjust when the image is not loaded at link_base.
 * A relocation word is the offset, from the image's start, of the word or instruction to adjust, with its kind in the
 * top 4 bits. The loader only moves an image by a multiple of 64 KB, so a MOVW never changes and a MOVT's immediate
 * takes the top half of the difference. tools/sdk/mkimage.py makes the file from an ELF linked with --emit-relocs.
 *
 * A program runs on its own stack (the process stack) at the top of the 8 MB, with its heap between its .bss and the
 * stack. It returns from its entry, or calls leave(), to go back to MMBasic.
 */
#ifndef CMM2IMG_H
#define CMM2IMG_H
#include <stdint.h>

#define CMM2IMG_MAGIC        "CMM2IMG"         /* the header's first 8 bytes, its NUL included */
#define CMM2IMG_HDR_VERSION  1
#define CMM2IMG_API_VERSION  5                 /* the services below (2: the screen and the keys held; 3: sound;
                                                  4: the mouse; 5: Ctrl-C taken, a file's details) */
#define CMM2IMG_KIND_APP     1                 /* an application: it runs, then MMBasic goes on */
#define CMM2IMG_FLAG_G2      0x01              /* needs a G2 (every image does, for now) */
#define CMM2IMG_BASE         0xD1800000u       /* where a G2 loads an application: link here and nothing is moved */
#define CMM2IMG_AREA         0x00800000u       /* the 8 MB it has: image, .bss, heap, stack */
#define CMM2IMG_STACK        0x00010000u       /* the stack when the header asks for none: 64 KB */

#define CMM2IMG_REL_ABS32    0u                /* add the difference to the word */
#define CMM2IMG_REL_MOVT     1u                /* add the difference's top half to a Thumb-2 MOVT's immediate */
#define CMM2IMG_REL_KIND(r)   ((uint32_t)(r) >> 28)
#define CMM2IMG_REL_OFFSET(r) ((uint32_t)(r) & 0x0FFFFFFFu)

typedef struct {
    char     magic[8];                         /* "CMM2IMG\0" */
    uint16_t hdr_version;                      /* CMM2IMG_HDR_VERSION */
    uint16_t api_version;                      /* the version of the services it needs */
    uint8_t  kind;                             /* CMM2IMG_KIND_APP */
    uint8_t  flags;                            /* CMM2IMG_FLAG_... */
    uint16_t reserved;
    uint32_t link_base;                        /* the address it was linked at, a multiple of 64 KB */
    uint32_t image_size;                       /* the bytes after the header that are loaded */
    uint32_t bss_size;                         /* zeroed after the image */
    uint32_t entry;                            /* the entry point's offset (odd: a Thumb address) */
    uint32_t stack_size;                       /* 0: CMM2IMG_STACK */
    uint32_t heap_min;                         /* the least heap it can run with */
    uint32_t reloc_count;                      /* relocation words after the image */
    uint32_t crc32;                            /* of the image and the relocation words (zlib's CRC-32) */
    char     name[16];                         /* NUL padded */
} cmm2img_header;                              /* 64 bytes */

struct cmm2_services;
typedef struct {
    const struct cmm2_services *svc;
    int      argc;
    char   **argv;                             /* argv[0] the file's name, then the words after RUN's comma */
    void    *heap;                             /* the program's heap: from its .bss to its stack, 8-byte aligned */
    uint32_t heap_size;
    uint32_t base;                             /* where the image is */
} cmm2img_start;

typedef struct {                               /* the display after video_mode: page 0, the one shown */
    uint8_t *fb;                               /* its first pixel; a line is pitch bytes on from the one before */
    uint32_t width, height, pitch, bpp;
    uint32_t copies;                           /* 1, or 2 in a mode whose lines are shown twice (3, 5, 6, 7...): each line
                                                  is stored twice, so line y is written at fb + y*pitch and again width
                                                  bytes later (pitch is then 2*width) */
} cmm2_video;

typedef struct {                               /* a file's or directory's details (file_stat) */
    uint32_t size;                             /* 0 for a directory */
    uint8_t  is_dir;
    uint8_t  month, day, hour, minute, second; /* its last change (0 for a drive's root) */
    uint16_t year;
} cmm2_stat;

typedef struct {                               /* one directory entry (dir_read) */
    char     name[256];
    uint32_t size;
    uint8_t  is_dir;
} cmm2_dirent;

#define CMM2_O_READ    0x01                    /* file_open's modes */
#define CMM2_O_WRITE   0x02
#define CMM2_O_CREATE  0x04                    /* with WRITE: created, or emptied if it is there */
#define CMM2_O_APPEND  0x08                    /* with WRITE: created if need be, and written at its end */

/* The services. A program must never meet an MMBasic error: each returns a status instead, negative for an error
   (for the files, minus FatFs's FRESULT). The table only grows: check version before using a later entry. */
struct cmm2_services {
    uint32_t version;                          /* CMM2IMG_API_VERSION */
    uint32_t size;                             /* sizeof(struct cmm2_services) in the firmware */
    /* the console: the serial port and the screen, as PRINT */
    void     (*con_putc)(int c);
    void     (*con_puts)(const char *s);
    int      (*con_getc)(void);                /* the next key, or -1: it never waits */
    /* time */
    uint64_t (*micros)(void);                  /* microseconds since start-up */
    void     (*delay_us)(uint32_t us);
    void     (*datetime)(int t[6]);            /* year, month, day, hour, minute, second */
    /* files: names as MMBasic takes them (A:/dir/name, C:..., or relative to the current directory); handles 0-7 */
    int      (*file_open)(const char *name, int mode);
    int      (*file_close)(int h);
    int32_t  (*file_read)(int h, void *buf, uint32_t n);          /* the bytes read */
    int32_t  (*file_write)(int h, const void *buf, uint32_t n);   /* the bytes written */
    int      (*file_seek)(int h, uint32_t pos);
    int32_t  (*file_tell)(int h);
    int32_t  (*file_size)(int h);
    int      (*file_sync)(int h);
    int      (*file_remove)(const char *name);
    int      (*file_rename)(const char *from, const char *to);
    int      (*dir_make)(const char *name);
    int      (*dir_open)(const char *path);    /* a handle 0-3 */
    int      (*dir_read)(int h, cmm2_dirent *e);   /* 1: an entry, 0: the end */
    int      (*dir_close)(int h);
    /* the system */
    int      (*poll)(void);                    /* the USB devices and keyboards seen to; 1 once Ctrl-C has been pressed:
                                                  the program should end, and the BASIC program that ran it stops too */
    void     (*leave)(int code);               /* back to MMBasic from anywhere, as if the entry had returned code */
    uint32_t (*cpu_hz)(void);
    void    *(*alloc)(uint32_t size);          /* a block of MMBasic's heap (outside the 8 MB), or NULL: freed at the end */
    void     (*release)(void *p);
    /* the firmware's OPL3 synth (ymfm's YMF262: Src/OPLChip.cpp), its code in flash */
    void    *(*opl_new)(uint32_t rate);        /* a chip, reset, making rate Hz stereo; NULL if there is no room */
    void     (*opl_write)(void *chip, uint32_t reg, uint32_t value);   /* reg 0-0x1FF, written at once */
    void     (*opl_generate)(void *chip, int16_t *buf, uint32_t frames);   /* frames of 16-bit stereo */
    void     (*opl_free)(void *chip);
    /* version 2: the screen and the keys held. The mode and the palette BASIC had are put back when the program ends. */
    int      (*video_mode)(int mode, int depth, cmm2_video *v);   /* MODE mode,depth (depth 8 only: 256 colours from
                                                  the palette); 0 with v filled, or negative if the mode can't be set */
    int      (*video_palette)(int start, int count, const uint8_t *rgb);   /* entries from R,G,B bytes, shown from the
                                                  next frame (it waits for it) */
    void     (*video_wait)(void);              /* until the display starts its next frame */
    int      (*keys_held)(int keys[7]);        /* KEYDOWN's: [0]-[5] the keys held (MMBasic's codes: ASCII, &H80-&H89
                                                  the arrows and editing keys, &H91 up F1...), [6] the modifier bits (1
                                                  left Alt, 2 left Ctrl, 4 left GUI, 8 left Shift, 16-128 the right
                                                  ones); returns how many keys are held */
    /* version 3: sound. The stream is 16-bit stereo, made by the program's fill, which the firmware calls from an
       interrupt (PendSV, below every other) each time a buffer is wanted: 1024 frames, 23 ms. fill runs on MMBasic's
       stack, not the program's: it must be quick, keep its locals small and call no service, malloc or stdio. Its
       sound goes out at PLAY VOLUME's level. */
    int      (*audio_start)(void (*fill)(int16_t *buf, uint32_t frames));   /* the rate in Hz (44.1 kHz as the timer
                                                  makes it), or negative: -2 sound already playing, -3 no room */
    void     (*audio_stop)(void);              /* (ended for it when the program ends) */
    void     (*audio_lock)(void);              /* fill held off, while the program changes what fill reads: briefly */
    void     (*audio_unlock)(void);
    /* the music: a Standard MIDI File in the program's memory (kept there while it plays), played by the firmware's
       OPL3 synth with Doom's DMX patches and added to the stream after fill (audio_start first). bank: a GENMIDI
       lump's 175 instruments of 36 bytes (after its 8-byte "#OPL_II#"), or NULL for the firmware's (Freedoom's). */
    int      (*music_play)(const void *midi, uint32_t size, int loop, const uint8_t *bank);   /* 0, or -1 not a MIDI
                                                  file, -2 its timing not supported, -3 no room */
    void     (*music_stop)(void);
    void     (*music_volume)(int volume);      /* 0-127 (127 at the start) */
    void     (*music_pause)(int paused);
    int      (*music_playing)(void);           /* 0 once a file not looped has ended */
    void     (*music_rate)(int divisor);       /* the synth's output at its full rate, 49,716 Hz (1, the default), or
                                                  at 1/2, 1/3 or 1/4 of it: less CPU (Doom's music, 50% at 1, 37% at
                                                  2), less treble; pitch and timing are kept. Held till the end */
    /* version 4: the mouse in use (USB, PS/2 or I2C: MM.MOUSE's), its position kept by the firmware in the screen's
       pixels (clamped to it, and centred by video_mode) */
    int      (*mouse)(int m[4]);               /* 1 with m: x, y, the buttons (1 left, 2 right, 4 middle), the wheel's
                                                  count; 0 if there is no mouse (m all 0) */
    void     (*mouse_set)(int x, int y);       /* its position moved there */
    void     (*con_screen)(int on);            /* 0: con_putc, con_puts (and so stdout) to the serial console only,
                                                  not drawn over a program's display; 1 (the start): both */
    /* version 5 */
    int      (*break_take)(void);              /* 1 if Ctrl-C has been pressed since the last call, which clears it:
                                                  the program deals with it (a BASIC program that ran it goes on);
                                                  poll() reports it too, until it is taken */
    int      (*file_stat)(const char *name, cmm2_stat *st);   /* 0, or minus FatFs's FRESULT (4: no such file) */
};

#endif
