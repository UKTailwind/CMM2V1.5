"""port_stepper.py PICOMITE_DIR CMM2_DIR - the PicoMite's io/stepper.c and io/stepper.h rewritten for the CMM2, as
Src/stepper.c and Inc/stepper.h (Peter, 25 Oct 2026; PicoMite 37c5d1c first).

Mechanical changes only, so that a later PicoMite stepper.c can be ported again by running this. What the pico SDK
gave is in Inc/stepper_cmm2.h and Src/stepper_cmm2.c (the CMM2's TIM1 tick, the pins, the interrupt mask, the TMC
transmitter):
  - #ifdef rp2350: the rp2350 branch kept (the stepper is RP2350-only on the PicoMite)
  - the pico SDK's includes replaced by stepper_cmm2.h
  - PinDef[p].GPno -> PinGP(p) (the stepper keeps GP numbers: the 64-bit masks, External.c), PINMAP[gp] -> GPPinOf(gp)
  - __not_in_flash_func(f) -> ITCM_CODE f (the 100 kHz interrupt from ITCM)
  - PWM slice 10, its interrupt and the checks of what else uses it -> StepperTimerStart/Stop/Ack (TIM1 here)
  - getcsargs(x, n) -> getargs(x, n, ","), getpinarg -> GetPinArg, g_OptionBase -> OptionBase
  - unsigned char * -> char * (the CMM2's interpreter API), and the casts of string literals dropped
"""
import re
import sys

pico, cmm2 = sys.argv[1], sys.argv[2]

# The CMM2's tick: 200 kHz (Peter, 25 Oct 2026: a 5 us step pulse, which suits every driver, and steps up to 100 kHz),
# where the PicoMite's is 100 kHz. STEP_ACCUMULATOR_MAX goes up with it so that RATE_SCALE stays 1000: the planner's
# rate constants (STEPPER_TAIL_MIN_STEP_RATE ...) are written in those units. The jerk's smallest step per tick is 1,
# so the smallest jerk the S-curve can use is four times the PicoMite's (ISR_FREQ squared over steps/mm x RATE_SCALE).
ISR_HZ = 200000


def resolve(text, name, keep_first):
    """the #ifdef name blocks: the first branch kept (or the #else one)"""
    out, lines, i = [], text.split('\n'), 0
    while i < len(lines):
        if lines[i].strip() == '#ifdef ' + name:
            depth, first, second = 1, [], []
            cur = first
            i += 1
            while i < len(lines):
                s = lines[i].strip()
                if s.startswith('#if'):
                    depth += 1
                elif s == '#endif':
                    depth -= 1
                    if depth == 0:
                        break
                elif s == '#else' and depth == 1:
                    cur = second
                    i += 1
                    continue
                cur.append(lines[i])
                i += 1
            kept = first if keep_first else second
            if kept:
                out += resolve('\n'.join(kept), name, keep_first).split('\n')
            i += 1
            continue
        out.append(lines[i])
        i += 1
    return '\n'.join(out)


def once(d, old, new, what):
    n = d.count(old)
    if n != 1:
        sys.exit('port_stepper: %s: %d matches' % (what, n))
    return d.replace(old, new)


def common(d):
    d = resolve(d, 'rp2350', True)
    d = re.sub(r'\bPinDef\[([^\]]+)\]\.GPno\b', r'PinGP(\1)', d)
    d = re.sub(r'\bPINMAP\[([^\]]+)\]', r'GPPinOf(\1)', d)
    d = re.sub(r'__not_in_flash_func\((\w+)\)', r'ITCM_CODE \1', d)
    d = re.sub(r'\bgetcsargs\(([^,]+), *([^)]+)\)', r'getargs(\1, \2, ",")', d)
    d = re.sub(r'\bgetpinarg\b', 'GetPinArg', d)
    d = re.sub(r'\bg_OptionBase\b', 'OptionBase', d)
    d = d.replace('(unsigned char *)"', '"')
    d = re.sub(r'\bunsigned char \*', 'char *', d)
    d = re.sub(r'\(unsigned char \*\)', '(char *)', d)
    d = re.sub(r'\bconst char \*\*', 'const char **', d)
    return d


# ---- stepper.h --------------------------------------------------------------------------------------------------------
h = open(pico + '/io/stepper.h', encoding='latin-1').read().replace('\r\n', '\n')
h = once(h, '#include "pico/stdlib.h"\n', '#include "stepper_cmm2.h"                     // (the pico SDK\'s names: the CMM2\'s)\n',
         'stepper.h stdlib')
h = once(h, '#define ISR_FREQ 100000                // 100KHz interrupt frequency (Hz)\n',
         '#define ISR_FREQ %d                // %dKHz interrupt frequency (Hz): the CMM2\'s (tools/port_stepper.py)\n'
         % (ISR_HZ, ISR_HZ // 1000), 'ISR_FREQ')
h = once(h, '#define STEP_ACCUMULATOR_MAX 100000000 // 100M threshold for step timing\n',
         '#define STEP_ACCUMULATOR_MAX %d // %dM threshold for step timing: ISR_FREQ x 1000, RATE_SCALE 1000\n'
         % (ISR_HZ * 1000, ISR_HZ // 1000), 'STEP_ACCUMULATOR_MAX')
k, top = ISR_HZ // 1000, ISR_HZ // 2000                             # (the documentation's figures: the tick, the step cap)
h = h.replace('100kHz', '%dkHz' % k).replace('50kHz', '%dkHz' % top).replace('50000 steps/s', '%d000 steps/s' % top)
h = h.replace('(50000 / steps_per_mm', '(%d000 / steps_per_mm' % top)
h = common(h)
open(cmm2 + '/Inc/stepper.h', 'w', encoding='latin-1', newline='\n').write(
    '/* The PicoMite\'s io/stepper.h, made by tools/port_stepper.py: do not edit, port again. */\n' + h)

# ---- stepper.c --------------------------------------------------------------------------------------------------------
d = open(pico + '/io/stepper.c', encoding='latin-1').read().replace('\r\n', '\n')
d = once(d, '#include "hardware/pwm.h"\n#include "hardware/clocks.h"\n#include "hardware/gpio.h"\n#include "hardware/timer.h"\n'
            '#include <stdarg.h>\n#include <stdlib.h>\n#include <hardware/sync.h>\n',
         '#include <stdarg.h>\n#include <stdlib.h>\n', 'the SDK includes')
d = once(d, '#include "pico/stdlib.h"\n#include "hardware/clocks.h"\n#include "hardware/sync.h"\n#include "hardware/gpio.h"\n'
            '#include "hardware/structs/systick.h"\n', '', 'the TMC part\'s SDK includes')

# the 100 kHz tick: PWM slice 10's wrap interrupt there, TIM1's update interrupt here (stepper_cmm2.c)
d = once(d, '    pwm_set_enabled(STEPPER_PWM_SLICE, false);\n    pwm_set_irq1_enabled(STEPPER_PWM_SLICE, false);\n'
            '    irq_set_enabled(PWM_IRQ_WRAP_1, false);\n', '    StepperTimerStop();\n', 'the timer\'s stop')
# STEPPER INIT: FAST TIMER and PWM slice 10's other users (no conflict here: TIM1 is the stepper's alone)
a = d.index('        // Check for conflict with EXT_FAST_TIMER (uses same IRQ)\n')
end = 'stepper_error("Stepper timer slice in use");\n        }\n#endif\n'
b = d.index(end, a) + len(end)
d = d[:a] + d[b:]
# the retired arc buffers' check: the CMM2's two heaps (stepper_cmm2.c), not the PicoMite's heap and PSRAM
a = d.index('static inline bool stepper_retired_node_ok(const unsigned char *p)\n{\n')
a = d.index('{\n', a) + 2
b = d.index('\n}\n', a) + 1
d = d[:a] + '    return StepperHeapBlock(p); /* (a page of either heap here: stepper_cmm2.c) */\n' + d[b:]
# STEPPER INIT: the timer's start
a = d.index('        // Set up PWM slice 10 for 100KHz interrupt')
end = '        pwm_set_enabled(STEPPER_PWM_SLICE, true);\n'
b = d.index(end, a) + len(end)
mid = d[a:b]
keep = mid[mid.index('        // Start disarmed'):mid.index('        // Clear any pending interrupt')]
d = d[:a] + keep + '        StepperTimerStart();                          // (TIM1 at 100 kHz, NVIC level 0: stepper_cmm2.c)\n' + d[b:]
d = once(d, '    pwm_clear_irq(STEPPER_PWM_SLICE);\n', '    StepperTimerAck();\n', 'the ISR\'s acknowledge')

# TMC22xx's transmitter: SysTick ticks there; the CMM2's serialtx takes the baud rate and times the bits with the
# cycle counter, interrupts off across each frame (stepper_cmm2.c)
d, n = re.subn(r'( *)uint32_t clk_hz = clock_get_hz\(clk_sys\);\n *int bittime = 0x00FFFFFFu \+ 12 - \(int\)\(clk_hz / TMC_BAUD\);\n',
               r'\1int bittime = TMC_BAUD; /* (the CMM2\'s serialtx takes the baud rate: stepper_cmm2.c) */\n', d)
if n != 2:
    sys.exit('port_stepper: the TMC bit times: %d' % n)
d, n = re.subn(r'\n( *)(/\* ---- Make sure SysTick[^\n]*\n *)?systick_hw->rvr = 0x00FFFFFFu;\n *systick_hw->cvr = 0;\n'
               r' *systick_hw->csr = 0x5;[^\n]*\n', '\n', d)
if n != 2:
    sys.exit('port_stepper: the TMC SysTick set-ups: %d' % n)
d = d.replace('/* ---- Bit time in SysTick ticks at the *current* clk_sys ---- */', '/* ---- The bit time ---- */')

d = d.replace('"Stepper initialized - 100KHz timer active\\r\\n"', '"Stepper initialized - %dKHz timer active\\r\\n"' % (ISR_HZ // 1000))
d = d.replace('// (TIM1 at 100 kHz, NVIC level 0: stepper_cmm2.c)', '// (TIM1 at ISR_FREQ, NVIC level 0: stepper_cmm2.c)')
d = common(d)
d = once(d, 'extern void serialtx(uint64_t gppin, char *string, int bittime);', 'extern void serialtx(uint64_t gppin, unsigned char *string, int bittime);', "serialtx's bytes")
open(cmm2 + '/Src/stepper.c', 'w', encoding='latin-1', newline='\n').write(
    '/* The PicoMite\'s io/stepper.c, made by tools/port_stepper.py: do not edit, port again. */\n' + d)
print('Src/stepper.c and Inc/stepper.h written')
