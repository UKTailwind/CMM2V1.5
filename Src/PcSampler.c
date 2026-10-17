/***********************************************************************************************************************
PcSampler.c - a PC sampler for finding where the interpreter spends its time (the PicoMite's OPTION PROFILING ON,
SAMPLE, as a diagnostic build: the "Sampler" CMake preset defines PCSAMPLER; a Release build has none of it).

TIM15 interrupts every 97 us (an odd period, so the samples do not lock to the 1 ms tick) at a high priority, so the
interrupt handlers are sampled too.  The handler takes the interrupted PC from the exception frame and counts it,
and the program line being run (CurrentLinePtr), in two open-addressing tables on the heap.  RUN starts it (after the
heap is cleared) and the end of the program prints both tables as the PicoMite does ([PCS] and [PCSLINE] lines), so
the PicoMite's Bas/bench/pcs_report.py resolves the PCs against this build's ELF.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"

#ifdef PCSAMPLER
#define PCS_PERIOD_US 97
#define PCS_ENTRIES 4096                                            // per table, a power of two
typedef struct { uint32_t key, n; } pcs_ent_t;
static pcs_ent_t *pcs_pc = NULL, *pcs_line = NULL;                  // PC -> samples, CurrentLinePtr -> samples
static volatile uint32_t pcs_samples, pcs_dropped, pcs_dropped_line;

static inline void pcs_add(pcs_ent_t *t, uint32_t key, volatile uint32_t *dropped) {
    uint32_t mask = PCS_ENTRIES - 1, h = (key * 2654435761u) >> 8;
    for(int probe = 0; probe < 32; probe++, h++) {
        pcs_ent_t *e = &t[h & mask];
        if(e->key == key) { e->n++; return; }
        if(e->n == 0) { e->key = key; e->n = 1; return; }
    }
    (*dropped)++;
}

void __attribute__((used)) pcs_isr_c(uint32_t *frame) {
    TIM15->SR = ~TIM_SR_UIF;
    if(pcs_pc == NULL) return;
    pcs_samples++;
    pcs_add(pcs_pc, frame[6] & ~1u, &pcs_dropped);                 // the stacked PC (r0-r3, r12, lr, pc, xpsr)
    pcs_add(pcs_line, (uint32_t)CurrentLinePtr, &pcs_dropped_line);
}

// everything runs on MSP, so SP at entry is the exception frame; EXC_RETURN is kept on the stack and popped into PC
void __attribute__((naked)) TIM15_IRQHandler(void) {
    __asm volatile(
        "mov r0, sp        \n"
        "push {r4, lr}     \n"
        "bl pcs_isr_c      \n"
        "pop {r4, pc}      \n");
}

static void PcsStop(void) {
    TIM15->CR1 = 0;
    TIM15->DIER = 0;
    NVIC_DisableIRQ(TIM15_IRQn);
}

// ClearRuntime, before the heap is wiped: the tables are in it
void PcsAbandon(void) {
    PcsStop();
    pcs_pc = pcs_line = NULL;
}

// RUN: the heap has just been cleared, so the tables are new
void PcsStart(void) {
    uint32_t clk = HAL_RCC_GetPCLK2Freq();
    PcsStop();
    pcs_pc = GetMemory(PCS_ENTRIES * sizeof(pcs_ent_t));
    pcs_line = GetMemory(PCS_ENTRIES * sizeof(pcs_ent_t));
    pcs_samples = pcs_dropped = pcs_dropped_line = 0;
    if(RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) clk *= 2;                  // (an APB prescaler doubles the timers' clock)
    __HAL_RCC_TIM15_CLK_ENABLE();
    TIM15->CR1 = 0;
    TIM15->PSC = clk / 1000000 - 1;                                 // 1 MHz
    TIM15->ARR = PCS_PERIOD_US - 1;
    TIM15->CNT = 0;
    TIM15->EGR = TIM_EGR_UG;
    TIM15->SR = 0;
    TIM15->DIER = TIM_DIER_UIE;
    NVIC_SetPriority(TIM15_IRQn, 1);
    NVIC_EnableIRQ(TIM15_IRQn);
    TIM15->CR1 = TIM_CR1_CEN;
}

// the line a CurrentLinePtr is in (its header names it), or -1
static int PcsLineNumber(uint32_t p) {
    char *lp = (char *)p;
    if(lp == NULL || lp < (char *)ProgMemory || lp >= (char *)ProgMemory + PROG_FLASH_SIZE || *lp != T_NEWLINE) return -1;
    return LHDR_LINE(lp);
}

static void PcsPrintTable(pcs_ent_t *t, int lines) {
    char buf[64];
    for(;;) {
        uint32_t best = 0;
        int bi = -1;
        for(int k = 0; k < PCS_ENTRIES; k++)
            if(t[k].n > best) { best = t[k].n; bi = k; }
        if(bi < 0) break;
        if(lines) sprintf(buf, "[PCSLINE] %d %u\r\n", PcsLineNumber(t[bi].key), (unsigned)best);
        else sprintf(buf, "[PCS] %08x %u\r\n", (unsigned)t[bi].key, (unsigned)best);
        MMPrintString(buf);
        t[bi].n = 0;
    }
}

// the end of the program: both tables, the busiest first
void PcsReport(void) {
    char buf[96];
    PcsStop();
    if(pcs_pc == NULL) return;
    sprintf(buf, "\r\n[PCS] samples=%u dropped=%u dropped_line=%u entries=%u\r\n",
            (unsigned)pcs_samples, (unsigned)pcs_dropped, (unsigned)pcs_dropped_line, (unsigned)PCS_ENTRIES);
    MMPrintString(buf);
    PcsPrintTable(pcs_pc, 0);
    PcsPrintTable(pcs_line, 1);
    pcs_pc = pcs_line = NULL;                                       // (heap memory: the next RUN clears it)
}
#endif
