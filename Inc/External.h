/***************************************************************************
CMM2 MMBasic
External.h

Define the MMBasic commands for reading and writing to the digital and analog input/output pins

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

------------------------------------------------------------------------------
  * In addition the software components from STMicroelectronics are provided
  * subject to the license as detailed below:
------------------------------------------------------------------------------
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
*******************************************************************************/




#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef EXTERNAL_HEADER
#define EXTERNAL_HEADER

#define NBR_PULSE_SLOTS     5                       // number of concurrent pulse commands, each entry is 8 bytes


extern char *InterruptReturn;
extern int check_interrupt(void);
extern void ClearExternalIO(void);


/****************************************************************************************************************************
New, more portable, method of manipulating an I/O pin
*****************************************************************************************************************************/

// the basic functions
// set or clear bits in the pin's sfr registers

// some useful defines
#define PinOpenCollectorOn(x)   PinSetBit(x, ODCSET)
#define PinOpenCollectorOff(x)  PinSetBit(x, ODCCLR)
#define PinHigh(x)              PinSetBit(x, LATSET)
#define PinLow(x)               PinSetBit(x, LATCLR)
#define PinSetOutput(x)         PinSetBit(x, TRISCLR)
#define PinSetInput(x)          PinSetBit(x, TRISSET)

// Define the offsets from the PORT address
#define ANSEL               -8
#define ANSELCLR            -7
#define ANSELSET            -6
#define ANSELINV            -5
#define TRIS                -4
#define TRISCLR             -3
#define TRISSET             -2
#define TRISINV             -1
//#define PORT                0

#define PORTCLR             1
#define PORTSET             2
#define PORTINV             3
#define LAT                 4
#define LATCLR              5
#define LATSET              6
#define LATINV              7
#define ODC                 8
#define ODCCLR              9
#define ODCSET              10
#define ODCINV              11
#define CNPU                12
#define CNPUCLR             13
#define CNPUSET             14
#define CNPUINV             15
#define CNPD                16
#define CNPDCLR             17
#define CNPDSET             18
#define CNPDINV             19
#define CNCON               20
#define CNCONCLR            21
#define CNCONSET            22
#define CNCONINV            23
#define CNEN                24
#define CNENCLR             25
#define CNENSET             26
#define CNENINV             27
#define CNSTAT              28
#define CNSTATCLR           29
#define CNSTATSET           30
#define CNSTATINV           31


#define EXT_NOT_CONFIG          0
#define EXT_ANA_IN				1
#define EXT_DIG_IN				2
#define EXT_FREQ_IN				3
#define EXT_PER_IN				4
#define EXT_CNT_IN				5
#define EXT_INT_HI				6
#define EXT_INT_LO				7
#define EXT_DIG_OUT				8
#define EXT_OC_OUT				9
#define EXT_INT_BOTH			10
#define EXT_COM_RESERVED        100                 // this pin is reserved and SETPIN and PIN cannot be used
#define EXT_BOOT_RESERVED       101                 // this pin is reserved at bootup and cannot be used
#define EXT_DS18B20_RESERVED    102                 // this pin is reserved for DS18B20 and cannot be used

extern int ExtCurrentConfig[];
extern volatile int INT0Count, INT0Value, INT0InitTimer, INT0Timer;
extern volatile int INT1Count, INT1Value, INT1InitTimer, INT1Timer;
extern volatile int INT2Count, INT2Value, INT2InitTimer, INT2Timer;
extern volatile int INT3Count, INT3Value, INT3InitTimer, INT3Timer;
extern volatile int INT4Count, INT4Value, INT4InitTimer, INT4Timer;
extern volatile uint64_t INT5Count, INT5Value, INT5InitTimer, INT5Timer;
extern void PinSetBit(int pin, unsigned int offset);
extern int PinRead(int pin);
extern volatile unsigned int GetPinStatus(int pin);
extern int GetPinBit(int pin);
extern void WriteCoreTimer(unsigned long timeset);
extern unsigned long ReadCoreTimer(void);
extern unsigned long readusclock(void);
extern void writeusclock(unsigned long timeset);
extern void initExtIO(void);
extern void ExtCfg(int pin, int cfg, int option) ;
extern void ExtSet(int pin, long long int val);
extern int64_t ExtInp(int pin);
extern int IsInvalidPin(int pin);
extern int GetPinArg(char *p);                  // a pin argument: a number, or with OPTION PICOMITE a GP name
extern int GetPinArgN(char *p);                 // the same, checked as getint(p, 1, NBRPINS) checks a number
extern int PinArgGP(char *p, int *pin);         // the same, giving the GP number (-1: a number) and the pin (0: none)
extern int PinGP(int pin);                      // a pin's GP number (GPPin[]), -1 if it has none
// the PicoMite's GPIOs as one 64-bit word, bit n GPn on the pin GPPin[] gives it (the pico SDK's names: External.c)
extern void gpio_put_masked64(uint64_t mask, uint64_t value);
extern void gpio_put(uint32_t gp, int value);
extern void gpio_set_dir(uint32_t gp, int out);
extern void gpio_set_mask64(uint64_t mask);
extern void gpio_clr_mask64(uint64_t mask);
extern void gpio_xor_mask64(uint64_t mask);
extern uint64_t gpio_get_all64(void);
extern uint64_t gpio_get_out_level_all64(void);
extern int GPPinOf(int gp);                     // a GP's pin
extern int PinFnArg(char *p, MMFLOAT *f, long long int *i64, char **s);   // PIN('s argument, evaluated
extern int p100interrupts[];
extern unsigned long ReadCount5(void);
extern void WriteCount5(unsigned long timeset);
extern volatile unsigned char ADCbits[];

// for CheckPin() action can be set to:
#define CP_CHECKALL          0b0000     // abort with an error if invalid, in use or reserved
#define CP_NOABORT           0b0001     // the function will not abort with an error
#define CP_IGNORE_INUSE      0b0010     // the function will ignore pins that are in use (but not including reserved pins)
#define CP_IGNORE_RESERVED   0b0100     // the function will ignore reserved pins (EXT_COM_RESERVED and EXT_BOOT_RESERVED)
#define CP_IGNORE_BOOTRES    0b1000     // the function will ignore the boot reserved pins (EXT_BOOT_RESERVED)
extern int MIPS16 CheckPin(int pin, int action);

// What check_interrupt must do after each statement (the PicoMite's S1c): .any is one load, zero meaning no interrupt
// can be due and the scan is skipped. .count: interrupts signalled by their source (IntSignal: an ISR, a callback or a
// command) and not yet scanned for; each scan consumes one, so interrupts that arrive together are served by
// consecutive statements. .poll: INT_POLL_SCAN while an armed source has no signal of its own: scan every statement.
#define INT_POLL_SCAN 1
typedef union {
    uint32_t any;
    struct {
        uint16_t count;
        uint8_t poll;
        uint8_t spare;
    };
} IntReady_t;
extern volatile IntReady_t IntReady;
extern void IntSignal(void);
#define IntPoll() (IntReady.poll |= INT_POLL_SCAN)                  // a source with no signal of its own was armed
typedef enum
{
  GPIO_Mode_IN   = 0x00, /*!< GPIO Input Mode */
  GPIO_Mode_OUT  = 0x01, /*!< GPIO Output Mode */
  GPIO_Mode_AF   = 0x02, /*!< GPIO Alternate function Mode */
  GPIO_Mode_AN   = 0x03  /*!< GPIO Analog Mode */
}GPIOMode_TypeDef;
typedef enum
{
  GPIO_PuPd_NOPULL = 0x00,
  GPIO_PuPd_UP     = 0x01,
  GPIO_PuPd_DOWN   = 0x02
}GPIOPuPd_TypeDef;
typedef enum
{
  GPIO_OType_PP = 0x0,
  GPIO_OType_OD = 0x1
}GPIOOType_TypeDef;
// IR related stuff
extern void *IrDev, *IrCmd;
extern volatile char IrVarType, IrState, IrGotMsg;
extern int IrBits, IrCount;
extern char *IrInterrupt;
void IrClose(void);
void IrInit(void);
void IrReset(void);
void IRSendSignal(int pin, int half_cycles);
void DHT22(char *p);                            // BITBANG DHT22 (HUMID), WS2812 and BITSTREAM's own: their arguments (External.c),
void WS2812(char *q);                           // for DEVICE too (MiscSTM32.c)
void Bitstream(char *tp);

// numpad declares
extern char *KeypadInterrupt;
int KeypadCheck(void);

#define IR_CLOSED                   0
#define IR_WAIT_START               1
#define IR_WAIT_START_END           2
#define SONY_WAIT_BIT_START         3
#define SONY_WAIT_BIT_END           4
#define NEC_WAIT_FIRST_BIT_START    5
#define NEC_WAIT_BIT_START          7
#define NEC_WAIT_BIT_END            8
#define elapsed             readusclock()


#endif
#endif

