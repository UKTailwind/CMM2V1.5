/* HdmiAudio.c: a G3's sound through HDMI, beside the DACs' (docs/G3_HDMI_Audio_Plan.md, step 2: 14 Nov 2026). Every
   sound goes to both, always: there is nothing to choose (Peter, 8 Nov).

   - SAI2 block A is an I2S master transmitter on PD11 (SD), PD12 (FS) and PD13 (SCK) to the MS7210, clocked by pll1_q
     (48 MHz on a board at 480 MHz) divided by 17: 64 bit clocks a frame, 16-bit samples in 32-bit slots, 44,117.6 Hz,
     0.04% fast, inside consumer audio's 0.1%. It runs from the start-up on, sending silence between sounds: a screen
     that loses the sound can take a second to find it again. The chip's side is MS7210.c's (ms_audio).
   - DMA1 stream 1 (DMAMUX1 channel 1, SAI2_A; stream 0 is the DACs' for a program's stream) sends a circular buffer of
     two halves of HA_HALF frames, at the top of the D2 SRAM (the 4 KB above lbuff, Memory.h: DMA1 reaches neither
     TCM); its half and full interrupts make the half just sent again (ha_fill), cleaned from the data cache.
   - The sources: TIM4's interrupt (Audio_Interrupt, stm32h7xx_it.c) puts each tick's pair in a ring (HdmiPut,
     Hardware_Includes.h, inline so that it is in ITCM with the interrupt). ha_fill takes the ring at the SAI's rate: as
     it is when TIM4 runs at that rate (the 44.1 kHz sources, whose prescaler HdmiPsc locks to the SAI: both clocks are
     PLL1's, so they never drift), or by linear interpolation at the exact ratio of the two clocks for other rates, with
     a nudge of 0.05% if the ring's fill wanders. The ring is let fill to a mark before it plays, so the interrupt's
     jitter never empties it.
   - A loadable program's stream (Loader.c) is played by DMA with no TIM4 interrupt: its 16-bit frames are kept in
     HdmiStream, the same size as the DACs' buffer, and taken at the DACs' DMA's own position, half a buffer of ours
     ahead, as the two run at one rate (Loader.c's TIM4 prescaler is HdmiPsc's).
   - When the sound runs out the output decays to silence over a few milliseconds, not a step (a click).
   - All here is in .extra, at the end of .text: new code put among .text's moves the firmware's hot loops in the
     I-cache, which slowed other statements by 5-8% (11 Nov 2026). */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"

#define EXTRA __attribute__((section(".extra")))
#define PSpeedDiv (PeripheralBusSpeed / 2)                          // (Audio.c's: TIM4's count rate, its period being 2)
#define HA_HALF   128                                               // frames in a half of the DMA buffer: 2.9 ms
#define HA_BUF    ((uint32_t *)0x30047000)                          // the DMA buffer: 2 x HA_HALF frames, 1 KB, then the
                                                                    // ring, HDMI_RINGN frames, 3 KB, to the D2 SRAM's end
volatile uint32_t HdmiRingW, HdmiRingR;                             // the ring's next write (HdmiPut) and next read (ha_fill)
uint32_t HdmiRate;                                                  // the SAI's rate (Hz): 0 if it is not running
uint32_t HdmiK;                                                     // TIM4's prescaler + 1 for the SAI's rate, if a whole one
uint32_t *volatile HdmiStream;                                      // a program's stream's 16-bit frames (Loader.c), or NULL
int HdmiStreamFrames;                                               // its frames: the DACs' DMA buffer's
static uint32_t k16;                                                // HdmiK in 16.16
static int playing;                                                 // the ring is being played (it had reached its mark)
static int target;                                                  // the ring's fill when it started
static uint32_t phase;                                              // 0.32: where between the ring's frame and the next
static uint64_t step;                                               // the ring's frames a SAI frame, 32.32
static uint32_t stepfor = ~0u;                                      // TIM4's PSC and ARR that step is for
static uint32_t wwas;                                               // the ring's write at the last fill
static int32_t lastl, lastr;                                        // the last frame sent
static int streampos = -1;                                          // where in HdmiStream, -1 to find

static void EXTRA ha_fill(uint32_t *d) {
    int i = 0;
    uint32_t *s = HdmiStream;
    if(s && CurrentlyPlaying == P_STREAM && (DMA1_Stream0->CR & DMA_SxCR_EN)) {   // a program's stream
        const int n = HdmiStreamFrames;
        int want = n - (int)DMA1_Stream0->NDTR + HA_HALF, off;      // the DACs' frame now, and the half sent before ours
        if(want >= n) want -= n;
        off = streampos - want;
        if(off < 0) off += n;
        if(streampos < 0 || (off > HA_HALF && off < n - HA_HALF)) streampos = want;   // (found, or lost: 3 ms out)
        for(; i < HA_HALF; i++) {
            d[i] = s[streampos];
            if(++streampos == n) streampos = 0;
        }
        lastl = (int16_t)d[HA_HALF - 1];
        lastr = (int32_t)d[HA_HALF - 1] >> 16;
        HdmiRingR = wwas = HdmiRingW;                               // (the ring kept empty meanwhile)
        playing = 0;
    } else {
        uint32_t w = HdmiRingW, r = HdmiRingR, psc = TIM4->PSC | TIM4->ARR << 16;
        int fill = (int)w - (int)r, need, mark;
        uint64_t st;
        streampos = -1;
        if(fill < 0) fill += HDMI_RINGN;
        if(psc != stepfor) {                                        // TIM4's rate / the SAI's = 2K / ((PSC + 1) (ARR + 1))
            stepfor = psc;
            step = ((uint64_t)k16 << 17) / ((uint64_t)(TIM4->PSC + 1) * (TIM4->ARR + 1));
        }
        need = (int)((step * HA_HALF + phase) >> 32) + 1;           // the ring's frames a half takes
        mark = need + 96;                                           // played from this fill on: 2 ms of jitter either way
        st = step;
        if(!playing) {
            if(fill >= mark || (fill && w == wwas)) { playing = 1; phase = 0; target = fill; }   // (or a short sound,
        } else if(w != wwas) {                                      // ended) TIM4 still running: a nudge if the fill wanders
            if(fill > target + 64) st += step >> 11;                // from where it started (never at the 44.1 kHz rates,
            else if(fill < target - 64) st -= step >> 11;           // TIM4's being the SAI's: their frames go as they are)
        }
        wwas = w;
        if(playing) {
            for(; i < HA_HALF; i++) {
                uint32_t a, b;
                int32_t al, ar, adv;
                uint64_t p;
                if(fill < 1 || (fill < 2 && phase)) { playing = 0; break; }   // dry
                a = HDMI_RING[r];
                al = (int16_t)a;
                ar = (int32_t)a >> 16;
                if(phase) {                                         // between a and the next: linear, 15-bit fraction
                    int32_t f = (int32_t)(phase >> 17);
                    b = HDMI_RING[r + 1 < HDMI_RINGN ? r + 1 : 0];
                    al += (((int32_t)(int16_t)b - al) * f) >> 15;
                    ar += ((((int32_t)b >> 16) - ar) * f) >> 15;
                }
                d[i] = (uint16_t)al | (uint32_t)ar << 16;
                lastl = al;
                lastr = ar;
                p = (uint64_t)phase + st;
                phase = (uint32_t)p;
                adv = (int32_t)(p >> 32);
                if(adv > fill) adv = fill;
                fill -= adv;
                r += adv;
                if(r >= HDMI_RINGN) r -= HDMI_RINGN;
            }
            HdmiRingR = r;
        }
    }
    for(; i < HA_HALF; i++) {                                       // nothing (more): towards silence, 1/64 a frame
        lastl -= lastl / 64 ? lastl / 64 : (lastl > 0) - (lastl < 0);
        lastr -= lastr / 64 ? lastr / 64 : (lastr > 0) - (lastr < 0);
        d[i] = (uint16_t)lastl | (uint32_t)lastr << 16;
    }
    SCB_CleanDCache_by_Addr(d, HA_HALF * 4);                        // (the DMA reads the SRAM, not the cache)
}

void EXTRA DMA1_Stream1_IRQHandler(void) {
    const uint32_t f = DMA1->LISR;
    DMA1->LIFCR = f & (0x3DUL << 6);                                // (stream 1's flags)
    if(f & DMA_LISR_HTIF1) ha_fill(HA_BUF);                         // the first half sent: made again
    if(f & DMA_LISR_TCIF1) ha_fill(HA_BUF + HA_HALF);
}

/* a G3's iconvert and i2convert (Audio.c): each sample with PLAY VOLUME applied in 16 bits (at 100 the decoder's as it
   is), ramped as theirs, for HDMI, from which the interrupt makes the DACs' values (HDMI_DAC, HDMI_DAC2: the same as a
   G2's, but for a unit at other volumes). i2convert's were at half level for PLAY EFFECT to be added: here they are at
   full level, the DACs' halved in the interrupt and Loader.c, and HDMI's sum clipped (HdmiPut). ramped is iconvert's
   rule: the volumes looked at only while they move */
void EXTRA HdmiConvert(int16_t *o, const int16_t *s, int count, int ramped) {
    int l = mapping[vol_left], r = mapping[vol_right], ramp = 0;
    for(int i = 0; i < count; i += 2) {
        if(++ramp >= 11 && (!ramped || vol_left != vol_target_left || vol_right != vol_target_right)) {   // (Audio.c's
            ramp = 0;                                                                                     // VOL_RAMP_INTERVAL)
            ramp_volume();
            l = mapping[vol_left];
            r = mapping[vol_right];
        }
        o[i] = (int16_t)((s[i] * l) / 2000);
        o[i + 1] = (int16_t)((s[i + 1] * r) / 2000);
    }
}

/* TIM4's prescaler for a sample rate: on a G3 the 44.1 kHz family's (44.1, 22.05 and 11.025 kHz) is the SAI's rate
   or a half or a quarter of it exactly, so the ring is sent as it is; otherwise, and on a G1 or a G2, psc, as the
   firmware sets it */
uint32_t EXTRA HdmiPsc(uint32_t rate, uint32_t psc) {
    if(HdmiK && rate) {
        uint32_t k = (44100 + rate / 2) / rate;
        if((k == 1 || k == 2 || k == 4) && rate * k >= 44090 && rate * k <= 44110) return HdmiK * k - 1;
    }
    return psc;
}

/* a G3's start-up (MS7210_Start, the chip found): SAI2 and its DMA set going, sending silence. HdmiRate stays 0 (no
   HDMI sound, the DACs' as a G2's) if pll1_q gives no rate near 44.1 kHz (a board at 400 MHz, whose USB takes HSI48) */
void EXTRA HdmiAudioStart(void) {
    PLL1_ClocksTypeDef pll;
    GPIO_InitTypeDef g = {0};
    uint32_t q, div, rate;
    HdmiRate = HdmiK = 0;
    if(!(RCC->PLLCFGR & RCC_PLLCFGR_DIVQ1EN)) return;
    HAL_RCCEx_GetPLL1ClockFreq(&pll);
    q = pll.PLL1_Q_Frequency;
    div = (q + 64 * 44100 / 2) / (64 * 44100);                      // SCK = q / div, 64 to a frame
    if(div < 1 || div > 63) return;
    rate = q / div / 64;
    if(rate < 44100 - 220 || rate > 44100 + 220) return;            // (0.5%)
    k16 = (uint32_t)((((uint64_t)PSpeedDiv * div * 64) << 16) / q);
    HdmiK = (k16 & 0xFFFF) ? 0 : k16 >> 16;                         // 2720 at 480 MHz: 120 MHz / 2720 = 48 MHz / 17 / 64

    memset(HA_BUF, 0, 2 * HA_HALF * 4);
    SCB_CleanDCache_by_Addr(HA_BUF, 2 * HA_HALF * 4);
    HdmiRingW = HdmiRingR = wwas = 0;
    playing = 0;
    lastl = lastr = 0;
    HdmiStream = NULL;

    RCC->D2CCIP1R = (RCC->D2CCIP1R & ~RCC_D2CCIP1R_SAI23SEL) | (0UL << RCC_D2CCIP1R_SAI23SEL_Pos);   // pll1_q (a field
    __HAL_RCC_SAI2_CLK_ENABLE();                                    // at a time: SAI1SEL is the video's, MS7210.c)
    __HAL_RCC_GPIOD_CLK_ENABLE();
    g.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;                // SD_A, FS_A, SCK_A
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_MEDIUM;
    g.Alternate = GPIO_AF10_SAI2;
    HAL_GPIO_Init(GPIOD, &g);
    SAI2_Block_A->CR1 = 0;
    for(int i = 0; i < 100000 && (SAI2_Block_A->CR1 & SAI_xCR1_SAIEN); i++) {}
    SAI2_Block_A->CR2 = SAI_xCR2_FTH_0;                             // (a DMA request while the FIFO is under a quarter)
    SAI2_Block_A->FRCR = 63 | (31UL << 8) | SAI_xFRCR_FSDEF | SAI_xFRCR_FSOFF;   // 64 bits, FS 32 (low: left), a bit early
    SAI2_Block_A->SLOTR = (2UL << 6) | (1UL << 8) | (3UL << 16);   // 32-bit slots, two, both on, data at their start
    SAI2_Block_A->CR1 = (4UL << 5) | SAI_xCR1_CKSTR | SAI_xCR1_NODIV | (div << SAI_xCR1_MCKDIV_Pos) | SAI_xCR1_DMAEN;
                                                                    // 16-bit data, out on SCK's falling edge, no MCLK
    __HAL_RCC_DMA1_CLK_ENABLE();
    DMA1_Stream1->CR = 0;
    while(DMA1_Stream1->CR & DMA_SxCR_EN) {}
    DMA1->LIFCR = 0x3DUL << 6;
    DMAMUX1_Channel1->CCR = DMA_REQUEST_SAI2_A;
    DMA1_Stream1->PAR = (uint32_t)&SAI2_Block_A->DR;
    DMA1_Stream1->M0AR = (uint32_t)HA_BUF;
    DMA1_Stream1->NDTR = 2 * 2 * HA_HALF;                           // half-words: left, right
    DMA1_Stream1->FCR = 0;                                          // (direct: a half-word a request)
    DMA1_Stream1->CR = DMA_SxCR_PL_1 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0 | DMA_SxCR_MINC | DMA_SxCR_CIRC |
                       DMA_SxCR_DIR_0 | DMA_SxCR_TCIE | DMA_SxCR_HTIE;
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, IRQ_PRI(1), 0);         // under TIM4's (and the DACs' DMA's)
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    DMA1_Stream1->CR |= DMA_SxCR_EN;
    SAI2_Block_A->CR1 |= SAI_xCR1_SAIEN;
    HdmiRate = rate;
}

