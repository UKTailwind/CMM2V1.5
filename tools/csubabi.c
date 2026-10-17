/* csubabi.c: every CallTable slot that takes or returns a float or a double, called from a CSUB, and the results
   stored as they come back: doubles in f[], integers and floats' bits in r[], strings in s1 and s2. tools/gen_csubtests.py
   builds it twice into tests/cmm2/csubabi.bas: for the PicoMite (armcfgen's default, Cortex-M0+ soft-float, which finds
   CallTablePS through vector slot 7) and for the CMM2 (--cpu cortex-m7, hard-float: CallTableP through slot 8).
   Nothing here does float arithmetic of its own: on the M0+ that would need libgcc.
   In: f[0] a, f[1] b, f[18] the aspect; r[0] an integer, r[1] and r[2] two floats' bits, r[3] a double's bits. */
#include "PicoCFunctions.h"
typedef union { float f; unsigned int u; } fbits;
static long long FB(float x) { fbits v; v.f = x; return v.u; }
static float BF(long long x) { fbits v; v.u = (unsigned int)x; return v.f; }

void main(long long *r, MMFLOAT *f, unsigned char *s1, unsigned char *s2) {
    MMFLOAT a = f[0], b = f[1];
    long long n = r[0];
    float x = BF(r[1]), y = BF(r[2]);
    f[2] = FMul(a, b);
    f[3] = FAdd(a, b);
    f[4] = FSub(a, b);
    f[5] = FDiv(a, b);
    f[6] = IntToFloat(n);
    f[7] = ((MMFLOAT (*)(unsigned long long))Vector_LoadFloat)((unsigned long long)r[3]);
    f[8] = Sine(a);
    f[9] = Cosine(a);
    f[10] = Sqrt(a);
    f[11] = Atan2(a, b);
    f[12] = Power(a, b);
    f[13] = Log(a);
    f[14] = Tan(a);
    f[15] = StoD(x);
    f[16] = TimerVal();
    f[17] = RndVal();
    r[4] = FCmp(a, b);
    r[5] = FCmp(b, a);
    r[6] = FCmp(a, a);
    r[7] = FloatToInt(a);
    r[8] = FB(SAdd(x, y));
    r[9] = FB(SSub(x, y));
    r[10] = FB(SMul(x, y));
    r[11] = FB(SDiv(x, y));
    r[12] = SCmp(x, y);
    r[13] = SCmp(y, x);
    r[14] = SCmp(x, x);
    r[15] = FB(SSin(x));
    r[16] = FB(SCos(x));
    r[17] = FB(SSqrt(x));
    r[18] = FB(SAtan2(x, y));
    r[19] = FB(SPow(x, y));
    r[20] = FB(DtoS(a));
    r[21] = StoI(x);
    r[22] = FB(ItoS(n));
    FloatToStr((char *)s1, b, 0, 4, ' ');                           // a C string: made an MMBasic one in place
    CtoM(s1);
    StrFormat(s2, a, 0, 0, 0, 6, ' ');                              // an MMBasic string already
    DrawCircle(100, 100, 40, 1, 0xFFFFFF, -1, f[18]);
}
