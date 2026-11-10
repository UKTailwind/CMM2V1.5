"""make.py: builds apps/amiga/out/amiga.app, an Amiga 500 for the CMM2: pico-amiga (drewpo28's line-based emulator for the
RP2350, Musashi's 68000 and its own OCS/ECS chipset) fetched the first time into apps/amiga/pico-amiga (git ignores it) at
the commit below. Its core (src/amiga and Musashi, as its cmake/amiga_core.cmake builds them) is built as it is;
amiga_cmm2.c is the platform (what src/pico is on the RP2350 boards): the display, the sound, the keyboard, the mouse,
the joystick and the disks on the loader's services (Peter, 9 Nov 2026: "what about taking the emulator from the other
source").

GPL-3.0, as pico-amiga is (Musashi under its own licence): its source must go with any copy given out (pico-amiga's, at
that commit, and this directory). No Kickstart is built in: the user's own.

    python apps/amiga/make.py
then put out/amiga.app in A:/amiga with a Kickstart (amiga-os-130.rom, Amiga Forever's Kickstart 1.3, or another by
-kick) and disk images (.adf), and at the CMM2's prompt:
    CHDIR "A:/amiga"
    RUN "amiga.app", game.adf
"""
import glob, hashlib, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'pico-amiga')
URL = 'https://github.com/drewpo28/pico-amiga.git'
COMMIT = '46f86e988abd8fd96e20f278b25e99a2984b00bb'
# the ITCM's 24 KB above the firmware's code (build.py --itcm; Peter, 9 Nov 2026: "There is also a bit of itcm ram left for
# really hot routines"): Denise, the line scheduler, the copper, Paula, Musashi's loop and the blitter's two busiest, 21.9
# KB with -funroll-loops (below). Measured on Mental Hangover's frames 1500-2500 (pm_bench, 10 Nov), ms a frame: 17.06 as
# it was; 15.97 with the memory map and the whole blitter in too; 15.82 the memory map left out (each of the 68000's
# reads from the AXI RAM's handlers came through a veneer); 14.51 unrolled, as here
ITCM = ['denise', 'amiga', 'copper', 'paula', 'm68kcpu:m68k_execute', 'blitter:line_blit,blitter_poll']
AXI = ['m68kops', 'm68kcpu', 'amiga', 'blitter', 'cia', 'copper', 'custom', 'denise', 'fb', 'floppy', 'mem', 'paula']
DEFINES = ['M68K_NO_BUS_ERROR_TRAP',            # no setjmp at each entry to the 68000 (patches/0001)
           'AMIGA_ROW_DONE_ALL',                # each finished row to amiga_cmm2.c's row_done (patches/0001)
           'AMIGA_ROW_RING=4',                  # Denise's rows made in a ring of 4 in the AXI RAM (patches/0001), not 288
           'AMIGA_FAST_FETCH',                 # the 68000's opcodes read inline from plain memory (patches/0001)
           'AMIGA_SPRITE_IDX',                 # sprite-covered lowres pixels by the line's index caches (patches/0001)
           'AMIGA_BLIT_STORE',                 # blits store D without reading it first (patches/0001)
           'AMIGA_FAST_SPREAD',                # spread_planes' lowres doubling by lane masks (patches/0001)
           'AMIGA_PF_ALIGN',                   # the playfield buffers 2 past a word: units' stores aligned (patches/0001)
           'AMIGA_BLIT_FAST',                  # the area blit's registers in locals, its common kinds instanced (patches/0001)
           'AMIGA_FAST_MEM',                   # the memory map as three tables, the I/O paths out of line (patches/0001)
           'AMIGA_NO_ADDR_MASK',               # Musashi's 24-bit mask left to the callbacks, which mask again (patches/0001)
           'AMIGA_REV16',                      # big-endian halfwords by LDRH + REV16, no UXTH (patches/0001)
           'AMIGA_PF_EXTENT',                  # a playfield buffer cleared over its last line's writes only (patches/0001)
           'AMIGA_SINGLE_CORE',                # Denise's stages' atomics relaxed: no DMBs (patches/0001; one core)
           'AMIGA_NTSC_OPTION',                 # amiga_config_t's ntsc: an NTSC Agnus (patches/0001; amiga_cmm2.c's default)
           'SLICE_CC=32']                       # the copper and the 68000 take turns each 32 colour clocks, not 16: half the
                                                # entries to Musashi (its amiga.c: what is drawn is exact either way, each
                                                # display write carrying its own beam position); 1 ms a frame less


def patch():
    """the patches applied to the commit's files, once: again (from the commit) only when they change"""
    patches = sorted(glob.glob(os.path.join(HERE, 'patches', '*.patch')))
    want = hashlib.sha1(b''.join(open(p, 'rb').read() for p in patches)).hexdigest()
    stamp = os.path.join(SRC, '.cmm2-patches')
    if os.path.exists(stamp) and open(stamp).read().strip() == want:
        return
    subprocess.run(['git', '-C', SRC, 'checkout', '-q', '--', '.'], check=True)
    for p in patches:
        subprocess.run(['git', '-C', SRC, 'apply', p], check=True)
    open(stamp, 'w').write(want + '\n')


def menu_font():
    """out/uifont.h: pico-amiga's own menu font (src/pico/ui/UiFont.cpp's 6 x 10, 0x20-0x87) for amiga_cmm2.c's disk
    menu, its table as UiFont.cpp has it (the rest of that file is C++, for pico-amiga's own menu)"""
    src = open(os.path.join(SRC, 'src', 'pico', 'ui', 'UiFont.cpp'), encoding='utf-8', errors='replace').read()
    body = src[src.index('ui_font6x10[UI_FONT_COUNT * UI_FONT_H] = {'):]
    body = re.sub(r'//[^\n]*', '', body[body.index('{') + 1:body.index('};')])
    data = [int(v, 16) for v in re.findall(r'0x[0-9A-Fa-f]{2}', body)]
    if len(data) != (0x87 - 0x20 + 1) * 10:
        sys.exit('UiFont.cpp: %d bytes of font, not %d' % (len(data), (0x87 - 0x20 + 1) * 10))
    out = ('/* made by make.py from pico-amiga\'s src/pico/ui/UiFont.cpp: its 6 x 10 menu font, 0x20-0x87, a byte a row (bit 5\n'
           '   the leftmost pixel) */\nstatic const unsigned char ui_font6x10[%d] = {\n' % len(data))
    out += ''.join('    ' + ','.join('0x%02X' % v for v in data[i:i + 20]) + ',\n' for i in range(0, len(data), 20)) + '};\n'
    f = os.path.join(HERE, 'out', 'uifont.h')
    if not os.path.exists(f) or open(f).read() != out:
        open(f, 'w', newline='\n').write(out)


def sources():
    """cmake/amiga_core.cmake's AMIGA_CORE_SRC, and the platform"""
    out = [os.path.join(SRC, 'src', 'amiga', f + '.c') for f in ('amiga', 'blitter', 'cia', 'copper', 'custom', 'denise', 'fb',
                                                                'floppy', 'mem', 'paula', 'rtc', 'state')]
    M = os.path.join(SRC, 'external', 'musashi')
    out += [os.path.join(M, 'm68kcpu.c'), os.path.join(M, 'gen', 'm68kops.c'), os.path.join(M, 'softfloat', 'softfloat.c')]
    out += [os.path.join(HERE, 'amiga_cmm2.c')]
    return out


def main():
    if not os.path.isdir(SRC):                  # (that commit alone: the whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'fetch', '-q', '--depth', '1', URL, COMMIT], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:                          # (first: in a folder with a broken .git, git -C falls through to the CMM2 repository)
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    patch()
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    menu_font()
    listfile = os.path.join(HERE, 'out', 'sources.txt')
    open(listfile, 'w').write('\n'.join(sources()) + '\n')
    M = os.path.join(SRC, 'external', 'musashi')
    inc = [os.path.join(SRC, 'src', 'amiga'), M, os.path.join(M, 'gen'), os.path.join(HERE, 'out')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'amiga.app'),
           '--sources', listfile, '--api', '18', '--g2', '--stack', '0x10000', '--heap', str(2560 << 10)]
    for d in inc:
        cmd += ['-I', d]
    # into the processor's AXI RAM (Peter, 9 Nov 2026: "put the opcode tables and handlers in the 512K of video memory and
    # run the display from sdram"): Musashi's (its handlers, their 128 KB index and 64 KB cycle table) and the chips'
    cmd += ['--axi=' + n for n in AXI]
    itcm = os.environ.get('AMIGA_ITCM')         # (trials: NAME[:f,...] by spaces)
    cmd += ['--itcm=' + n for n in (itcm.split() if itcm is not None else ITCM)]
    # (-funroll-loops: 8% off a frame, Denise's and the platform's pixel loops; -O3 was too big for the ITCM)
    off = os.environ.get('AMIGA_OFF', '').split()   # (trials: DEFINES left out, by name)
    cmd += ['--cflag=-D' + d for d in DEFINES if d.split('=')[0] not in off] + ['--cflag=-std=gnu11', '--cflag=-w', '--cflag=-O2', '--cflag=-funroll-loops']
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
