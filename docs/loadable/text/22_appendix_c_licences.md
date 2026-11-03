# Appendix C – Licences

## The Tools

The files that are built into every program, tools/sdk/crt0.c, syscalls.c, cmm2.h and app.ld, and the header Inc/cmm2img.h, are under the MIT licence, as are the examples (hello.c, vidtest.c, nopoll.c, oplbench.c and oplplay.c):

Copyright (c) 2026 Peter Mather.

Permission is hereby granted, free of charge, to any person obtaining a copy of this file, to deal in it without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of it, and to permit persons to whom it is furnished to do so, subject to the following condition: the above copyright notice and this permission notice shall be included in all copies or substantial portions.

THE FILE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE FILE OR THE USE OR OTHER DEALINGS IN IT.

So a program built with them can have whatever licence its author chooses. The C library linked into it is newlib's, under its own permissive licences (BSD-style).

## Code Beside the Examples

- tools/sdk/examples/nuked, Nuked-OPL3, is under the GNU LGPL version 2.1 or later, so a program built with it (oplbench.app and oplplay.app) is under the LGPL as a whole.
- emu8950, used by oplplay, is under the MIT licence.

## The Programs Included with the Firmware

- **doom.app** is built from doomgeneric, which is Doom's source code under the GNU GPL version 2. Its source is doomgeneric at the commit named in apps/doom/make.py, with the files of apps/doom.
- **Freedoom** (freedoom1.wad), the game data played by doom.app, is copyright the Freedoom project's contributors, under a BSD 3-Clause licence.
- **scummvm.app** is built from frank-quest, a port of ScummVM, under the GNU GPL version 2 or later. Its source is frank-quest at the commit named in apps/scummvm/make.py, with the files of apps/scummvm.
- **micropython.app** is built from MicroPython (MIT licence) with ulab (MIT) and usqlite with SQLite (public domain), and the Pico Computer 3's modules.

## In the Firmware

The firmware's FM synthesiser, which the opl_ and music_ services use, is ymfm, copyright 2021 Aaron Giles, under a BSD 3-Clause licence. Its MIDI instrument bank comes from Freedoom, under a BSD 3-Clause licence. Their notices are in the firmware's release notes.
