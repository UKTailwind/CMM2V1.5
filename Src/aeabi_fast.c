/***********************************************************************************************************************
aeabi_fast.c - faster versions of run-time helpers the compiler calls.

64-bit integer to double (__aeabi_l2d, __aeabi_ul2d; also named __floatdidf, __floatundidf). The FPv5 FPU converts
32-bit integers only, so libgcc does these in software (its ieee754-df.S, through __adddf3's normalisation): MMBasic
calls them wherever an INTEGER meets a FLOAT. Here the high and low words are converted by the FPU, each exactly,
and added with one rounding, so the result is the correctly rounded one libgcc gives, for every value.
The run-time ABI's helpers use the base (soft-float) calling convention even in a hard-float build: the result goes
back in r0:r1, not d0, hence pcs("aapcs").
***********************************************************************************************************************/
#include <stdint.h>

__attribute__((pcs("aapcs"))) double __aeabi_l2d(long long x) {
    return (double)(int32_t)(x >> 32) * 4294967296.0 + (double)(uint32_t)x;
}

__attribute__((pcs("aapcs"))) double __aeabi_ul2d(unsigned long long x) {
    return (double)(uint32_t)(x >> 32) * 4294967296.0 + (double)(uint32_t)x;
}

__attribute__((pcs("aapcs"))) double __floatdidf(long long x) __attribute__((alias("__aeabi_l2d")));
__attribute__((pcs("aapcs"))) double __floatundidf(unsigned long long x) __attribute__((alias("__aeabi_ul2d")));
