"""make.py: builds apps/heretic/out/heretic.app, Heretic for the CMM2 (heretic_cmm2.c). frank-heretic is fetched the first time into
apps/heretic/frank-heretic (git ignores it), at the commit below, and its game (src/heretic: Chocolate Heretic on doomgeneric) is built
unaltered, its board's files replaced by heretic_cmm2.c, the front end of apps/doom, and stub/ (a Pico header and psram_malloc, for the
three files of the game that name them). The program is GPL-2.0, as frank-heretic is: its source must go with any copy given out
(frank-heretic's, at that commit, and this directory). The game's data is not part of it: heretic1.wad, the shareware's, or heretic.wad.

    python apps/heretic/make.py
then put out/heretic.app beside the WAD (A:/heretic) and, at the CMM2's prompt: CHDIR "A:/heretic": RUN "heretic.app"
"""
import glob, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'frank-heretic')
URL = 'https://github.com/rh1tech/frank-heretic.git'
COMMIT = 'cdd0e4a580df3472fd8bfa44bf8de84607ce4369'   # 12 May 2026
# of src/heretic/*.c, what the game does not use here: other platforms' sound, music and files, and the main that frank-heretic replaces
LEAVE = re.compile(r'^(i_allegro\w*|i_flmusic|i_musicpack|i_oplmusic|i_pcsound|i_sdl\w*|i_videohr|i_winmusic|i_main|i_input_stubs|'
                   r'i_joystick|d_dedicated|z_native|w_file_posix|w_file_win32|heretic_icon|doomgeneric_\w+|gusconf)\.c$')


def main():
    if not os.path.isdir(SRC):
        subprocess.run(['git', 'clone', '-q', URL, SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', COMMIT], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    game = os.path.join(SRC, 'src', 'heretic')
    srcs = [f for f in sorted(glob.glob(os.path.join(game, '*.c'))) if not LEAVE.match(os.path.basename(f))]
    srcs.append(os.path.join(HERE, 'heretic_cmm2.c'))
    flags = ['-DHERETIC=1', '-DCMAP256', '-DDOOMGENERIC_RESX=320', '-DDOOMGENERIC_RESY=240', '-DFEATURE_SOUND', '-DNUM_SOUND_CHANNELS=16',
             '-DNO_USE_LIBSAMPLERATE', '-DNO_USE_TIMIDITY', '-DNO_USE_GUS', '-DNO_USE_MUSIC_PACKS', '-w']
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'heretic.app')]
    cmd += srcs + ['--api', '8', '--vram', '--stack', '0x20000', '--heap', str(2560 << 10), '-I', game, '-I', os.path.join(HERE, 'stub')]
    cmd += ['--cflag=' + f for f in flags]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
