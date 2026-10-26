 /**
  ******************************************************************************
  * File Name          : LTDC.c
  * Description        : This file provides code for the configuration
  *                      of the LTDC instances.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ltdc.h"
#include "CLUT.h"
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#define PLL3_TIMEOUT_VALUE         ((uint32_t)2)    /* 2 ms */

#define DIVIDER_P_UPDATE          0U
#define DIVIDER_Q_UPDATE          1U
#define DIVIDER_R_UPDATE          2U
struct s_pagetable PageTable[66];
int LastPage;
volatile uint32_t ShareNeeded=0;
extern volatile uint8_t pagesetdone;
extern uint32_t pll1,pll2,PLL3M,PLL3N,PLL3P,PLL3Q,PLL3R,PLL3RGE;
extern void GT911_Init();
uint32_t CLUT[256];
static void MX_LTDC_Init(int x, int y, int yscale, int colours, int bc);
LTDC_LayerCfgTypeDef pLayerCfg = {0};
LTDC_LayerCfgTypeDef pLayerCfg1 = {0};
LTDC_HandleTypeDef hltdc={0};
const int xres[MAX_MODES+1]={0,800,640,320,480,240,256,320,640,1024,848,1280,960,400,960,1280,1920,384,1024};
const int yres[MAX_MODES+1]={0,600,400,200,432,216,240,240,480,768,480,720,540,300,540,1024,1080,240,600};
const int pageheight[MAX_MODES+1]={0,627,499,499,499,499,499,499,499,805,516,749,1124,627,568,1067,1124,499,637};
/* The display's size: the mode's (xres, yres), or under OPTION PICOMITE the PicoMite's (PicoMode.c), smaller. setmode
   takes a size asked for in ModeReqW and ModeReqH (and a font in ModeReqFont) and clears them, so every other call is
   the mode's own size. The pages are that size, and the LTDC shows them as a window at the top left of the screen. */
int ModeReqW, ModeReqH, ModeReqFont, ModeW, ModeH;
int ModeXres(int mode) { return ModeW ? ModeW : xres[mode]; }
int ModeYres(int mode) { return ModeH ? ModeH : yres[mode]; }
/* OPTION PICOMITE's smaller modes, made from the base mode's timing as mode 7 is made from mode 8 (PicoMode.c): the pixel
   clock divided by ModeHs (each pixel ModeHs wide), each row shown ModeVs times (2: the pages store it twice, as the
   doubled modes' do; in a 4 or 1-bit mode the merge writes it so; 8-bit rows four times: Merge8, Rgb121.c). 0: the
   mode's own. DisplayMerged: the LTDC shows the merge's display (Rgb121Display), not page 0. */
int ModeReqHs, ModeReqVs, ModeHs, ModeVs, DisplayMerged;
int ModeRowScale(int mode) {
  if(ModeVs) return ModeVs;
  return (mode==3 || mode==5 || mode==6 || mode==7 || mode==12 || mode==13 || mode==17) ? 2 : 1;
}

#define RoundUptoPage8(a)     ((((uint64_t)a) + (uint64_t)(8*1024 - 1)) & (uint64_t)(~(8*1024 - 1)))// round up to the nearest whole integer
#define RoundUptoPage512(a)     ((((uint64_t)a) + (uint64_t)(512*1024 - 1)) & (uint64_t)(~(512*1024 - 1)))// round up to the nearest whole integer
//
int scalesize(int vmode,int vcol, int pageno){
 int retscale=1;
  if(ModeRowScale(vmode)==2){
    if(pageno==0 || pageno==1) retscale=2;
    if(pageno==2 && vcol==12)retscale=2;
  }
  if( vcol!=8)retscale *= 2;
  if(vcol==32)retscale *= 2;
  return retscale;
}

int pagegen(int Vmode, int Vcol){
  uint32_t startadd=0x24000000, sdram=0xD0000000;
  int i, size,pageno=0;
  Rgb121Active=0;                                                   // (the merge stops while the pages are laid out: setmode restarts it)
  memset(&PageTable,0,sizeof(struct s_pagetable)*64);
  ShareNeeded=0;
  DisplayMerged=(Vcol<8 || (Vcol==8 && ModeVs==4));
  if(Vcol==4 || (Vcol==8 && ModeVs==4))startadd=Rgb121Layout(Vmode, startadd);   // a 4-bit mode (or 8-bit rows four times):
                                                                    // the display the merge writes, then the pages (Rgb121.c)
  if(Vcol==1){                                                      // a 1-bit mode: the tiles' colours and the display, then the
	  startadd=MonoLayout(Vmode, startadd, &sdram);                 // pages (Mono.c); the display in SDRAM is shown, as a page 0 there is
	  if(sdram>(uint32_t)0xD0000000)ShareNeeded=RoundUptoPage512(sdram);
  }
  while(1){
    size=RoundUptoPage8(Vcol==4 ? ((ModeXres(Vmode)+1)/2)*ModeYres(Vmode) : Vcol==1 ? (ModeXres(Vmode)/8)*ModeYres(Vmode) : ModeXres(Vmode)*ModeYres(Vmode)*scalesize(Vmode,Vcol,pageno));
    if(startadd+(uint32_t)size>0x24080000 && startadd<(uint32_t)0xD0000000)startadd=sdram;
    if((startadd+(uint32_t)size>(G1Hardware ? (uint32_t)0xD0300000 : (uint32_t)0xD0700000))  || pageno>MAXPAGES)break;
    PageTable[pageno].address=(uint8_t *)startadd;
    PageTable[pageno].nbytes=(Vcol>8)+1;                            // (4 and 1-bit: 1, so sizes made from it are enough)
    if(Vcol==32)PageTable[pageno].nbytes*=2;
    PageTable[pageno].expand=0;
    if(Vcol>=8 && ModeRowScale(Vmode)==2){
    	if(pageno==0 || pageno==1) PageTable[pageno].expand=1;
    	if(pageno==2 && Vcol==12)PageTable[pageno].expand=1;
    }
    PageTable[pageno].xmax=ModeXres(Vmode);
    PageTable[pageno].ymax=ModeYres(Vmode);
    PageTable[pageno].maxlines=pageheight[Vmode];
    PageTable[pageno].size=(Vcol==4 ? ((ModeXres(Vmode)+1)/2)*ModeYres(Vmode) : Vcol==1 ? (ModeXres(Vmode)/8)*ModeYres(Vmode) : ModeXres(Vmode)*ModeYres(Vmode)*scalesize(Vmode,Vcol,pageno));
    startadd+=(uint32_t)size;
    if(pageno==0 && Vcol>=8 && startadd>(uint32_t)0xD0000000){      // (a 4 or 1-bit page 0 is not shown)
    	ShareNeeded=RoundUptoPage512(startadd);
    }
    if(pageno==1 && Vcol==12 && startadd>(uint32_t)0xD0000000)ShareNeeded=RoundUptoPage512(startadd);
    pageno++;
  }
  for(i=1;i<=MAXCAM;i++){
	  camera[i].viewplane=-32767;
  }
  HRes=PageTable[0].xmax;
  VRes=PageTable[0].ymax;
  return pageno-1;
}

void __attribute__ ((optimize("-O2"))) HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *ltdc){
		pagesetdone=1;
		Framecomplete=1;
		if(FrameInterrupt != NULL) IntSignal();
		if(Rgb121Active) NVIC_SetPendingIRQ(CEC_IRQn);                // a 4 or 1-bit mode: the merge, at a low priority (Rgb121.c)
		HAL_LTDC_ProgramLineEvent(&hltdc, hltdc.Init.AccumulatedActiveH+1);
}
HAL_StatusTypeDef PLL3_Config(RCC_PLL3InitTypeDef *pll3, uint32_t Divider)
{
  uint32_t tickstart;
  HAL_StatusTypeDef status = HAL_OK;
  assert_param(IS_RCC_PLL3M_VALUE(pll3->PLL3M));
  assert_param(IS_RCC_PLL3N_VALUE(pll3->PLL3N));
  assert_param(IS_RCC_PLL3P_VALUE(pll3->PLL3P));
  assert_param(IS_RCC_PLL3R_VALUE(pll3->PLL3R));
  assert_param(IS_RCC_PLL3Q_VALUE(pll3->PLL3Q));

  /* Check that PLL3 OSC clock source is already set */
  if(__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_NONE)
  {
    return HAL_ERROR;
  }


  else
  {
    /* Disable  PLL3. */
    __HAL_RCC_PLL3_DISABLE();

    /* Get Start Tick*/
    tickstart = HAL_GetTick();
    /* Wait till PLL3 is ready */
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) != RESET)
    {
      if((int32_t) (HAL_GetTick() - tickstart ) > PLL3_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }

    /* Configure the PLL3  multiplication and division factors. */
    /* Configure the PLL3  multiplication and division factors. */
    __HAL_RCC_PLL3_CONFIG(pll3->PLL3M,
                          pll3->PLL3N,
                          pll3->PLL3P,
                          pll3->PLL3Q,
                          pll3->PLL3R);

    /* Select PLL3 input reference frequency range: VCI */
    __HAL_RCC_PLL3_VCIRANGE(pll3->PLL3RGE) ;

    /* Select PLL3 output frequency range : VCO */
    __HAL_RCC_PLL3_VCORANGE(pll3->PLL3VCOSEL) ;

    /* Disable PLL3FRACN . */
    __HAL_RCC_PLL3FRACN_DISABLE();

    /* Configures PLL3 clock Fractional Part Of The Multiplication Factor */
//    __HAL_RCC_PLL3FRACN_CONFIG(pll3->PLL3FRACN);

    /* Enable PLL3FRACN . */
//    __HAL_RCC_PLL3FRACN_ENABLE();

    /* Enable the PLL3 clock output */
    if(Divider == DIVIDER_P_UPDATE)
    {
      __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVP);
    }
    else if(Divider == DIVIDER_Q_UPDATE)
    {
      __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);
    }
    else
    {
      __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);
    }

    /* Enable  PLL3. */
    __HAL_RCC_PLL3_ENABLE();

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till PLL3 is ready */
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) == RESET)
    {
      if((int32_t) (HAL_GetTick() - tickstart ) > PLL3_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }

  }


  return status;
}

/* OPTION VGA OUTPUT for the colours that are not the 8-bit tables' (L8_CLUT_*: the 4-bit modes' palette and the 1-bit
   modes' tiles in the CLUT). The tables never drive a channel to full: HIGH (the default) and MEDIUM reach 0xE0, LOW
   0xC0. Each channel goes through the red levels of the table the option picks, between its eight. Peter, 23 Oct 2026,
   the G1 through a capture device: the more of the screen is full white the more the picture jitters (the outputs'
   current moves the supply), "This is why the clut originally never drove pixels to 0xFFFFFF"; with OPTION VGA
   OUTPUT LOW mode 11 was "rock solid". */
static uint8_t VgaCurve[256];
static int VgaCurveFor = -1;
uint32_t VgaLevel(uint32_t rgb) {
    if(VgaCurveFor != Option.colourmap) {
        const uint32_t *t = Option.colourmap == 0 ? L8_CLUT_LOW : (Option.colourmap == 1 ? L8_CLUT_MEDIUM : L8_CLUT_HIGH);
        for(int v = 0; v < 256; v++) {
            int k = v * 7 / 255, f = v * 7 - k * 255;                  // f/255 of the way from level k to k + 1
            int a = (t[k << 5] >> 16) & 0xFF, b = k < 7 ? (t[(k + 1) << 5] >> 16) & 0xFF : a;
            VgaCurve[v] = a + (b - a) * f / 255;
        }
        VgaCurveFor = Option.colourmap;
    }
    return (uint32_t)VgaCurve[(rgb >> 16) & 0xFF] << 16 | (uint32_t)VgaCurve[(rgb >> 8) & 0xFF] << 8 | VgaCurve[rgb & 0xFF];
}

// and for the 16 and 32-bit modes, which have no CLUT: the layer's constant alpha over black, so full is the tables'
// top (0xE0, or 0xC0 with LOW). The 12-bit mode's layers blend by their pixels' alpha times it, so it is not given one.
static uint32_t VgaAlpha(void) {
    return Option.colourmap == 0 ? 0xC0 : 0xE0;
}

void reset_CLUT(void){
	if(VideoColour==8){
		if(Option.colourmap==0)mycopy((char *)CLUT,(char *)L8_CLUT_LOW, sizeof(CLUT));
		else if(Option.colourmap==1)mycopy((char *)CLUT,(char *)L8_CLUT_MEDIUM, sizeof(CLUT));
		else mycopy((char *)CLUT,(char *)L8_CLUT_HIGH, sizeof(CLUT));
    	pagesetdone=0;
    	while(!pagesetdone)CheckAbort();
    	HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
	} else if(VideoColour==4){                                       // the PicoMite's 16 colours (Rgb121.c)
		Rgb121Clut();
		pagesetdone=0;
		while(!pagesetdone)CheckAbort();
		HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
	}
}

void setmode(int mode, int colour, int bc, int force){
	static int fnt=((0 << 4) | 1);
	static int DisplayH=0, DisplayV=0;                                       // the physical characteristics of the display
	int oldmode=VideoMode;
	RCC_PLL3InitTypeDef pll;
	gui_fcolour = PromptFC = Option.DefaultFC;                     // (the defaults: OPTION DEFAULT COLOURS, white on black
	gui_bcolour = PromptBC = Option.DefaultBC;                     // unless it is set; they were set back to those here)
   // turtle_init_not_done=1;
	if(VideoColour!=colour && PageTable[WPN].address!=NULL)error("Colour mismatch with framebuffer ");
	{                                                                // the display's size asked for (OPTION PICOMITE), or the
		int w = ModeReqW, h = ModeReqH, hs = ModeReqHs, vs = ModeReqVs;   // mode's: a new one is not the same mode
		ModeReqW = ModeReqH = ModeReqHs = ModeReqVs = 0;
		if((w != ModeW || h != ModeH || hs != ModeHs || vs != ModeVs) && force <= 0) force = 1;
		ModeW = w;
		ModeH = h;
		ModeHs = hs;
		ModeVs = vs;
		PicoActive = (w != 0);
		Rgb121Palette();                                             // (the 16 colours: HDMI's in its modes, else VGA's)
	}
	int reqfont = ModeReqFont;
	ModeReqFont = 0;
	if(VideoMode==mode && VideoColour==colour && force<=0){
		  SetFont(fnt);
		  Option.DefaultFont = gui_font;
		  if(bc!=VideoBackground || force<0){
			  VideoBackground=bc;
			  MX_LTDC_Init(DisplayH, DisplayV, (DisplayMerged ? Rgb121DisplayScale : (PageTable[0].expand ? 2 : 1)), VideoColour, VideoBackground);
		  }
		  LastPage=pagegen(VideoMode, VideoColour);
		  if(VideoColour<8){                                            // (the PicoMite's framebuffers closed, as its MODE does)
			  WritePage=ReadPage=0;
			  if(VideoColour==1)MonoStart();                            // (and its MODE 1 cleared, the tiles reset: Mono.c)
		  }
		  if(DisplayMerged)Rgb121Active=1;
		  return;
	}
	cursoron=0;
	cursorenable=0;
	FreeMemorySafe((void *)&cursorsave);
	SpritesForget();                                                 // the screen is cleared: no sprite is showing, but
	if(VideoColour!=colour){										// their images are kept (colour words: sprites.c); a
		FreeMemorySafe((void *)&loadcursordata);                    // loaded cursor is in the old depth's pixels
	}
	//FreeMemorySafe((void *)&main_turtle_polyX);
	//FreeMemorySafe((void *)&main_turtle_polyY);
	if(mode>=0){
		deferredcopy=0;
    	pagesetdone=0;
    	while(!pagesetdone){
        	routinechecks(1);
    	}
		if(hltdc.Instance!=0  && VideoColour<=8)HAL_LTDC_DisableCLUT(&hltdc, 0);
		HAL_NVIC_DisableIRQ(LTDC_IRQn);
		if(VideoColour==12){
			mymemset(&pLayerCfg1,0,sizeof(LTDC_LayerCfgTypeDef));
			HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1);
		}
	    HAL_LTDC_DeInit(&hltdc);
	} else mode=-mode;
	VideoMode=mode;VideoColour=colour; VideoBackground=bc;
	LastPage=pagegen(VideoMode, VideoColour);
	MPU_Config_nCacheable(0);
	if(colour==8){
		if(Option.colourmap==0)mycopy((char *)CLUT,(char *)L8_CLUT_LOW, sizeof(CLUT));
		else if(Option.colourmap==1)mycopy((char *)CLUT,(char *)L8_CLUT_MEDIUM, sizeof(CLUT));
		else mycopy((char *)CLUT,(char *)L8_CLUT_HIGH, sizeof(CLUT));
	} else if(colour==4) Rgb121Clut();
	else if(colour==1) MonoClut();                                  // (the tiles' colours are given entries: Mono.c)
    hltdc.Instance=0;
    CurrentX=0;
    CurrentY=0;
	/* PLL3, the pixel clock. The LTDC is made for panels clocked by its LCD_CLK, where the clock's jitter does not
	   matter; over VGA the receiver makes its own pixel clock from HSYNC, so any wander in PLL3 puts each line's pixels
	   in a different place. A capture device shows it (Peter, 23 Oct 2026, the G1: "the line start timing is varying").
	   The jitter falls as the reference and the VCO rise: so the 8 MHz crystal undivided (M 1, the 8-16 MHz range) and
	   the highest VCO in range (192-836 MHz, so N 24-104) whose clock, 8N/R, is within 0.45% of the mode's (VESA allows
	   0.5%). Peter, on the monitor: a 2 MHz reference was "massive flickering", 4 MHz (the old settings) flickered,
	   and at 8 MHz mode 11 with an 816 MHz VCO was better than mode 9 with 520. Integer N only: FRACN's modulator
	   would add jitter of its own. */
	pll.PLL3VCOSEL=RCC_PLL3VCOWIDE;
	pll.PLL3FRACN=0;
	DisplayH=ModeXres(VideoMode);                                     // (OPTION PICOMITE's size: a window of the screen)
	DisplayV=ModeYres(VideoMode);
	if(VideoMode==1){
	  pll.PLL3M = 1; //75Hz 49.5MHz pixel clock, VCO 792 MHz (VESA 800x600 at 75Hz: Peter's test, 23 Oct 2026)
	  pll.PLL3N = 99;
	  pll.PLL3P = 16;
	  pll.PLL3Q = 16;
	  pll.PLL3R = 16;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((0 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==2){
	  pll.PLL3M = 1; //75Hz 31.385MHz (31.5 -0.37%) pixel clock, VCO 816 MHz
	  pll.PLL3N = 102;
	  pll.PLL3P = 26;
	  pll.PLL3Q = 26;
	  pll.PLL3R = 26;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((0 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==3){
	  pll.PLL3M = 1; //75Hz 15.698MHz (15.75 -0.33%) pixel clock, VCO 832 MHz
	  pll.PLL3N = 104;
	  pll.PLL3P = 53;
	  pll.PLL3Q = 53;
	  pll.PLL3R = 53;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((6 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==4){
	  pll.PLL3M = 1; //60Hz 23.543MHz (23.625 -0.35%) pixel clock, VCO 824 MHz
	  pll.PLL3N = 103;
	  pll.PLL3P = 35;
	  pll.PLL3Q = 35;
	  pll.PLL3R = 35;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((0 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==5){
	  pll.PLL3M = 1; //75Hz 11.771MHz (11.8125 -0.35%) pixel clock, VCO 824 MHz
	  pll.PLL3N = 103;
	  pll.PLL3P = 70;
	  pll.PLL3Q = 70;
	  pll.PLL3R = 70;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((6 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==6){
	  pll.PLL3M = 1; //75Hz 12.606MHz (12.6 +0.05%) pixel clock, VCO 832 MHz
	  pll.PLL3N = 104;
	  pll.PLL3P = 66;
	  pll.PLL3Q = 66;
	  pll.PLL3R = 66;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((6 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==7){
	  pll.PLL3M = 1; //75Hz 15.698MHz (15.75 -0.33%) pixel clock, VCO 832 MHz
	  pll.PLL3N = 104;
	  pll.PLL3P = 53;
	  pll.PLL3Q = 53;
	  pll.PLL3R = 53;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((6 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==8){
	  pll.PLL3M = 1; //75Hz 31.385MHz (31.5 -0.37%) pixel clock, VCO 816 MHz
	  pll.PLL3N = 102;
	  pll.PLL3P = 26;
	  pll.PLL3Q = 26;
	  pll.PLL3R = 26;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((0 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==9){
	  pll.PLL3M = 1; //60Hz 64.727MHz (65 -0.42%) pixel clock, VCO 712 MHz
	  pll.PLL3N = 89;
	  pll.PLL3P = 11;
	  pll.PLL3Q = 11;
	  pll.PLL3R = 11;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((3 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==10){
	  pll.PLL3M = 1; //60Hz 33.667MHz (33.75 -0.25%) pixel clock, VCO 808 MHz
	  pll.PLL3N = 101;
	  pll.PLL3P = 24;
	  pll.PLL3Q = 24;
	  pll.PLL3R = 24;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((0 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==11){
	  pll.PLL3M = 1; //60Hz 74.182MHz (74.25 -0.09%) pixel clock, VCO 816 MHz
	  pll.PLL3N = 102;
	  pll.PLL3P = 11;
	  pll.PLL3Q = 11;
	  pll.PLL3R = 11;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((3 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==12){
	  pll.PLL3M = 1; //60Hz 74.182MHz (74.25 -0.09%) pixel clock, VCO 816 MHz
	  pll.PLL3N = 102;
	  pll.PLL3P = 11;
	  pll.PLL3Q = 11;
	  pll.PLL3R = 11;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((0 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else 	if(VideoMode==13){
	  pll.PLL3M = 1; //75Hz 24.75MHz pixel clock, VCO 792 MHz (mode 1's at half the clock: Peter, 23 Oct 2026)
	  pll.PLL3N = 99;
	  pll.PLL3P = 32;
	  pll.PLL3Q = 32;
	  pll.PLL3R = 32;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((6 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==14){
	  pll.PLL3M = 1; //60Hz 37.818MHz (37.75 +0.18%) pixel clock, VCO 832 MHz
	  pll.PLL3N = 104;
	  pll.PLL3P = 22;
	  pll.PLL3Q = 22;
	  pll.PLL3R = 22;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((0 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==15){
	  pll.PLL3M = 1; //60Hz 108MHz pixel clock, VCO 648 MHz
	  pll.PLL3N = 81;
	  pll.PLL3P = 6;
	  pll.PLL3Q = 6;
	  pll.PLL3R = 6;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((3 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==16){
	  pll.PLL3M = 1; //60Hz 148.8MHz (148.5 +0.20%) pixel clock, VCO 744 MHz
	  pll.PLL3N = 93;
	  pll.PLL3P = 5;
	  pll.PLL3Q = 5;
	  pll.PLL3R = 5;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((2 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==17){
	  pll.PLL3M = 1; //75Hz 18.909MHz (18.9 +0.05%) pixel clock, VCO 832 MHz
	  pll.PLL3N = 104;
	  pll.PLL3P = 44;
	  pll.PLL3Q = 44;
	  pll.PLL3R = 44;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((6 << 4) | 1);
	  Option.DefaultFont = gui_font;
	} else if(VideoMode==18){
	  pll.PLL3M = 1; //59.8Hz 50.5MHz (the PicoMite's 50.4 +0.2%) pixel clock, VCO 808 MHz (it was 58.286 MHz, 68 Hz)
	  pll.PLL3N = 101;
	  pll.PLL3P = 16;
	  pll.PLL3Q = 16;
	  pll.PLL3R = 16;
	  pll.PLL3RGE=RCC_PLL3VCIRANGE_3;
	  fnt = ((3 << 4) | 1);
	  Option.DefaultFont = gui_font;
	}
	if(reqfont) fnt = reqfont;                                        // (the PicoMite's font for its mode: PicoMode.c)
	if(ModeHs > 1){                                                   // (OPTION PICOMITE: the pixel clock divided, PicoMode.c)
		pll.PLL3P *= ModeHs;
		pll.PLL3Q *= ModeHs;
		pll.PLL3R *= ModeHs;
	}
	WritePage=0; ReadPage=0;
	if(DisplayMerged)mymemset(Rgb121Display,0,DisplayH*DisplayV*Rgb121DisplayScale);
	if(oldmode!=VideoMode || force)PLL3_Config(&pll,DIVIDER_P_UPDATE|DIVIDER_Q_UPDATE|DIVIDER_R_UPDATE);
	MX_LTDC_Init(DisplayH, DisplayV, (DisplayMerged ? Rgb121DisplayScale : (PageTable[0].expand ? 2 : 1)), VideoColour, VideoBackground);
	if( VideoColour==16 || VideoColour==12){
		DrawRectangle = DrawRectangle16;
		DrawBitmap = DrawBitmap16;
		DrawBuffer = DrawBuffer16;
		ReadBuffer = ReadBuffer16;
		ScrollLCD = ScrollBuff16V;
		ScrollBufferV = ScrollBuff16V;
		ScrollBufferH = ScrollBuff16H;
		DrawBufferFast = DrawBufferFast16;
		ReadBufferFast = ReadBufferFast16;
		BlitShowBuffer=BlitShowBuff;
		DrawPixel=DrawPixel16;
	} else if(VideoColour==4) {
		Rgb121Pointers();                                           // the 4-bit set (Rgb121.c)
	} else if(VideoColour==1) {
		MonoPointers();                                             // the 1-bit set (Mono.c)
	} else if(VideoColour==8) {
		DrawRectangle = DrawRectangle8;
		DrawBitmap = DrawBitmap8;
		DrawBuffer = DrawBuffer8;
		ReadBuffer = ReadBuffer8;
		ScrollLCD = ScrollBuff8V;
		ScrollBufferV = ScrollBuff8V;
		ScrollBufferH = ScrollBuff8H;
		DrawBufferFast = DrawBufferFast8;
		ReadBufferFast = ReadBufferFast8;
		BlitShowBuffer=BlitShowBuff;
		DrawPixel=DrawPixel8;
		if(DisplayMerged){                                          // (8-bit rows four times: the merge, Rgb121.c)
			HAL_NVIC_SetPriority(CEC_IRQn, IRQ_PRI(6), 0);
			HAL_NVIC_EnableIRQ(CEC_IRQn);
		}
	} else {
		DrawRectangle = DrawRectangle32;
		DrawBitmap = DrawBitmap32;
		DrawBuffer = DrawBuffer32;
		ReadBuffer = ReadBuffer32;
		ScrollLCD = ScrollBuff32V;
		ScrollBufferV = ScrollBuff32V;
		ScrollBufferH = ScrollBuff32H;
		DrawBufferFast = DrawBufferFast32;
		ReadBufferFast = ReadBufferFast32;
		BlitShowBuffer=BlitShowBuff;
		DrawPixel=DrawPixel32;
	}
	SCB_CleanInvalidateDCache();
	MM_Delay(100);
	ClearScreen(gui_bcolour);
	if(VideoColour==12)PageCopy(0, 1, 0);
	SetFont(fnt);
	Option.DefaultFont = gui_font;
	clearrepeat();
	if(VideoColour==1)MonoStart();                                   // the tiles the font's height (Mono.c)
	Rgb121Active=DisplayMerged;                                       // a 4 or 1-bit mode: the merge at each frame (Rgb121.c)
}

/* LTDC init function */
static void MX_LTDC_Init(int x, int y, int yscale, int colours, int bc)
{
	  hltdc.Instance = LTDC;
	  hltdc.Init.Backcolor.Blue = bc & 0xFF;
	  hltdc.Init.Backcolor.Green = (bc & 0xFF00)>>8;
	  hltdc.Init.Backcolor.Red = (bc & 0xFF0000)>>16;
	  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
	  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
	  if(colours==12){
		  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB4444;
		  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
		  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
		  pLayerCfg.Backcolor.Blue = 0;
		  pLayerCfg.Backcolor.Green = 0;
		  pLayerCfg.Backcolor.Red = 0;
		  pLayerCfg.Alpha = 255;
		  pLayerCfg.Alpha0 = 255;
		  pLayerCfg.FBStartAdress = (DisplayMerged ? (uint32_t)Rgb121Display : (uint32_t)PageTable[0].address);
		  pLayerCfg.WindowX0 = 0;
		  pLayerCfg.WindowY0 = 0;
		  pLayerCfg.WindowX1 = x;
		  pLayerCfg.WindowY1 = y*yscale;
		  pLayerCfg.ImageWidth = x;
		  pLayerCfg.ImageHeight = y*yscale;
		  //
		  pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_ARGB4444;
		  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
		  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
		  pLayerCfg1.Backcolor.Blue = 0;
		  pLayerCfg1.Backcolor.Green = 0;
		  pLayerCfg1.Backcolor.Red = 0;
		  pLayerCfg1.Alpha = 255;
		  pLayerCfg1.Alpha0 = 0;
		  pLayerCfg1.FBStartAdress = (uint32_t)PageTable[1].address;
		  pLayerCfg1.WindowX0 = 0;
		  pLayerCfg1.WindowY0 = 0;
		  pLayerCfg1.WindowX1 = x;
		  pLayerCfg1.WindowY1 = y*yscale;
		  pLayerCfg1.ImageWidth = x;
		  pLayerCfg1.ImageHeight = y*yscale;
	  } else {
		  if(colours==16) pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
		  else if(colours<=8) pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_L8;   // (4 and 1-bit: the merge's 8-bit display)
		  else pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
		  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
		  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
		  pLayerCfg.Backcolor.Blue = 0;
		  pLayerCfg.Backcolor.Green = 0;
		  pLayerCfg.Backcolor.Red = 0;
		  pLayerCfg.Alpha = (colours > 8 ? VgaAlpha() : 255);         // (OPTION VGA OUTPUT: the CLUT's modes have it there)
		  pLayerCfg.Alpha0 = 255;
		  pLayerCfg.FBStartAdress = (DisplayMerged ? (uint32_t)Rgb121Display : (uint32_t)PageTable[0].address);
		  pLayerCfg.WindowX0 = 0;
		  pLayerCfg.WindowY0 = 0;
		  pLayerCfg.WindowX1 = x;
		  pLayerCfg.WindowY1 = y*yscale;
		  pLayerCfg.ImageWidth = x;
		  pLayerCfg.ImageHeight = y*yscale;
	  }
	  /* USER CODE BEGIN LTDC_Init 1 */
	  if(VideoMode==1){//800x600
	  	  hltdc.Instance = LTDC; //75Hz 49.5MHz pixel clock (VESA: H 800 + 16 + 80 + 160, V 600 + 1 + 3 + 21, +H +V)
	  	  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
	  	  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
	  	  hltdc.Init.HorizontalSync = 79;
	  	  hltdc.Init.VerticalSync = 2;
	  	  hltdc.Init.AccumulatedHBP = 239 + Option.offsets[1];
	  	  hltdc.Init.AccumulatedVBP = 23;
	  	  hltdc.Init.AccumulatedActiveW = 1039;
	  	  hltdc.Init.AccumulatedActiveH = 623;
	  	  hltdc.Init.TotalWidth = 1055;
	  	  hltdc.Init.TotalHeigh = 624;
	  } else if(VideoMode==2){ //640x400
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 63;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 183 + Option.offsets[2];
		  hltdc.Init.AccumulatedVBP = 58;
		  hltdc.Init.AccumulatedActiveW = 823;;
		  hltdc.Init.AccumulatedActiveH = 458;
		  hltdc.Init.TotalWidth = 839;
		  hltdc.Init.TotalHeigh = 499;
	  } else if((VideoMode==3)){//320x200
		  hltdc.Instance = LTDC;
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 31;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 91 + Option.offsets[3];
		  hltdc.Init.AccumulatedVBP = 58;
		  hltdc.Init.AccumulatedActiveW = 411;;
		  hltdc.Init.AccumulatedActiveH = 458;
		  hltdc.Init.TotalWidth = 419;
		  hltdc.Init.TotalHeigh = 499;
	  } else if(VideoMode==4){
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 47;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 137 + Option.offsets[4];
		  hltdc.Init.AccumulatedVBP = 42;
		  hltdc.Init.AccumulatedActiveW = 617;;
		  hltdc.Init.AccumulatedActiveH = 474;
		  hltdc.Init.TotalWidth = 629;
		  hltdc.Init.TotalHeigh = 499;
	  } else if(VideoMode==5){
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 23;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 67 + Option.offsets[5];
		  hltdc.Init.AccumulatedVBP = 42;
		  hltdc.Init.AccumulatedActiveW = 307;;
		  hltdc.Init.AccumulatedActiveH = 474;
		  hltdc.Init.TotalWidth = 313;
		  hltdc.Init.TotalHeigh = 499;
	  } else if(VideoMode==6){
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 25;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 73 + Option.offsets[6];
		  hltdc.Init.AccumulatedVBP = 18;
		  hltdc.Init.AccumulatedActiveW = 329;;
		  hltdc.Init.AccumulatedActiveH = 498;
		  hltdc.Init.TotalWidth = 335;
		  hltdc.Init.TotalHeigh = 499;
	  } else if((VideoMode==7)){//320x240
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 31;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 91 + Option.offsets[7];
		  hltdc.Init.AccumulatedVBP = 18;
		  hltdc.Init.AccumulatedActiveW = 411;
		  hltdc.Init.AccumulatedActiveH = 498;
		  hltdc.Init.TotalWidth = 419;
		  hltdc.Init.TotalHeigh = 499;
	  } else if(VideoMode==8){ //640x480
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 63;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 183 + Option.offsets[8];
		  hltdc.Init.AccumulatedVBP = 18;
		  hltdc.Init.AccumulatedActiveW = 823;
		  hltdc.Init.AccumulatedActiveH = 498;
		  hltdc.Init.TotalWidth = 839;
		  hltdc.Init.TotalHeigh = 499;
	  } else if(VideoMode==9){ //1024x768
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 135;
		  hltdc.Init.VerticalSync = 5;
		  hltdc.Init.AccumulatedHBP = 295 + Option.offsets[9];
		  hltdc.Init.AccumulatedVBP = 34;
		  hltdc.Init.AccumulatedActiveW = 1319;;
		  hltdc.Init.AccumulatedActiveH = 802;
		  hltdc.Init.TotalWidth = 1343;
		  hltdc.Init.TotalHeigh = 805;
	  } else if(VideoMode==10){ //848x480
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
		  hltdc.Init.HorizontalSync = 111;
		  hltdc.Init.VerticalSync = 7;
		  hltdc.Init.AccumulatedHBP = 223 + Option.offsets[10];
		  hltdc.Init.AccumulatedVBP = 30;
		  hltdc.Init.AccumulatedActiveW = 1071;
		  hltdc.Init.AccumulatedActiveH = 510;
		  hltdc.Init.TotalWidth = 1087;
		  hltdc.Init.TotalHeigh = 516;
	  } else if(VideoMode==11){ //1280x720
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
		  hltdc.Init.HorizontalSync = 39;
		  hltdc.Init.VerticalSync = 4;
		  hltdc.Init.AccumulatedHBP = 259 + Option.offsets[11];
		  hltdc.Init.AccumulatedVBP = 24;
		  hltdc.Init.AccumulatedActiveW = 1539;
		  hltdc.Init.AccumulatedActiveH = 744;
		  hltdc.Init.TotalWidth = 1649;
		  hltdc.Init.TotalHeigh = 749;
	  } else if(VideoMode==12){ //960x540
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
		  hltdc.Init.HorizontalSync = 21;
		  hltdc.Init.VerticalSync = 4;
		  hltdc.Init.AccumulatedHBP = 95 + Option.offsets[12];
		  hltdc.Init.AccumulatedVBP = 40;
		  hltdc.Init.AccumulatedActiveW = 1055;
		  hltdc.Init.AccumulatedActiveH = 1120;
		  hltdc.Init.TotalWidth = 1099;
		  hltdc.Init.TotalHeigh = 1124;
	  } else if(VideoMode==13){//400x300
	  	  hltdc.Instance = LTDC; //75Hz 24.75MHz pixel clock (mode 1's 800x600 at 75Hz, H halved: 400 + 8 + 40 + 80)
	  	  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
	  	  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
	  	  hltdc.Init.HorizontalSync = 39;
	  	  hltdc.Init.VerticalSync = 2;
	  	  hltdc.Init.AccumulatedHBP = 119 + Option.offsets[13];
	  	  hltdc.Init.AccumulatedVBP = 23;
	  	  hltdc.Init.AccumulatedActiveW = 519;
	  	  hltdc.Init.AccumulatedActiveH = 623;
	  	  hltdc.Init.TotalWidth = 527;
	  	  hltdc.Init.TotalHeigh = 624;
	  } else if(VideoMode==14){ //960x540
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
		  hltdc.Init.HorizontalSync = 39;
		  hltdc.Init.VerticalSync = 4;
		  hltdc.Init.AccumulatedHBP = 191/2 + Option.offsets[14];
		  hltdc.Init.AccumulatedVBP = 19;
		  hltdc.Init.AccumulatedActiveW = 2111/2;
		  hltdc.Init.AccumulatedActiveH = 559;
		  hltdc.Init.TotalWidth = 2199/2;
		  hltdc.Init.TotalHeigh = 568;
	  } else if(VideoMode==15){ //1280x1024
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
		  hltdc.Init.HorizontalSync = 111;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 359 + Option.offsets[15];
		  hltdc.Init.AccumulatedVBP = 40;
		  hltdc.Init.AccumulatedActiveW = 1639;
		  hltdc.Init.AccumulatedActiveH = 1064;
		  hltdc.Init.TotalWidth = 1687;
		  hltdc.Init.TotalHeigh = 1065;
	  } else if(VideoMode==16){ //1920x1080
	  /* USER CODE END LTDC_Init 1 */
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AH;
		  hltdc.Init.HorizontalSync = 43;
		  hltdc.Init.VerticalSync = 4;
		  hltdc.Init.AccumulatedHBP = 191 + Option.offsets[16];
		  hltdc.Init.AccumulatedVBP = 40;
		  hltdc.Init.AccumulatedActiveW = 2111;
		  hltdc.Init.AccumulatedActiveH = 1120;
		  hltdc.Init.TotalWidth = 2199;
		  hltdc.Init.TotalHeigh = 1124;
	  } else if((VideoMode==17)){//384x240
		  hltdc.Instance = LTDC;
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 37;
		  hltdc.Init.VerticalSync = 2;
		  hltdc.Init.AccumulatedHBP = 109 + Option.offsets[17];
		  hltdc.Init.AccumulatedVBP = 18;
		  hltdc.Init.AccumulatedActiveW = 493;
		  hltdc.Init.AccumulatedActiveH = 498;
		  hltdc.Init.TotalWidth = 503;
		  hltdc.Init.TotalHeigh = 499;
	  } else if((VideoMode==18)){//1024x600
		  // the PicoMite's 1024x600 (graphics/Screens.h, MODE_H_X_ and MODE_V_X_: 1024x768's line, 800x600@60's frame),
		  // which monitors take (Peter, 24 Oct 2026): H 1024 + 24 + 136 + 160, V 600 + 1 + 4 + 23, +H -V. It was a
		  // panel's (H 1024 + 210 + 10 + 100, V 600 + 18 + 10 + 10, -H -V, 68 Hz), which a monitor showed poorly.
		  hltdc.Instance = LTDC;
		  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AH;
		  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
		  hltdc.Init.HorizontalSync = 135;
		  hltdc.Init.VerticalSync = 3;
		  hltdc.Init.AccumulatedHBP = 295 + Option.offsets[18];
		  hltdc.Init.AccumulatedVBP = 26;
		  hltdc.Init.AccumulatedActiveW = 1319;
		  hltdc.Init.AccumulatedActiveH = 626;
		  hltdc.Init.TotalWidth = 1343;
		  hltdc.Init.TotalHeigh = 627;
	  }
	  if(ModeHs > 1){                                                 // OPTION PICOMITE's smaller modes: the line's parts in
		  hltdc.Init.HorizontalSync = (hltdc.Init.HorizontalSync + 1) / ModeHs - 1;   // pixels of the divided clock,
		  hltdc.Init.AccumulatedHBP = (hltdc.Init.AccumulatedHBP + 1) / ModeHs - 1;   // as mode 7's are mode 8's halved
		  hltdc.Init.AccumulatedActiveW = (hltdc.Init.AccumulatedActiveW + 1) / ModeHs - 1;
		  hltdc.Init.TotalWidth = (hltdc.Init.TotalWidth + 1) / ModeHs - 1;
	  }
	  PageTable[0].maxlines=hltdc.Init.TotalHeigh;
	  PageTable[0].startactive=hltdc.Init.AccumulatedVBP;
if (HAL_LTDC_Init(&hltdc) != HAL_OK)
{
  error("HAL_LTDC_Init");
}
if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
{
	error("HAL_LTDC_ConfigLayer");
}
if(VideoColour==12){
	if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
	{
		error("HAL_LTDC_ConfigLayer1");
	}
} //else HAL_LTDC_ConfigLayer(&hltdc, NULL, 1);
//if(VideoMode==1)HAL_LTDC_SetPitch(&hltdc, 832, 0);
//if(VideoMode==4)HAL_LTDC_SetPitch(&hltdc, 512, 0);
if(VideoColour<=8){
	/*##-2- CLUT Configuration #################################################*/
	HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
	/*##-3- Enable CLUT For Layer 1 ############################################*/
	HAL_LTDC_EnableCLUT(&hltdc, 0);
}
	HAL_LTDC_ProgramLineEvent(&hltdc, hltdc.Init.AccumulatedActiveH+1);
	HAL_NVIC_SetPriority(LTDC_IRQn, IRQ_PRI(0), 0);
	HAL_NVIC_EnableIRQ(LTDC_IRQn);
}


void HAL_LTDC_MspInit(LTDC_HandleTypeDef* ltdcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(ltdcHandle->Instance==LTDC)
  {
  /* USER CODE BEGIN LTDC_MspInit 0 */
  /* USER CODE END LTDC_MspInit 0 */
    /* LTDC clock enable */
    __HAL_RCC_LTDC_CLK_ENABLE();
  
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**LTDC GPIO Configuration    
    PE4     ------> LTDC_B0
    PE5     ------> LTDC_G0
    PE6     ------> LTDC_G1
    PI9     ------> LTDC_VSYNC
    PI10     ------> LTDC_HSYNC
    PF10     ------> LTDC_DE
    PC0     ------> LTDC_R5
    PH2     ------> LTDC_R0
    PH3     ------> LTDC_R1
    PB1     ------> LTDC_R6
    PH8     ------> LTDC_R2
    PH9     ------> LTDC_R3
    PH10     ------> LTDC_R4
    PG6     ------> LTDC_R7
    PG7     ------> LTDC_CLK
    PH13     ------> LTDC_G2
    PH15     ------> LTDC_G4
    PI0     ------> LTDC_G5
    PI1     ------> LTDC_G6
    PI2     ------> LTDC_G7
    PD6     ------> LTDC_B2
    PG10     ------> LTDC_G3
    PG11     ------> LTDC_B3
    PG12     ------> LTDC_B1
    PI4     ------> LTDC_B4
    PI5     ------> LTDC_B5
    PI6     ------> LTDC_B6
    PI7     ------> LTDC_B7 
    */
    /* The pins' edges are slowed (Peter, 23 Oct 2026, the G1 through a capture device: "line start timing varying"):
       HSYNC and VSYNC LOW, the colours MEDIUM. At HIGH they rang and moved the ground under the syncs; Peter judged
       MEDIUM for the colours the best on the monitor, LOW for them worse. */
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;                    // VSYNC and HSYNC: the slowest edges (45 kHz at most)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_13|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // PG7, LTDC_CLK: only in mode 18, which is for driving an LCD panel (Peter, 23 Oct 2026). VGA has no use for the
    // pixel clock, and driven it is a 12-150 MHz square wave beside the video lines: analog in every other mode (the
    // pins are not deinitialised between modes: HAL_LTDC_MspDeInit1 is not the HAL's hook)
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = (VideoMode == 18 ? GPIO_MODE_AF_PP : GPIO_MODE_ANALOG);
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4 
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* USER CODE BEGIN LTDC_MspInit 1 */

  /* USER CODE END LTDC_MspInit 1 */
  }
}

void HAL_LTDC_MspDeInit1(LTDC_HandleTypeDef* ltdcHandle)
{

  if(ltdcHandle->Instance==LTDC)
  {
  /* USER CODE BEGIN LTDC_MspDeInit 0 */

  /* USER CODE END LTDC_MspDeInit 0 */
    /* Peripheral clock disable */
//    __HAL_RCC_LTDC_CLK_DISABLE();
  
    /**LTDC GPIO Configuration    
    PE4     ------> LTDC_B0
    PE5     ------> LTDC_G0
    PE6     ------> LTDC_G1
    PI9     ------> LTDC_VSYNC
    PI10     ------> LTDC_HSYNC
    PF10     ------> LTDC_DE
    PC0     ------> LTDC_R5
    PH2     ------> LTDC_R0
    PH3     ------> LTDC_R1
    PB1     ------> LTDC_R6
    PH8     ------> LTDC_R2
    PH9     ------> LTDC_R3
    PH10     ------> LTDC_R4
    PG6     ------> LTDC_R7
    PG7     ------> LTDC_CLK
    PH13     ------> LTDC_G2
    PH15     ------> LTDC_G4
    PI0     ------> LTDC_G5
    PI1     ------> LTDC_G6
    PI2     ------> LTDC_G7
    PD6     ------> LTDC_B2
    PG10     ------> LTDC_G3
    PG11     ------> LTDC_B3
    PG12     ------> LTDC_B1
    PI4     ------> LTDC_B4
    PI5     ------> LTDC_B5
    PI6     ------> LTDC_B6
    PI7     ------> LTDC_B7 
    */
/*    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOI, GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_0|GPIO_PIN_1 
                          |GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6 
                          |GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_10);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0);

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_8|GPIO_PIN_9 
                          |GPIO_PIN_10|GPIO_PIN_13|GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_6|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_6);*/

  // USER CODE BEGIN LTDC_MspDeInit 1

  // USER CODE END LTDC_MspDeInit 1
  }
} 

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
