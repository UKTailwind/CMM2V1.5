/***************************************************************************
CMM2 MMBasic
MM_Custom.c

Handles all the custom commands and functions in the Maximite implementation of MMBasic.  These are commands and functions
that are not normally part of the Maximite.  This is a good place to insert your own customised commands.

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

*******************************************************************************/

#include <stdio.h>

#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#define max(x, y) (((x) > (y)) ? (x) : (y))


/*************************************************************************************************************************
**************************************************************************************************************************
IMPORTANT:
This module is empty and should be used for your special functions and commands.  In the standard distribution this file
will never be changed, so your code should be safe here.  You should avoid placing commands and functions in other files as
they may be changed and you would then need to re insert your changes in a new release of the source.

**************************************************************************************************************************
**************************************************************************************************************************/

/********************************************************************************************************************************************
 custom commands and functions
 each function is responsible for decoding a command
 all function names are in the form cmd_xxxx() (for a basic command) or fun_xxxx() (for a basic function) so, if you want to search for the
 function responsible for the NAME command look for cmd_name

 There are 4 items of information that are setup before the command is run.
 All these are globals.

 int cmdtoken	This is the token number of the command (some commands can handle multiple
				statement types and this helps them differentiate)

 char *cmdline	This is the command line terminated with a zero char and trimmed of leading
				spaces.  It may exist anywhere in memory (or even ROM).

 char *nextstmt	This is a pointer to the next statement to be executed.  The only thing a
				command can do with it is save it or change it to some other location.

 char *CurrentLinePtr  This is read only and is set to NULL if the command is in immediate mode.

 The only actions a command can do to change the program flow is to change nextstmt or
 execute longjmp(mark, 1) if it wants to abort the program.

 ********************************************************************************************************************************************/
/**
  * @brief  Sets the TIM Autoreload Register value on runtime without calling
  *         another time any Init function.
  * @param  __HANDLE__: TIM handle.
  * @param  __AUTORELOAD__: specifies the Counter register new value.
  * @retval None
  */
extern DAC_HandleTypeDef hdac1;
//extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern TIM_HandleTypeDef htim13;
//uint64_t *d1point=NULL, *d2point=NULL;
int64_t *d1point=NULL, *d2point=NULL;  //parseintegerarray uses int64_t
int d1max, d2max;
volatile int d1pos, d2pos;
volatile uint64_t * volatile  a1point=NULL, * volatile a2point=NULL, *volatile  a3point=NULL;
volatile MMFLOAT * volatile a1float=NULL, * volatile a2float=NULL, * volatile a3float=NULL;
int ADCmax=0;
volatile int ADCpos=0;
extern int ADC_init(int32_t pin, int fast);
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
volatile int ADCchannelA=0;
volatile int ADCchannelB=0;
volatile int ADCchannelC=0;
int ADCtriggervalue=0;
int ADCtriggerchannel=0;
int ADCnegativeslope=0;
int ADCtriggertimeout=0;                                            // ADC TRIGGER's third argument: samples to wait for the edge, 0 for ever
MMFLOAT ADCscale[3], ADCbottom[3];                                  // ADC START's scale (per count) and offset for each channel: volts without
char *ADCInterrupt;
volatile char *DACInterrupt;
int ADCNumchannels=0;
extern const MMFLOAT ADCdiv[];
int overrun;
volatile int ADCcomplete = false;
volatile int DACcomplete = false;
/**
  * @brief TIM13 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM13_Init(int prescale, int period)
{

  /* USER CODE BEGIN TIM13_Init 0 */

  /* USER CODE END TIM13_Init 0 */

  /* USER CODE BEGIN TIM13_Init 1 */

  /* USER CODE END TIM13_Init 1 */
  htim13.Instance = TIM13;
  htim13.Init.Prescaler = prescale;
  htim13.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim13.Init.Period = period;
  htim13.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim13.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim13) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM13_Init 2 */
  if (HAL_TIM_Base_Start_IT(&htim13) != HAL_OK)
  {
    /* Starting Error */
	  error("HAL_TIM_Base_Start_IT");
  }
  /* USER CODE END TIM13_Init 2 */

}

static void MX_TIM6_Init(int prescale, int period)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = prescale;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = period;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    error("HAL_TIM_Base_Init");
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    error("HAL_TIMEx_MasterConfigSynchronization");
  }
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    /* Initialization Error */
    error("HAL_TIM_Base_Init");
  }

  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
  {
    /* Starting Error */
	  error("HAL_TIM_Base_Start_IT");
  }
}
static void MX_TIM7_Init(int prescale, int period)
{

  /* USER CODE BEGIN TIM6_Init 0 */
	HAL_TIM_Base_MspDeInit(&htim7);
  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = prescale;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = period;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    error("HAL_TIM_Base_Init");
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    error("HAL_TIMEx_MasterConfigSynchronization");
  }
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    /* Initialization Error */
    error("HAL_TIM_Base_Init");
  }

  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if (HAL_TIM_Base_Start_IT(&htim7) != HAL_OK)
  {
    /* Starting Error */
	  error("HAL_TIM_Base_Start_IT");
  }
}
MMFLOAT headingrange(MMFLOAT n){
	while(n<0)n+=360.0;
	while(n>=360.0)n-=360.0;
	return n;
}
// SETTICK FAST's timer stopped, its interrupt too (it ran on after the program before: up to 50 kHz at the prompt)
void FastTickStop(void){
	TickInt[NBRSETTICKS] = NULL;
	if(htim13.Instance) HAL_TIM_Base_Stop_IT(&htim13);
	TickTimer[NBRSETTICKS] = 0;
}
void cmd_fasttick(char *in){
	getargs(&in, 3, ",");
	if(!(argc == 3)) error("Argument count");
    MMFLOAT freq=getnumber(argv[0])*2.0L;
	if(freq < 0) error("Number out of bounds");
	if(freq == 0) {
		FastTickStop();													// turn off the interrupt
	} else {
		int prescale=1;
		int period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
		while(period>65535){
			prescale++;
			prescale*=2;
			prescale--;
			period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
		}
		prescale++;
		prescale*=2;
		prescale--;
		TickInt[NBRSETTICKS] = GetIntAddress(argv[2]);					    // get a pointer to the interrupt routine
		TickTimer[NBRSETTICKS] = 0;										    // set the timer running (TIM13 signals each tick)
		MX_TIM13_Init(prescale,period);
	}
}


void ADCclose(void){
	if(ADCchannelA){
		HAL_TIM_Base_Stop(&htim7);
		HAL_TIM_Base_DeInit(&htim7);
		if (HAL_ADC_DeInit(&hadc1) != HAL_OK)
		{
			// ADC de-initialization Error /
			error("HAL_ADC_DeInitA");
		}
		ExtCfg(ADCchannelA, EXT_NOT_CONFIG, 0);
		}
	if(ADCchannelB){
		if (HAL_ADC_DeInit(&hadc2) != HAL_OK)
		{
		    //* ADC de-initialization Error/
		    error("HAL_ADC_DeInitB");
		}
		ExtCfg(ADCchannelB, EXT_NOT_CONFIG, 0);
	}
	if(ADCchannelC){
		if (HAL_ADC_DeInit(&hadc3) != HAL_OK)
		{
		    //* ADC de-initialization Error/
		    error("HAL_ADC_DeInitC");
		}
		ExtCfg(ADCchannelC, EXT_NOT_CONFIG, 0);
	}
	ADCchannelA = ADCchannelB = ADCchannelC = ADCtriggerchannel = ADCNumchannels = ADCpos = ADCmax = 0;
	ADCInterrupt=NULL;
	ADCcomplete = false;
}
//#endif
void cmd_ADC(void){
	char *tp;
    static int  prescale, period;
	ADC_ChannelConfTypeDef sConfigA, sConfigB, sConfigC;
	int ADCb;
    MMFLOAT freq=0.0;
	tp = checkstring(cmdline, "OPEN");
	if(tp) {
     	getargs(&tp, 9, ",");
     	if(argc<3)error("Syntax");
		if(ADCchannelA)error("ADC already open");
        mymemset(&sConfigA,0,sizeof(ADC_ChannelConfTypeDef));
    	mymemset(&sConfigB,0,sizeof(ADC_ChannelConfTypeDef));
    	memset(&sConfigC,0,sizeof(ADC_ChannelConfTypeDef));
    	ADCchannelB = ADCchannelC = 0;
    	if(argc == 9) {
        	IntSignal();
        	ADCInterrupt = GetIntAddress(argv[8]);							// get the interrupt location
    	} else
    		ADCInterrupt = NULL;
        freq=getnumber(argv[0])*2.0L;
        ADCb=16;
        if(freq>1000000.0)error("Frequency greater than 500KHz");
        if(freq>960000.0 && SystemCoreClock==400000)error("Frequency greater than 480KHz");
        if(freq>20000.0)ADCb=14;
        if(freq>40000.0)ADCb=12;
        if(freq>80000.0)ADCb=10;
        if(freq>320000.0)ADCb=8;
    	prescale=1;
        period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
        while(period>65535){
        	prescale++;
        	prescale*=2;
        	prescale--;
            period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
        }
    	prescale++;
    	prescale*=2;
    	prescale--;
        {                                                       // the pins are checked before they become the channels: an error here closes the ADC
            int a = getint(argv[2],0,NBRPINS), b = 0, c = 0;       // (ClearExternalIO), which must not unconfigure a pin that failed because it is a
            if(argc>3 && *argv[4])b=getint(argv[4],0,NBRPINS);    // COM or SPI port's (Gerry's b14)
            if(argc>5 && *argv[6])c=getint(argv[6],0,NBRPINS);
            if(PinDef[a].ADC!=ADC1)error("First channel must use ANALOG_A pin");
            CheckPin(a, CP_IGNORE_INUSE);
            if(b){
                if(PinDef[b].ADC!=ADC2)error("Second channel must use ANALOG_B pin");
                CheckPin(b, CP_IGNORE_INUSE);
            }
            if(c){
                if(PinDef[c].ADC!=ADC3)error("Third channel must use ANALOG_C pin");
                CheckPin(c, CP_IGNORE_INUSE);
            }
            ADCchannelA = a; ADCchannelB = b; ADCchannelC = c;
        }
        ADCNumchannels=1;
        if(ADCchannelB){
            ExtCfg(ADCchannelB, EXT_ANA_IN, ADCb);
            ExtCfg(ADCchannelB, EXT_COM_RESERVED, 0);
            ADCNumchannels++;
        }
        if(ADCchannelC){
            ExtCfg(ADCchannelC, EXT_ANA_IN, ADCb);
            ExtCfg(ADCchannelC, EXT_COM_RESERVED, 0);
            ADCNumchannels++;
        }
        ExtCfg(ADCchannelA, EXT_ANA_IN, ADCb);
        ExtCfg(ADCchannelA, EXT_COM_RESERVED, 0);
        ADC_init(ADCchannelA, 1);
    	sConfigA.Channel      = PinDef[ADCchannelA].ADCchannel;                /* Sampled channel number */
    	sConfigA.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCx_CHANNEL */
    	sConfigA.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;    /* Sampling time (number of clock cycles unit) */
    	sConfigA.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
    	sConfigA.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */
    	sConfigA.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */


    	if (HAL_ADC_ConfigChannel(&hadc1, &sConfigA) != HAL_OK)
    	{
    		/* Channel Configuration Error */
		    error("HAL_ADC_ConfigChannelA");
    	}
        if(ADCchannelB){
        	ADC_init(ADCchannelB ,1);
        	sConfigB.Channel      = PinDef[ADCchannelB].ADCchannel;                /* Sampled channel number */
        	sConfigB.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCx_CHANNEL */
        	sConfigB.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;    /* Sampling time (number of clock cycles unit) */
        	sConfigB.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
        	sConfigB.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */
        	sConfigB.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */


        	if (HAL_ADC_ConfigChannel(&hadc2, &sConfigB) != HAL_OK)
        	{
        		/* Channel Configuration Error */
      		    error("HAL_ADC_ConfigChannelB");
        	}
        }
        if(ADCchannelC){
        	ADC_init(ADCchannelC ,1);
        	sConfigC.Channel      = PinDef[ADCchannelC].ADCchannel;                /* Sampled channel number */
        	sConfigC.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCx_CHANNEL */
        	sConfigC.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;    /* Sampling time (number of clock cycles unit) */
        	sConfigC.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
        	sConfigC.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */
        	sConfigC.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */


        	if (HAL_ADC_ConfigChannel(&hadc3, &sConfigC) != HAL_OK)
        	{
        		/* Channel Configuration Error */
      		    error("HAL_ADC_ConfigChannelC");
        	}
        }
    	return;
	}
	tp = checkstring(cmdline, "START");
	if(tp) {
     	getargs(&tp, 17, ",");                                      // a(), b(), c(), then bottom and top for each channel
		if(!ADCchannelA)error("ADC not open");
        if(!(argc >= 1))error("Argument count");
        a1point=NULL; a2point=NULL; a3point=NULL;
        a1float=NULL; a2float=NULL; a3float=NULL;
        ADCmax=0;
        ADCpos=0;
        // each array whole (a!(5) filled ADCmax+1 samples from element 5, past the end) and not a constant; with an
        // interrupt the command returns at once and the ADC fills them later, so they must be a global's
        MMFLOAT *af;
        ADCmax = parsefloatarray(argv[0], &af, 1, 1, NULL, true, NULL) - 1;
        if(ADCInterrupt) RequireGlobal(af);
        a1float = af; a1point = (uint64_t *)af;
        if(argc>=3 && *argv[2]){                                    // (an empty array slot when the scaling follows)
            if(!ADCchannelB)error("Second channel not open");
            if(parsefloatarray(argv[2], &af, 2, 1, NULL, true, NULL) - 1 != ADCmax) error("Arrays should be the same size");
            if(ADCInterrupt) RequireGlobal(af);
            a2float = af; a2point = (uint64_t *)af;
        }
        if(argc>=5 && *argv[4]){
            if(!ADCchannelC)error("Third channel not open");
            if(parsefloatarray(argv[4], &af, 3, 1, NULL, true, NULL) - 1 != ADCmax) error("Arrays should be the same size");
            if(ADCInterrupt) RequireGlobal(af);
            a3float = af; a3point = (uint64_t *)af;
        }
        // the scale and offset for each channel: the values its bottom and top readings map to, volts without (the
        // PicoMite's; Gerry's b14)
        for(int k = 0; k < 3; k++) { ADCbottom[k] = 0; ADCscale[k] = VCC; }
        if(argc >= 9 && *argv[6] && *argv[8]) { ADCbottom[0] = getnumber(argv[6]); ADCscale[0] = getnumber(argv[8]) - ADCbottom[0]; }
        if(argc >= 13 && *argv[10] && *argv[12]) { ADCbottom[1] = getnumber(argv[10]); ADCscale[1] = getnumber(argv[12]) - ADCbottom[1]; }
        if(argc >= 17 && *argv[14] && *argv[16]) { ADCbottom[2] = getnumber(argv[14]); ADCscale[2] = getnumber(argv[16]) - ADCbottom[2]; }
        ADCscale[0] /= ADCdiv[ADCbits[ADCchannelA]];
        if(ADCchannelB) ADCscale[1] /= ADCdiv[ADCbits[ADCchannelB]];
        if(ADCchannelC) ADCscale[2] /= ADCdiv[ADCbits[ADCchannelC]];
		if (HAL_ADC_Start(&hadc1) != HAL_OK)
		{
			/* Start Conversation Error */
			error("HAL_ADC_StartA");
		}
		if(ADCchannelB){
			if (HAL_ADC_Start(&hadc2) != HAL_OK)
			{
				/* Start Conversation Error */
				error("HAL_ADC_StartB");
			}
		}
		if(ADCchannelC){
			if (HAL_ADC_Start(&hadc3) != HAL_OK)
			{
				/* Start Conversation Error */
				error("HAL_ADC_StartC");
			}
		}
		ADCcomplete=0;
		overrun=0;
		MX_TIM7_Init(prescale,period);
		if(ADCInterrupt==NULL){
    		while(!ADCcomplete){
    			routinechecks(1);
    		}
 		}
    	return;
	}
	tp = checkstring(cmdline, "CLOSE");
	if(tp) {
		if(!ADCchannelA)error("ADC not open");
		ADCclose();
		return;
	}
	tp = checkstring(cmdline, "TRIGGER");
	if(tp) {
		MMFLOAT voltage;
     	getargs(&tp, 5, ",");
     	if(argc!=3 && argc!=5)error("Syntax");
     	ADCtriggerchannel=getint(argv[0],1,ADCNumchannels);
    	voltage = getnumber(argv[2]);
    	if(voltage<=-VCC || voltage >=VCC) error("Invalid Voltage");
		ADCnegativeslope=0;
    	if(voltage<0.0){
    		ADCnegativeslope=1;
    		voltage=-voltage;
    	}
		ADCtriggervalue=(int)(voltage/VCC*65535);
		ADCtriggertimeout = (argc == 5 && *argv[4]) ? getint(argv[4], 0, 0x7FFFFFFF) : 0;   // samples to wait for the edge; 0 waits for ever
    	return;
	}
	tp = checkstring(cmdline, "FREQUENCY");
	if(tp) {
		int newbits;
     	getargs(&tp, 1, ",");
 		if(!ADCchannelA)error("ADC not open");
        freq=getnumber(argv[0])*2.0L;
        newbits=16;
        if(freq>1000000.0)error("Frequency greater than 500KHz");
        if(freq>960000.0 && SystemCoreClock==400000)error("Frequency greater than 480KHz");
        if(freq>20000.0)newbits=14;
        if(freq>40000.0)newbits=12;
        if(freq>80000.0)newbits=10;
        if(freq>320000.0)newbits=8;
		if(ADCbits[ADCchannelA]!=newbits)error("Invalid frequency change - use CLOSE then OPEN");
    	prescale=1;
        period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
        while(period>65535){
        	prescale++;
        	prescale*=2;
        	prescale--;
            period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
        }
    	prescale++;
    	prescale*=2;
    	prescale--;
    	return;
	}
}
//#endif

void dacclose(void){
	CurrentlyPlaying=P_NOTHING;
	if(d1max){
		HAL_TIM_Base_Stop(&htim6);
		HAL_TIM_Base_DeInit(&htim6);
	}
    HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2000);
    HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2, DAC_ALIGN_12B_R, 2000);
	d1max=0;
	d2max=0;
}
void cmd_DAC(void){
	char *tp;
    int channel;
    uint16_t dacvalue;
    float voltage;
    MMFLOAT freq;
   // void *ptr1 = NULL;
   // void *ptr2 = NULL;
    int  prescale, period;
    tp = checkstring(cmdline, "STOP");
    if(tp){
    	dacclose();
    	return;
    }
    tp = checkstring(cmdline, "START");
    if(tp){
     	getargs(&tp, 7, ",");
        if(!(CurrentlyPlaying == P_NOTHING || CurrentlyPlaying == P_DAC )) error("DAC in use");
        if(CurrentlyPlaying == P_DAC)dacclose();
        if(!(argc == 7 ||argc == 5 || argc == 3))error("Argument count");
        freq=getnumber(argv[0]);
        if(freq>500000) error("Frequency > 500KHz");
        d1max=0;
        d2max=0;
        d1pos=0;
        d2pos=0;
        d1max=parseintegerarray(argv[2],&d1point,1,1, NULL, false, NULL)-1;
        RequireGlobal(d1point);                                     // the DAC interrupt reads it after the command returns
               //PInt(d1max);
        if(argc>=5 && *argv[4]){                                    // (the second array, also when an interrupt follows)
           d2max=parseintegerarray(argv[4],&d2point,2,1, NULL, false, NULL)-1;
           RequireGlobal(d2point);
              // PInt(d2max);
        }
       /*
        ptr1 = findvar(argv[2], V_FIND | V_EMPTY_OK);
        if(VREC(VarIndex)->type & T_INT) {
            if(!DimIsEnd(VREC(VarIndex)->dimtbl[1])) error("Invalid variable");
            if(!DimIsRealArray(VREC(VarIndex)->dimtbl[0])) {		// Not an array
                error("Argument 2 must be integer array");
            }
            d1point = (uint64_t *)ptr1;
        } else error("Argument 2 must be integer array");
        d1max=(DimUpper(VREC(VarIndex)->dimtbl[0]) - OptionBase);
        if(argc>=5 && *argv[4]){
            ptr2 = findvar(argv[4], V_FIND | V_EMPTY_OK);
            if(VREC(VarIndex)->type & T_INT) {
                if(!DimIsEnd(VREC(VarIndex)->dimtbl[1])) error("Invalid variable");
                if(!DimIsRealArray(VREC(VarIndex)->dimtbl[0])) {		// Not an array
                    error("Argument 3 must be integer array");
                }
                d2point = (uint64_t *)ptr2;
            } else error("Argument 3 must be integer array");
            d2max=(DimUpper(VREC(VarIndex)->dimtbl[0]) - OptionBase);
        }
       */
    	if(argc == 7) {
    		DACcomplete = false;
    		IntSignal();
        	DACInterrupt = GetIntAddress(argv[6]);							// get the interrupt location
    	} else
    		DACInterrupt = NULL;
        CurrentlyPlaying=P_DAC;
    	prescale=1;
        period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
        while(period>65535){
        	prescale++;
        	prescale*=2;
        	prescale--;
            period=(int)(MMFLOAT)SystemCoreClock/2.0/freq/(MMFLOAT)(prescale+1);
        }
    	MX_TIM6_Init(prescale,period);
    	return;
    }
	getargs(&cmdline, 9, ",");
	if((argc & 0x01) == 0 || argc < 3) error("Invalid syntax");

    channel = getint(argv[0], 1, 2);
	voltage = getnumber(argv[2]);
	if(voltage<0 || voltage >=VCC) error("Invalid Voltage");
	dacvalue=(uint16_t)(voltage/VCC*4096.0);
    switch(channel) {
        case 1:  {
        	HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacvalue);
        	break;
        }
        case 2:  {
        	HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2, DAC_ALIGN_12B_R, dacvalue);
        	break;
        default: error("No such DAC");
        }
    }
}
