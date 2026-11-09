/* config.h: what atari800's configure would write for the Colour Maximite 2 (apps/atari800, Peter, 9 Nov 2026). It is a port of its own, as
   atari_basic.c or sdl/ are: a8_cmm2.c is the platform (the display, the keys, the joysticks, the sound and the time), on the loader's services.
   The C library is newlib's with the SDK's syscalls.c: files, directories (dirent.h), stat, getcwd and gettimeofday work, with MMBasic's names. */
#ifndef CONFIG_H
#define CONFIG_H
#include <sys/types.h>                          /* (before newlib's dirent.h, which needs its ssize_t: sysrom.c includes that first) */

#define PACKAGE_VERSION "7.2.1"
#define DEFAULT_CFG_NAME "atari800.cfg"          /* in the folder it was started in */
#define SYSTEM_WIDE_CFG_FILE "atari800.cfg"

/* the C library */
#define STDC_HEADERS 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_DIRENT_H 1
#define HAVE_OPENDIR 1
#define HAVE_ATEXIT 1
#define HAVE_FFLUSH 1
#define HAVE_FLOOR 1
#define HAVE_FSTAT 1
#define HAVE_GETCWD 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_LOCALTIME 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_MKDIR 1
#define HAVE_MODF 1
#define HAVE_RENAME 1
#define HAVE_REWIND 1
#define HAVE_RMDIR 1
#define HAVE_SNPRINTF 1
#define HAVE_STAT 1
#define HAVE_STRCASECMP 1
#define HAVE_STRCHR 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRNCPY 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_STRTOL 1
#define HAVE_TIME 1
#define HAVE_UNLINK 1
#define HAVE_VPRINTF 1
#define HAVE_VSNPRINTF 1

/* the emulator: configure's defaults, but for the monitor's extras, the recorders, the PBI devices, the 80-column cards and the R: device,
   which a CMM2 has no use for (or nothing to connect them to) */
#define NEW_CYCLE_EXACT 1
#define CRASH_MENU 1
#define EMUOS_ALTIRRA 1                         /* AltirraOS and Altirra BASIC, built in: used when the folder has no Atari ROMs (Peter) */
#define SOUND 1
#define STEREO_SOUND 1
#define CONSOLE_SOUND 1
#define INTERPOLATE_SOUND 1
#define NONLINEAR_MIXING 1
#define POKEYSND_SIGNED_SAMPLES 1

/* the platform's own (a8_cmm2.c) */
#define SUPPORTS_PLATFORM_TIME 1
#define SUPPORTS_PLATFORM_SLEEP 1
#define SUPPORTS_PLATFORM_PALETTEUPDATE 1

#endif
