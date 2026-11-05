/* fwlibm.c: libm from the firmware, for a runtime built --xip (build.py), which ships inside the firmware it runs on
   and so always has services version 11. Its own copy of these 19 would be 19 KB of the firmware's flash spent on
   functions the firmware already has (Peter, 4 Nov 2026), as the PC3's kernel shares its libm with its programs.
   Each is a jump through cmm2->libm, with the arguments left where they are. MIT License, as cmm2img.h. */
#include <math.h>
#include "cmm2.h"

#define F1(f) double f(double x) { return cmm2->libm->f(x); }
#define F2(f) double f(double x, double y) { return cmm2->libm->f(x, y); }

F1(sin) F1(cos) F1(tan) F1(asin) F1(acos) F1(atan) F1(sinh) F1(cosh) F1(tanh) F1(exp) F1(log) F1(log10)
F2(pow) F2(atan2) F2(fmod)

/* floor, ceil, fabs and sqrt are one instruction each on the M7's FPU (FPv5: VRINTM, VRINTP, VABS, VSQRT), cheaper
   than any call: newlib's floor is 61 cycles, VRINTM a few */
#define FPU(f, ins) double f(double x) { double r; __asm__(ins " %P0, %P1" : "=w"(r) : "w"(x)); return r; }
FPU(floor, "vrintm.f64") FPU(ceil, "vrintp.f64") FPU(fabs, "vabs.f64") FPU(sqrt, "vsqrt.f64")
