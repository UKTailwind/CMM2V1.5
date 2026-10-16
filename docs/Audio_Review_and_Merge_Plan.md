# Audio: the CMM2's Audio.c against the PicoMite's, and a merge plan

Written 16 Oct 2026 for Peter. It compares `Src/Audio.c` (2,030 lines) and the audio interrupt in `Src/stm32h7xx_it.c` with the PicoMite's `io/Audio.c` (3,770 lines), `io/AudioBBC.c`, `misc/SDCard.c` (its output interrupt) and `third_party_mod/hxcmod.c`. The CMM2 always drives the H7's two 12-bit DACs from TIM4, so the PicoMite's PWM, I2S and VS1053 paths do not apply. References are `C:` for our Audio.c, `C-it:` for our stm32h7xx_it.c and `P:` for the PicoMite's Audio.c. Everything here comes from reading the code; the items marked **checked** were confirmed line by line.

## 1. Summary

The CMM2's audio has a set of plain bugs, several of which the PicoMite fixed long ago: completion interrupts that never fire, mono FLAC and MP3 at double speed, a playlist that overflows its buffer, MM.INFO(TRACK) copying from NULL, and cleanup done inside the audio interrupt. The PicoMite has five PLAY forms that need nothing but the DACs (ARRAY, SAMPLE, RELEASE, BBC SOUND, BBC ENVELOPE, with LOAD SOUND for a user wave table), a play-once MOD with an interrupt, and better output handling (volume ramps, a click-free stop). The CMM2 keeps TTS, EFFECT, the MOD rate argument and 96 kHz playback. The PicoMite's code also has bugs of its own that must not come across; they are listed in section 6 to be filed as PicoMite issues.

## 2. Bugs in the CMM2's audio

| | Bug | Where | The PicoMite |
|---|---|---|---|
| a | **checked** PLAY TONE and PLAY TTS never fire their completion interrupt: at the end the interrupt calls `CloseAudio(1)`, which sets `WAVInterrupt = NULL` before `WAVcomplete` is set | C-it:871-875, 983-987; C:1205 | `fillToneBuffer` sets `playreadcomplete`; `audio_checks` signals in the main loop without clearing the handler (P:3187, 3730-3767) |
| b | A directory of more than one track never fires its interrupt: each advance goes through `CloseAudio(0)`, which also clears `WAVInterrupt` | C:1266-1268, 1309-1311, 1341-1343 | handler kept |
| c | **checked** Mono FLAC and MP3 play at double speed with the channels alternating: `mono` is set only for WAV and EFFECT, and the interrupt's mono path tests `P_WAV`; an odd frame count at the end can step past `bcount` and hang until PLAY STOP | C:1290, 1726; C-it:891, 895-900 | `mono` for all three (P:973, 1085, 1163; SDCard.c:360-365) |
| d | `CloseAudio` (heap frees, file close, `drmp3_uninit`, freeing the playlist or the 220 KB TTS buffer) runs inside the TIM4 interrupt, racing the heap and FatFs in the main loop; `WAVcomplete` is not volatile | C-it:873, 985; C:131 | the interrupt only stops itself; cleanup in `audio_checks` |
| e | **checked** A directory with more than 20 matching files writes past the 20-entry playlist (`MAXALBUM`); also `fno` is read uninitialised when `f_stat` fails, and `== AM_DIR` misses directories with other attributes set | C:86, 1618-1637 (and the FLAC and MP3 copies) | stops at 20 |
| f | **checked** MM.INFO(TRACK) copies `alist[trackplaying].fn` while a single file plays, when `alist` is NULL | MM_Misc.c:3236-3238 | uses `WAVfilename` |
| g | **checked** PLAY SOUND's "Invalid type" can never fire (the mode is set to `nulltable` first and the check looks for NULL), so a bad type silently turns the channel off; the live channel is changed before the "in use" and frequency checks | C:1532-1564 | local copies committed at the end (P:2165, 2193, 2282) |
| h | PLAY SOUND while paused restarts a running timer and fails with "Starting audio timer" | C:1589-1593, 1136 | accepts `P_PAUSE_SOUND` |
| i | **checked** A failed PLAY TTS frees `buffer` but leaves `sbuff1`/`sbuff2` pointing at it; the next PLAY or CloseAudio frees it again | C:1176-1185 | n/a |
| k | **checked** PLAY MODFILE does not check that the module loaded, and its prescaler is `PSpeedDiv/(rate-1)` where every other path has `PSpeedDiv/rate-1` | C:1819-1825 | checks `mod_loaded` |
| l | A click on stop and on pause: the DACs jump to 2000 | C:1905-1916; C-it:1047-1051 | ramps to the midpoint over about 2 ms (P:3119-3152) |
| m | **checked** MODSAMPLE volume 0 becomes 255 (`unsigned char`, `getint(..,0,64) - 1`), about four times full scale; the PicoMite has the same | C:1656, 1667-1671 | same bug |
| n | Unchecked: `drwav_init`'s result and a NULL from `drflac_open`; files with more than two channels overflow the swing buffers; FLAC's 96 kHz check comes after the buffers are allocated; MP3 has no rate check | C:1277, 1353-1367, 1715 | same, mostly |
| o | Smaller: `bcount` and `SoundPlay` are 64-bit volatiles shared with the interrupt on a 32-bit core; the RNG is called inside the interrupt; TTS's `strncat(tts, "[", 256)` on a 256-byte string | C:122, 152, 1168; C-it:1014, 1033 | |

One more came from the dr_ libraries update itself and is already fixed in the working tree: PLAY MP3's decoder, now on the heap, was not freed when a track ended by itself.

## 3. PicoMite features that need only the DACs

- `PLAY ARRAY left%(), right%(), freq [, start [, end [, interrupt]]]`: 16-bit samples, four to an integer element (P:1732-1797). Its `audiorepeat` upsampling is a PWM device the timer does not need.
- `PLAY SAMPLE left%(), right%(), freq, A, D, S, R [, freqR, AR, DR, SR, RR] [, interrupt]` and `PLAY RELEASE`: one cycle looped under ADSR envelopes, one sample per element (P:1798-1942, 1369-1451). Built for the RP2350 only; that is a CPU limit, not a hardware one.
- `PLAY LOAD SOUND array%()`: a 1024-element user wave table for PLAY SOUND type "U" (P:1493-1510).
- `PLAY BBC SOUND chan, amp, pitch, dur`, `PLAY BBC ENVELOPE` (14 values) and `MM.INFO(BBC BUFFER [chan])`: AudioBBC.c, integer code filling the swing buffers at 44.1 kHz.
- PLAY SOUND position "M", and string expressions for position and type.
- `PLAY MODFILE file$, interrupt` plays once and fires the interrupt (hxcmod's `noloop`); MM.INFO(TRACK) covering MOD.
- A playlist may mix formats (`PlayAlbumTrack`, which the file manager could use too).
- Output: volume changes ramp instead of stepping (P:646-690, 1604-1625); stop and pause ramp to the midpoint; band-limited square and sawtooth (polyBLEP) for PLAY SOUND.
- Structure: tones, sounds and synthesis are generated into buffers in the main loop, so the interrupt only plays buffers (P:3155-3411).

VS1053 only, so not for the CMM2: `PLAY NOTE`, `PLAY STREAM`, `PLAY MIDI`, `PLAY MIDIFILE`, `PLAY HALT`, `PLAY CONTINUE`.

## 4. Behaviour that differs, where one has to be chosen

1. Does PLAY STOP or CLOSE fire the user's interrupt? The PicoMite's `CloseAudio` keeps the handler and signals it, so stopping (and each track change in an album) fires it; the CMM2 clears it. Proposal: fire it at the natural end only, as the CMM2 intends.
2. MOD mixing: the CMM2 mixes at the true output rate with a rate argument (8000 to 48000); the PicoMite mixes at 22050 and doubles. Keep the CMM2's, and add the play-once interrupt as a further argument.
3. PAUSE when already paused: an error on the CMM2, silent on the PicoMite. Proposal: silent.
4. PLAY TONE while paused: the CMM2 resumes, the PicoMite stays paused. Proposal: the CMM2's.
5. MM.INFO(SOUND) strings differ ("MODFILE", "TTS", "DAC", a bare "PAUSED" against "MOD", "PAUSED WAV", "ARRAY", "SAMPLE", "BBC"). A program tests these, so they follow the dialect switch: the CMM2's under the CMM2 dialect.
6. FLAC and MP3 are decoded to float on the CMM2 and come out at a lower level than WAV (±1900 counts against full scale); the PicoMite decodes all three to 16 bits at one level. Proposal: the PicoMite's, so the formats play at the same loudness; MOD keeps its headroom for EFFECT.
7. Sample-rate limits: keep the CMM2's 96 kHz for WAV and FLAC, add the check to MP3, and refuse rates below what TIM4's 16-bit prescaler can reach.

## 5. CMM2-only, to keep

PLAY TTS [PHONETIC] (sam.c, reciter.c); PLAY EFFECT, the second WAV stream mixed over MOD; the MODFILE and MODSAMPLE rate arguments; 96 kHz WAV and FLAC; the empty-string playlist of the current directory; DAC START/STOP sharing TIM6 (`P_DAC`); the hardware RNG for noise; the SD-card-removed check in `onRead`. The `CurrentlyPlaying` enum differs (the CMM2 puts `P_NOTHING` first and adds `P_TTS`, `P_DAC`); the PicoMite's tests such as `CurrentlyPlaying < P_STOP` depend on its order, so ported logic must use names, not order.

## 6. PicoMite bugs not to carry across

Checked against the code and filed as PicoMite issues, except the two marked unchecked.


- PLAY ARRAY has no "Sound output in use" check, and frees and replaces the buffers of whatever is playing (P:1732-1797). Issue 25.
- `audio_checks` frees `mcontext` without setting it to NULL, and the next `CloseAudio` frees it again, after a play-once MOD (P:3741-3742, 759). Issue 26.
- When `hxcmod_fillbuffer` stops at `noloop` partway through a buffer, the caller still plays and converts the whole buffer (hxcmod.c:1364-1369; P:3543-3557), probably a burst at the end. Unchecked.
- PLAY SOUND picks the phase increment from the old type when switching to or from noise (P:2229, 2252). Issue 27.
- `SoundPlay = 1000;` before PLAY TONE's arguments are parsed, so a failing PLAY TONE cuts a running endless tone short (P:1628). Issue 30.
- TONE, SOUND and ARRAY do not clear an old `WAVInterrupt`, so a finished WAV's handler can fire again when they stop (only BBC SOUND clears it, P:1981). Unchecked.
- PLAY HALT calls `lfs_file_tell` on the .mem file instead of the track (P:2769-2770; VS1053 builds). Issue 29.
- MODSAMPLE volume 0 (as on the CMM2, 2m). Issue 28.

## 7. Merge order

As separate commits, each built with 0 warnings and with tests where a program can see the effect:

| # | Commit | Content |
|---|---|---|
| A1 | Fixes | 2a, 2b, 2c, 2e, 2f, 2g, 2h, 2i, 2k, 2m, 2n: completion interrupts kept, mono for all formats, the playlist bound, MM.INFO(TRACK) from the playing file's name, SOUND's checks before its changes, the TTS failure, the MOD checks, volume 0, the init results, more than two channels refused |
| A2 | Interrupt-safe cleanup | 2d: the interrupt stops itself and sets a flag; `audio_checks` does the cleanup, as the PicoMite |
| A3 | Synthesis | PLAY ARRAY, SAMPLE, RELEASE, LOAD SOUND, PLAY SOUND "M" and type "U", generated into buffers in the main loop |
| A4 | BBC sound | AudioBBC.c, PLAY BBC SOUND and ENVELOPE, MM.INFO(BBC BUFFER) |
| A5 | MOD | hxcmod's `noloop` (with the buffer-tail fix), PLAY MODFILE's interrupt, MM.INFO(TRACK) for MOD |
| A6 | Output | volume ramps, the click-free stop and pause on the DACs, polyBLEP square and sawtooth, one 16-bit level for all formats |

## 8. Decisions

1. The interrupt on PLAY STOP and on each track of an album (4.1): **the natural end only** (Peter, 16 Oct; done in A1).
2. One output level for all formats (4.6): **decoding to 16 bits and one level, ±1900 about 2000** (Peter, 16 Oct; done, below). A correction to 4.6: at VOLUME 100 a full-scale WAV swings ±2048 counts about 2048 (the DAC's whole range), FLAC, MP3, TONE and SOUND ±1900 about 2000, MOD ±1024 about 2048 (room for EFFECT). FLAC and MP3 are only 7% (0.6 dB) below WAV, not audibly quieter. The real choice is decoding FLAC and MP3 to 16 bits as WAV and the PicoMite do (half their buffer memory, one conversion for all three), and one level and centre for everything; proposed ±1900 about 2000, which leaves WAV 0.6 dB quieter, keeps it off the DAC's rails (the output buffer is on) and makes the centre the idle level, so starting and stopping a WAV no longer steps 48 counts.
3. PAUSE when already paused is silent (4.3); PLAY TONE while a tone is paused takes the new tone and stays paused until PLAY RESUME (4.4): both the PicoMite's (Peter, 16 Oct; done, below).
4. MM.INFO(SOUND): **the PicoMite's strings throughout** (Peter, 16 Oct): "MOD" for "MODFILE", "PAUSED TONE", "PAUSED WAV" and so on for "PAUSED", and "ARRAY", "SAMPLE", "BBC" as those come; "TTS" and "DAC" stay, as the PicoMite has no such states. A CMM2 program that tests for "MODFILE" or a bare "PAUSED" needs changing.

## 9. Progress

**A1, the fixes (16 Oct).** 2a, 2b, 2c, 2e, 2f, 2g, 2h, 2i, 2k, 2m and 2n as planned, and 2d with them, because the TONE and TTS interrupts needed it: at its end the audio interrupt now only stops TIM4, sets the DACs to the midpoint and raises `audiofinished`; `audio_checks` (from `CheckSDCard`, and at the start of every PLAY) does the `CloseAudio` and sets `WAVcomplete`, which is volatile. So A2 is folded into A1. Details:

- `CloseAudio(0)`, a track change, keeps `WAVInterrupt` and clears `WAVcomplete` (it set it, which with the handler kept would have fired the interrupt at every track); the dispatcher clears `WAVInterrupt` when it fires it, so a finished sound's handler cannot fire again at the end of a later one; PLAY TONE without an interrupt drops an old one.
- `mono` is set for FLAC and MP3; the interrupt's mono path serves all three formats and stops at `bcount`.
- The playlist stops at `MAXALBUM`, tests `f_stat`'s result and `AM_DIR` as a bit, and skips a name that would not fit in an entry.
- MM.INFO(TRACK) returns `WAVfilename`, set by the WAV, FLAC and MP3 starts (also while paused).
- PLAY SOUND finds the type into a local and checks position, use, type, frequency and volume before it changes the channel; while paused it neither restarts the timer nor leaves the paused state.
- WAV and FLAC refuse more than two channels; init failures close the file (and free the MP3 decoder) before the error, with "Not a WAV file it can play" and "Not a FLAC file it can play"; FLAC checks its rate before it allocates.
- MODFILE's prescaler is `PSpeedDiv/rate-1`; a module that hxcmod cannot load is refused ("Not a MOD file it can play") and its buffers freed.

Found while testing on Peter's audio card, and fixed in A1:

- PLAY WAV, FLAC or MP3 of a directory given with its drive (`PLAY WAV "A:/SOUNDS"`) scanned the directory and then failed with "The logical drive number is invalid": each entry was the current directory and the argument glued together, `A:/A:/SOUNDS/...`. The entries are now made from the directory's full path (`getfullpath`, which resolves the drive, `..` and the current directory).
- An empty directory gave "Could not find the file"; now "No WAV files in the directory" (FLAC, MP3), with the playlist freed.

Tests: `tests/audio` (README there). On Peter's card every check passes: each two-second tone, mono and stereo, takes 2.1 to 2.2 s to its interrupt (mono FLAC and MP3 took about 1 s before); the 500 ms TONE fires at 503 ms; TTS fires; 53 WAVs in a directory play 20 tracks with one interrupt at the end; PLAY STOP fires nothing; every module on the card loads except an Impulse Tracker file named `.MOD` and two corrupt ones.

Not fixed: hxcmod takes any file without a signature for a 15-sample Soundtracker module, so PLAY MODFILE of a FLAC file "plays" noise (the PicoMite has the same); a check of the song length and order table would refuse most such files. 2l goes with A6; 2o is left. The decisions of section 8 are still open, and nothing in A1 depends on them beyond firing the interrupt at the natural end only, which is what the CMM2 meant to do.

**The decisions of section 8 (16 Oct).** FLAC and MP3 decode to 16 bits (`DR_MP3_FLOAT_OUTPUT` off, as the PicoMite; with it on, dr_mp3's 16-bit read converts through a 9 KB array on the stack), into buffers of half the size (MP3 two of 16 KB instead of 40 KB, FLAC instead of 32 KB), and WAV, FLAC and MP3 share one conversion, `iconvert`. Every source is centred on 2000, where the DACs rest, at ±1900 for full scale at VOLUME 100: MOD and EFFECT at half each, as before; TTS's centre no longer moves with the volume. Measured by reading the DAC output registers while each plays (`tests/audio/levels.bas`, half-scale tones):

| Source | Before | After |
|---|---|---|
| WAV | 1024 to 3071, mean 2047 | 1050 to 2949, mean 2000 |
| FLAC | 1050 to 2950, mean 1998 | 1050 to 2950, mean 2000 |
| MP3 | 1094 to 2906, mean 2001 | 1095 to 2905, mean 2000 |
| TONE | 100 to 3900, mean 2000 | the same |
| TTS at VOLUME 50 | 208 to 1136, mean 664 | 1572 to 2443, mean 2003 |
| TTS at VOLUME 100 | 640 to 3488, mean 2056 | 694 to 3350, mean 2007 |
| MOD (PTETRIS) | mean 2044 | mean 1996 |

PAUSE when paused does nothing; PLAY TONE while a tone is paused stays paused; MM.INFO(SOUND) gives the PicoMite's strings ("MOD", "PAUSED WAV" and so on; "TTS" and "DAC" stay) and "OFF" as soon as a tone or speech has ended. In the interrupt, MOD no longer stops while an EFFECT waits for data.

**A3, synthesis (16 Oct).** The PicoMite's `PLAY ARRAY`, `PLAY SAMPLE`, `PLAY RELEASE` and `PLAY LOAD SOUND`, and PLAY SOUND's position and type as string expressions, position "M" (the PicoMite's: the same as "B") and type "U". Differences from the plan and from the PicoMite:

- TONE and SOUND are still computed in the audio interrupt; ARRAY and SAMPLE fill the same pair of buffers as WAV in the main loop (`readarray`, `readsamplearray`) and play through the WAV path of the interrupt. Moving TONE and SOUND to buffers, as the PicoMite does, gains nothing at 480 MHz and would add a buffer of latency.
- TIM4 cannot run below about 1.8 kHz, so ARRAY repeats each sample until the rate is 32 kHz or more, as the PicoMite's `audiorepeat` does for its PWM; StopAudio puts the repeat back to 1.
- ARRAY and SAMPLE check that the output is free ("Sound output in use"; issue 25 is the PicoMite's ARRAY without the check), parse both arrays into locals and change nothing until every argument is checked, and stop a running ARRAY or SAMPLE before starting the next.
- SAMPLE is not limited to the RP2350's build.
- The loaded table is forgotten by PLAY STOP and CLOSE (and RUN), not at every track change as on the PicoMite.
- A type that is not a keyword is evaluated as a string, so an unquoted wrong letter gives "Expected a string", as on the PicoMite.

Tests: `tests/audio/a3test.bas`. ARRAY takes 1.00 s for 8000 samples at 8000 a second, 44000 at 44000 and 100 at 100, 0.50 s for samples 4000 to 8000; its ±16000 sine reaches the DAC as 1073 to 2927 (±928 expected); SAMPLE at 50% sustain gives 1696 to 2304 (±304 expected) and its interrupt fires 251 ms after a RELEASE of 200 ms (one buffer of latency) and 352 ms after an attack of 50 and a decay of 300 to a sustain of 0; the loaded square table plays at exactly the level of type "Q". The A1 tests and `levels.bas` give the same results as before.

**A4, BBC sound (16 Oct).** `Src/AudioBBC.c` is the PicoMite's file unchanged but for a note, with `PLAY BBC SOUND`, `PLAY BBC ENVELOPE` and `MM.INFO(BBC BUFFER [channel])`. Its buffers play through the WAV path of the interrupt at 44100; they are 4 KB (23 ms) where the PicoMite's are 2 KB, as the CMM2 refills from its routine checks. CloseAudio silences the engine and keeps the envelopes; InitHeap (RUN, NEW, program load) frees its block. A note queued after the engine has run dry and the interrupt has stopped the timer starts it again.

Tests: `tests/audio/a4test.bas`, at the DAC. Pitch 89 measures 440 Hz, 137 880 Hz and 41 220 Hz; amplitude -15 gives ±445 (15 × 8 × 64 scaled to ±1900); a note of duration 20 ends 1.02 s after it starts; four queued notes leave 12 of 16 free; 20 notes of duration 1 queue in 0.16 s (the last waiting for room) and finish at 1.05 s (21 × 0.05); an envelope's decay to 60 sustains at ±222 and its release ends the sound; envelopes survive PLAY STOP; 41 notes queued at 40 to 120 ms after a short one, around the moment the engine runs dry, all end cleanly.
