"""mkopldata.py GENMIDI.OP2 TUNE.mid OUT.c: Src/OPLData.c, the data PLAY MIDI keeps in flash.

  OPLGenMidi    the GENMIDI instrument bank: 175 instruments of 36 bytes (128 General MIDI programs, then the drums
                of notes 35-81), the file's "#OPL_II#" header and its 32-byte names left out
  OPLTestTune   the MIDI file PLAY MIDI TEST plays

Both come from Freedoom (https://freedoom.github.io, BSD-3-Clause, whose notice OUT.c carries): GENMIDI from the
lump in freedm.wad of release 0.13.0, the tune from its musics directory. Standard library only.
"""
import os, sys

LICENCE = '''Copyright (c) 2001-2024 Contributors to the Freedoom project. All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:
  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
    disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
    following disclaimer in the documentation and/or other materials provided with the distribution.
  * Neither the name of the Freedoom project nor the names of its contributors may be used to endorse or promote
    products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.'''


def carray(name, data):
    lines = []
    for i in range(0, len(data), 24):
        lines.append('    ' + ','.join('0x%02X' % b for b in data[i:i + 24]) + ',')
    return 'const unsigned char %s[%d] = {\n%s\n};\n' % (name, len(data), '\n'.join(lines))


gm = open(sys.argv[1], 'rb').read()
if gm[:8] != b'#OPL_II#' or len(gm) < 8 + 175 * 36:
    sys.exit('%s: not a GENMIDI bank' % sys.argv[1])
tune = open(sys.argv[2], 'rb').read()
if tune[:4] != b'MThd':
    sys.exit('%s: not a MIDI file' % sys.argv[2])
with open(sys.argv[3], 'w', newline='\n') as f:
    f.write('/* OPLData.c: made by tools/mkopldata.py, not by hand. The GENMIDI instrument bank (175 instruments of 36 bytes:\n')
    f.write('   the 128 General MIDI programs, then the drums of notes 35-81) and the tune of PLAY MIDI TEST (%s), both from\n' % os.path.basename(sys.argv[2]))
    f.write('   Freedoom 0.13.0 (BSD-3-Clause), for PLAY MIDI (OPL.c). Freedoom\'s notice:\n\n')
    f.write('\n'.join('   ' + l for l in LICENCE.split('\n')) + '\n*/\n\n')
    f.write(carray('OPLGenMidi', gm[8:8 + 175 * 36]))
    f.write('\n')
    f.write(carray('OPLTestTune', tune))
    f.write('const unsigned int OPLTestTuneSize = %d;\n' % len(tune))
print('%s: GENMIDI %d bytes, %s %d bytes' % (sys.argv[3], 175 * 36, os.path.basename(sys.argv[2]), len(tune)))
