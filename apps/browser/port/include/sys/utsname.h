/* sys/utsname.h for a loadable program (apps/browser): newlib has none. uname is port/cmm2_main.c's. MIT License, as
   Inc/cmm2img.h. */
#ifndef CMM2_SYS_UTSNAME_H
#define CMM2_SYS_UTSNAME_H

#ifdef __cplusplus
extern "C" {
#endif

struct utsname {
    char sysname[32];
    char nodename[32];
    char release[32];
    char version[32];
    char machine[32];
};

int uname(struct utsname *u);

#ifdef __cplusplus
}
#endif
#endif
