"""gen_audio.py OUTDIR: two-second test tones for PLAY WAV, FLAC and MP3 on the CMM2, the program that times them
(audiotest.bas), a one-second mono WAV at 44.1 kHz for PLAY EFFECT, a1test.bas copied beside them, and tiny.mod (2.56 s,
for PLAY MODFILE's interrupt). Needs numpy, soundfile and lameenc."""
import os, sys, shutil
import numpy as np, soundfile as sf, lameenc
out = sys.argv[1]
os.makedirs(out, exist_ok=True)

def tone(rate, channels, secs=2.0):
    t = np.arange(int(rate * secs)) / rate
    l = 0.5 * np.sin(2 * np.pi * 440 * t)
    r = 0.5 * np.sin(2 * np.pi * 660 * t)
    return np.stack([l, r], axis=1) if channels == 2 else l[:, None]

files = []
for name, rate, ch, kind in (('ts_wav', 44100, 2, 'WAV'), ('tm_wav', 22050, 1, 'WAV'),
                             ('ts_flac', 44100, 2, 'FLAC'), ('ts_mp3', 44100, 2, 'MP3'),
                             ('tm_flac', 44100, 1, 'FLAC'), ('tm_mp3', 22050, 1, 'MP3')):
    a = tone(rate, ch)
    if kind == 'MP3':
        enc = lameenc.Encoder(); enc.set_bit_rate(128); enc.set_in_sample_rate(rate); enc.set_channels(ch); enc.set_quality(2)
        pcm = (a * 32767).astype('<i2').tobytes()
        data = enc.encode(pcm) + enc.flush()
        open(os.path.join(out, name + '.mp3'), 'wb').write(data)
        files.append((name + '.mp3', 'MP3', ch))
    else:
        ext = 'wav' if kind == 'WAV' else 'flac'
        sf.write(os.path.join(out, name + '.' + ext), a, rate, subtype='PCM_16', format=kind)
        files.append((name + '.' + ext, kind, ch))

sf.write(os.path.join(out, 'tm44_wav.wav'), 0.37 * np.sin(2 * np.pi * 523 * np.arange(44100) / 44100), 44100, subtype='PCM_16')
shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'a1test.bas'), out)

# tiny.mod: a ProTracker module of known length for PLAY MODFILE's play-once interrupt - one looped square sample, a
# note on row 0 with speed 1 (one 20 ms tick a row at 125 BPM), and its one pattern played twice: 128 rows, 2.56 s
import struct
smp = bytes([64] * 16 + [192] * 16)                                 # signed 8-bit: +64 then -64
hdr = b'tiny'.ljust(20, bytes(1))
hdr += b'square'.ljust(22, bytes(1)) + struct.pack('>HBBHH', len(smp) // 2, 0, 64, 0, len(smp) // 2)
hdr += (bytes(22) + struct.pack('>HBBHH', 0, 0, 0, 0, 1)) * 30
hdr += bytes([2, 127]) + bytes(128) + b'M.K.'
pat = bytearray(64 * 4 * 4)
pat[0:4] = bytes([428 >> 8, 428 & 0xFF, (1 << 4) | 0xF, 1])           # sample 1, period 428, effect F01: speed 1
open(os.path.join(out, 'tiny.mod'), 'wb').write(hdr + bytes(pat) + smp)

bas = ["' audiotest.bas - each two-second tone from PLAY to its completion interrupt (the dr_ libraries update)",
       "Dim integer fired, i",
       "Dim float t",
       "Dim string f$, k$, s$",
       "For i = 1 To %d" % len(files),
       "  Read f$, k$",
       "  fired = 0",
       "  t = Timer",
       "  Select Case k$",
       "    Case \"WAV\" : Play Wav f$, done",
       "    Case \"FLAC\" : Play Flac f$, done",
       "    Case \"MP3\" : Play Mp3 f$, done",
       "  End Select",
       "  Pause 300 : s$ = MM.Info(SOUND)",
       "  Do While fired = 0 And Timer - t < 6000 : Loop",
       "  Print f$; \"  \"; s$; \"  \"; Choice(fired, Str$(Int((Timer - t) / 100 + 0.5) / 10, 1, 1) + \" s\", \"no interrupt in 6 s\")",
       "  Play Stop",
       "  Pause 200",
       "Next",
       "Print \"done\"",
       "End",
       "Sub done",
       "  fired = 1",
       "End Sub"]
bas += ['Data "%s", "%s"' % (n, k) for (n, k, ch) in files]
open(os.path.join(out, 'audiotest.bas'), 'w', newline='\n').write('\n'.join(bas) + '\n')
for n, k, ch in files: print(n, k, ch, os.path.getsize(os.path.join(out, n)))
