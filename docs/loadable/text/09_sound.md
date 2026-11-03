# Sound and Music

A program makes sound by filling buffers: the firmware plays a stream of 16-bit stereo samples on the audio output and calls a function of the program's whenever it needs another buffer. On top of that the firmware can play MIDI music on its FM synthesiser, mixed into the same stream. PLAY VOLUME's setting applies to both.

## The Sound Stream

```
int  audio_start(void (*fill)(int16_t *buf, uint32_t frames));    /* version 3 */
void audio_stop(void);
void audio_lock(void);
void audio_unlock(void);
```

audio_start starts the stream and returns its rate in samples a second (about 44100: the exact rate the timer makes), or -2 if MMBasic is playing a sound, or -3 if there is no memory for the buffers. From then on the firmware calls fill each time a buffer is wanted, with buf to fill with frames pairs of samples (left, right, left, right...). A buffer is 1,024 frames, 23ms of sound, and there are two: while one plays the other is filled.

audio_stop stops it, and it is stopped when the program ends.

This program plays a tone whose pitch follows the mouse:

```
#include <math.h>
#include "cmm2.h"

static volatile float step;                           /* the phase step of a sample */
static float phase;

static void fill(int16_t *buf, uint32_t frames) {
    uint32_t i;
    for(i = 0; i < frames; i++) {
        int16_t s = (int16_t)(8000 * sinf(phase));
        buf[2 * i] = buf[2 * i + 1] = s;
        phase += step;
        if(phase > 6.2831853f) phase -= 6.2831853f;
    }
}

int main(int argc, char **argv) {
    int m[4], rate;
    if(cmm2->version < 4) return 1;
    rate = cmm2->audio_start(fill);
    if(rate < 0) return 2;
    while(!cmm2->poll()) {
        float hz = cmm2->mouse(m) ? 200 + m[0] : 440;
        step = 6.2831853f * hz / rate;               /* one float: no lock needed */
        cmm2->delay_us(10000);
    }
    cmm2->audio_stop();
    return 0;
}
```

## Code Called From an Interrupt

fill (and a frame hook, in the chapter The Screen) is called from an interrupt, PendSV, at the lowest priority of all the firmware's interrupts, while the program's own code is stopped. So it must follow some rules:

- **Be quick.** Its time comes off the program's: a fill that takes 5ms of each 23ms buffer takes over a fifth of the processor. It must always finish well within 23ms, or the sound breaks up.
- **Keep its local variables small.** It runs on MMBasic's stack, not the program's, and that has a few kilobytes to spare at most. Large tables belong in static memory.
- **Call no services** (but dma2d_copy and dma2d_busy from a frame hook), and no malloc(), free(), printf() or other C library function that keeps state. Calculation (string.h, math.h) is fine.
- **Share data with care.** A variable that both fill and the program change should be volatile. When the program changes more than one thing that fill reads (a voice's start, length and step, say), it does so between audio_lock() and audio_unlock(), which hold fill off: briefly, as the buffers run on meanwhile.

Doom's sound effects are mixed this way: its fill adds up sixteen voices of 8-bit samples, and its sound functions set up a voice inside audio_lock() and audio_unlock().

## Music

```
/* version 3 */
int  music_play(const void *midi, uint32_t size, int loop, const uint8_t *bank);
void music_stop(void);
void music_volume(int volume);
void music_pause(int paused);
int  music_playing(void);
void music_rate(int divisor);
```

music_play plays a Standard MIDI File (types 0 and 1), held in the program's memory, on the firmware's FM synthesiser, as PLAY MIDIFILE does: the music is added to the stream after fill has made each buffer, so audio_start must be called first (a program with no sound of its own gives it a fill that makes silence). The file must stay in memory while it plays. loop plays it again from the start each time it ends. bank is a GENMIDI instrument bank, the format that Doom and its relatives use: 175 instruments of 36 bytes (the 128 General MIDI instruments, then the drums of notes 35 to 81), the bytes after a GENMIDI lump's 8-byte header "#OPL_II#". NULL uses the firmware's bank, which is Freedoom's. It returns 0, -1 if the data is not a MIDI file, -2 if its timing is in SMPTE frames, or -3 if there is no memory for the synthesiser.

music_stop stops it. music_volume sets its level, 0 to 127 (127 at the start). music_pause(1) pauses it and music_pause(0) resumes it. music_playing returns 0 once a file not looped has ended.

The synthesiser takes a large share of the processor: on a Generation 2, from about 20% with no notes sounding to about 54% with all 18 voices (a Generation 1 at 400MHz needs a fifth more). music_rate(n) makes it produce its sound at 1/n of its full rate of 49,716 samples a second (n is 1 to 4), which cuts its share without changing the pitch or the timing, at the cost of some treble. Doom asks for half the rate, which takes its music from 51% to 37% of the processor; the full rate is the default.

This program plays a MIDI file once, named after RUN's comma, with the firmware's instruments:

```
#include <stdio.h>
#include <string.h>
#include "cmm2.h"

static uint8_t song[256 * 1024];

static void silence(int16_t *buf, uint32_t frames) { memset(buf, 0, frames * 4); }

int main(int argc, char **argv) {
    FILE *f = fopen(argc > 1 ? argv[1] : "music.mid", "rb");
    uint32_t n;
    if(f == NULL || cmm2->version < 3) return 1;
    n = fread(song, 1, sizeof(song), f);
    fclose(f);
    if(cmm2->audio_start(silence) < 0) return 2;
    if(cmm2->music_play(song, n, 0, NULL) < 0) return 3;
    while(cmm2->music_playing() && !cmm2->poll()) cmm2->delay_us(10000);
    return 0;                         /* the sound stops as the program ends */
}
```

## The FM Synthesiser Directly

For a program that drives an OPL chip register by register (a DOS game's AdLib driver, an emulator), the opl_ services give it chips of its own: see The FM Synthesiser in the chapter The Services. It makes their sound in its main code into a buffer of its own, and its fill copies from that buffer.
