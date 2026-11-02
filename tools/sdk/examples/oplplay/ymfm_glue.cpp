// ymfm_glue.cpp: ymfm's YMF262 (OPL3) behind three C functions, for oplplay.c (tools/sdk). MIT License, as
// cmm2img.h; ymfm itself is BSD-3 (ymfm/LICENSE). One call of ymfm_one is one sample at the chip's own rate
// (14,318,180 / 288 = 49,716 Hz); outputs A and B are left and right.
#include "ymfm_opl.h"

namespace {
struct chip_t {
    ymfm::ymfm_interface intf;
    ymfm::ymf262 opl;
    chip_t() : opl(intf) { opl.reset(); }
};
}

extern "C" void *ymfm_create(void) { return new chip_t(); }

extern "C" void ymfm_write(void *p, int reg, int val) {
    chip_t *c = static_cast<chip_t *>(p);
    if(reg & 0x100) c->opl.write_address_hi(reg & 0xFF);
    else c->opl.write_address(reg & 0xFF);
    c->opl.write_data(val);
}

extern "C" void ymfm_one(void *p, int32_t lr[2]) {
    chip_t *c = static_cast<chip_t *>(p);
    ymfm::ymf262::output_data out;
    c->opl.generate(&out, 1);
    lr[0] = out.data[0];
    lr[1] = out.data[1];
}
