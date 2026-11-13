/* dlfcn.h for sc-im (apps/scim/compat): a loadable program cannot load another, so dlopen always fails, and sc-im's
   triggers written in C (a .so) say so. Its Lua triggers are not built in either (no XLUA). BSD 4-clause, as sc-im. */
#ifndef CMM2_DLFCN_H
#define CMM2_DLFCN_H
#define RTLD_LAZY   1
#define RTLD_NOW    2
#define RTLD_GLOBAL 4
#define RTLD_LOCAL  0
static inline void *dlopen(const char *file, int mode) { (void)file; (void)mode; return 0; }
static inline void *dlsym(void *handle, const char *name) { (void)handle; (void)name; return 0; }
static inline int dlclose(void *handle) { (void)handle; return 0; }
static inline char *dlerror(void) { return (char *)"the Colour Maximite 2 cannot load a shared library"; }
#endif
