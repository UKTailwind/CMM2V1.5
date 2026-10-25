/***************************************************************************
CMM2 MMBasic
Serial.c

Handles the serial I/O  commands and functions in MMBasic..

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


#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "tusb.h"
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart6;
extern volatile int ConsoleTxBufHead;
extern volatile int ConsoleTxBufTail;

// variables for com1
int com1 = 0;														// true if COM1 is enabled
int com1_buf_size;													// size of the buffer used to receive chars
int com1_baud = 0;													// determines the baud rate
char *com1_interrupt, *com1_TX_interrupt;												// pointer to the interrupt routine
int com1_ilevel;													// number nbr of chars in the buffer for an interrupt
int com1_TX_complete = false;
unsigned char *com1Rx_buf;											// pointer to the buffer for received characters
volatile int com1Rx_head, com1Rx_tail;								// head and tail of the ring buffer for com1
unsigned char *com1Tx_buf;											// pointer to the buffer for transmitted characters
volatile int com1Tx_head, com1Tx_tail;								// head and tail of the ring buffer for com1
volatile int com1complete=1;
uint16_t Rx1Buffer;
char com1_mode;                                                     // keeps track of the settings for com4
unsigned char com1_bit9 = 0;                                        // used to track the 9th bit
extern uint32_t ticks_per_microsecond;

// variables for com2
int com2 = 0;														// true if COM2 is enabled
int com2_buf_size;													// size of the buffer used to receive chars
int com2_baud = 0;													// determines the baud rate
char *com2_interrupt, *com2_TX_interrupt;												// pointer to the interrupt routine
int com2_ilevel;													// number nbr of chars in the buffer for an interrupt
int com2_TX_complete = false;
unsigned char *com2Rx_buf;											// pointer to the buffer for received characters
volatile int com2Rx_head, com2Rx_tail;								// head and tail of the ring buffer for com2 Rx
unsigned char *com2Tx_buf;											// pointer to the buffer for transmitted characters
volatile int com2Tx_head, com2Tx_tail;								// head and tail of the ring buffer for com2 Tx
volatile int com2complete=1;
char com2_mode;                                                     // keeps track of the settings for com4
unsigned char com2_bit9 = 0;                                        // used to track the 9th bit

// variables for com3
int com3 = 0;														// true if COM2 is enabled
int com3_buf_size;													// size of the buffer used to receive chars
int com3_baud = 0;													// determines the baud rate
char *com3_interrupt, *com3_TX_interrupt;												// pointer to the interrupt routine
int com3_ilevel;													// number nbr of chars in the buffer for an interrupt
int com3_TX_complete = false;
unsigned char *com3Rx_buf;											// pointer to the buffer for received characters
volatile int com3Rx_head, com3Rx_tail;								// head and tail of the ring buffer for com2 Rx
unsigned char *com3Tx_buf;											// pointer to the buffer for transmitted characters
volatile int com3Tx_head, com3Tx_tail;								// head and tail of the ring buffer for com2 Tx
volatile int com3complete=1;
char com3_mode;                                                     // keeps track of the settings for com4
unsigned char com3_bit9 = 0;                                        // used to track the 9th bit
extern volatile int ConsoleTxBufHead;
extern volatile int ConsoleTxBufTail;

uint16_t Rx2Buffer;
extern uint8_t RxBuffer, TxBuffer;

// USB serial devices, COM4-COM7: TinyUSB's CDC host, which drives the USB-UART bridges (FTDI, CP210x, CH34x,
// PL2303) through the same calls.  COMn is the interface TinyUSB numbers n-4, in the order they were plugged in.
// The callbacks run inside tuh_task(), in the main thread, as does everything else here, so the ring buffers need no
// interrupt masking.  A device unplugged while its port is open leaves the port open; plugged back in, it is set up
// again as OPEN set it up (CdcService).  As on the PicoMite, where they are COM3-COM6.
struct s_cdccom CdcCom[CDC_PORTS];
void USBMessage(const char *m);                                     // printed once tuh_task() has returned (USBKeyboard.c)

// A port opened AS GPS: its bytes go from the ring buffer to the NMEA line assembler, as the UART interrupts put them
// (stm32h7xx_it.c), but only while no finished line waits for processgps() and none is being parsed.  A USB packet
// can hold more than one line, and the assembler keeps just one.  processgps() calls this again after each parse, as
// the PicoMite does for its Bluetooth GPS on COM7 (8c1dffc).
void CdcGpsFeed(void) {
	if(GPSchannel < CDC_FIRST_COM) return;
	struct s_cdccom *p = &CdcCom[GPSchannel - CDC_FIRST_COM];
	if(!p->open || p->Rx_buf == NULL) return;
	while(gpsready == NULL && !GPSbusy && p->Rx_head != p->Rx_tail) {
		char cc = p->Rx_buf[p->Rx_tail];
		p->Rx_tail = (p->Rx_tail + 1) % p->buf_size;
		*gpsbuf = cc;
		gpsbuf++;
		gpscount++;
		if(cc == 10 || gpscount == 127) {
			*gpsbuf = 0;
			gpscount = 0;
			if(gpscurrent) { gpscurrent = 0; gpsbuf = gpsbuf1; gpsready = gpsbuf2; }
			else { gpscurrent = 1; gpsbuf = gpsbuf2; gpsready = gpsbuf1; }
		}
	}
}

// The line coding and DTR/RTS that OPEN asked for.  The calls block (TinyUSB runs itself until the device answers);
// a request never sent (an ACM device that declares no line-state support) leaves its result XFER_RESULT_INVALID and
// is not an error.  Returns what the device refused, or NULL, and *res how it refused it.
static const char *CdcLineSetup(int idx, xfer_result_t *res) {
	struct s_cdccom *p = &CdcCom[idx];
	cdc_line_coding_t coding = {p->baud, p->stop_bits, p->parity, p->data_bits};
	tuh_cdc_rx_enable(idx, true);                                   // the device is read only while its port is open
	                                                                // (Peter, 22 Oct 2026; CLOSE stops it), armed first:
	                                                                // what it held while closed comes in during the
	*res = XFER_RESULT_INVALID;                                     // requests below, and is thrown away after them
	if(!tuh_cdc_set_line_coding(idx, &coding, NULL, (uintptr_t)res) && *res == XFER_RESULT_INVALID)
		tuh_cdc_set_baudrate(idx, p->baud, NULL, (uintptr_t)res);
	if(*res != XFER_RESULT_INVALID && *res != XFER_RESULT_SUCCESS) return "the baud rate";
	*res = XFER_RESULT_INVALID;
	tuh_cdc_set_control_line_state(idx, CDC_CONTROL_LINE_STATE_DTR | CDC_CONTROL_LINE_STATE_RTS, NULL, (uintptr_t)res);
	if(*res != XFER_RESULT_INVALID && *res != XFER_RESULT_SUCCESS) return "DTR/RTS";
	for(uint32_t t0 = tusb_time_millis_api(); tusb_time_millis_api() - t0 < 5; ) tuh_task();   // the rest of it: a
	tuh_cdc_read_clear(idx);                                        // device can send what it held in several transfers
	return NULL;
}

static const char *CdcBridge(uint16_t vid) {
	switch(vid) {
		case 0x0403: return "FTDI";
		case 0x10C4: return "CP210x";
		case 0x1A86: return "CH34x";
		case 0x067B: return "PL2303";
		default:     return "CDC";
	}
}

void tuh_cdc_mount_cb(uint8_t idx) {
	if(idx >= CDC_PORTS) return;
	char m[80];
	uint16_t vid = 0, pid = 0;
	tuh_itf_info_t info;
	if(tuh_cdc_itf_get_info(idx, &info)) tuh_vid_pid_get(info.daddr, &vid, &pid);
	strcpy(m, "USB serial device (");
	strcat(m, CdcBridge(vid));
	strcat(m, ") Connected as COM");
	IntToStr(m + strlen(m), idx + CDC_FIRST_COM, 10);
	strcat(m, "\r\n> ");
	USBMessage(m);
	if(CdcCom[idx].open) CdcCom[idx].setup = true;                  // plugged back in while open
}

void tuh_cdc_umount_cb(uint8_t idx) {
	if(idx >= CDC_PORTS) return;
	char m[48];
	strcpy(m, "USB serial device Disconnected from COM");
	IntToStr(m + strlen(m), idx + CDC_FIRST_COM, 10);
	strcat(m, "\r\n> ");
	USBMessage(m);
}

void tuh_cdc_rx_cb(uint8_t idx) {
	uint8_t buf[64];
	uint32_t count;
	struct s_cdccom *p = idx < CDC_PORTS ? &CdcCom[idx] : NULL;
	if(p == NULL || !p->open || p->Rx_buf == NULL) {                // not open: take them anyway so the device is not held up
		while(tuh_cdc_read(idx, buf, sizeof(buf)) > 0);
		return;
	}
	bool got = false;
	while((count = tuh_cdc_read(idx, buf, sizeof(buf))) > 0) {
		got = true;
		for(uint32_t i = 0; i < count; i++) {
			p->Rx_buf[p->Rx_head] = buf[i];
			p->Rx_head = (p->Rx_head + 1) % p->buf_size;
			if(p->Rx_head == p->Rx_tail) p->Rx_tail = (p->Rx_tail + 1) % p->buf_size; // full: throw away the oldest
		}
	}
	if(GPSchannel == idx + CDC_FIRST_COM) CdcGpsFeed();
	else if(got && p->interrupt != NULL && SerialRxStatus(idx + CDC_FIRST_COM) >= p->ilevel) IntSignal();
}

void tuh_cdc_tx_complete_cb(uint8_t idx) {
	if(idx >= CDC_PORTS) return;
	struct s_cdccom *p = &CdcCom[idx];
	if(p->open && p->TX_interrupt != NULL && tuh_cdc_write_available(idx) == CFG_TUH_CDC_TX_BUFSIZE) {
		p->TX_complete = true;                                      // all of it sent
		IntSignal();
	}
}

// From CheckKeyboard(), after tuh_task(): set up a device plugged back in while its port was open, and take the
// next NMEA line of a port opened AS GPS
void CdcService(void) {
	for(int i = 0; i < CDC_PORTS; i++)
		if(CdcCom[i].setup) {
			CdcCom[i].setup = false;
			xfer_result_t res;
			if(CdcCom[i].open && tuh_cdc_mounted(i)) CdcLineSetup(i, &res);
		}
	CdcGpsFeed();
}

// The interrupt due from COM4-COM7, if any (check_interrupt)
char *CdcInterrupt(void) {
	for(int i = 0; i < CDC_PORTS; i++) {
		struct s_cdccom *p = &CdcCom[i];
		if(!p->open) continue;
		if(p->interrupt != NULL && SerialRxStatus(i + CDC_FIRST_COM) >= p->ilevel) return p->interrupt;
		if(p->TX_interrupt != NULL && p->TX_complete) {
			p->TX_complete = false;
			return p->TX_interrupt;
		}
	}
	return NULL;
}

// the console's UART at its baud rate, with its 16-byte receive FIFO: HAL_UART_DeInit clears CR1 (FIFOEN with it) and
// HAL_UART_Init does not set it again, so the console had a one-byte receive register, 22 us at 460800, and lost a byte
// whenever another interrupt held its handler off longer (the G2 under OPTION PICOMITE, 23 Oct 2026: every second
// character of a line just after a program). The handler still takes a byte an interrupt (RXFNE: Size 1 is below
// the threshold's two, so UART_Start_Receive_IT enables RXFNEIE, not only RXFTIE); sending is still by TC.
static void ConsoleUartStart(UART_HandleTypeDef *huart){
	HAL_UART_DeInit(huart);
	huart->Init.BaudRate = Option.Baudrate;
	HAL_UART_Init(huart);
	HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8);
	HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8);
	HAL_UARTEx_EnableFifoMode(huart);
	HAL_UART_Receive_IT(huart, &RxBuffer, 1);
}
void start_console(void){
	  if(Option.ConsolePort==3){
		  MX_USART1_UART_Init1();
		  ConsoleUartStart(&huart1);
	  } else if(Option.ConsolePort==1){
	      ExtCfg(COM1_RX_PIN, EXT_BOOT_RESERVED, 0);
	      ExtCfg(COM1_TX_PIN, EXT_BOOT_RESERVED, 0);
		  MX_USART2_UART_Init2();
		  ConsoleUartStart(&huart2);
	  } else if(Option.ConsolePort==2){
	      ExtCfg(COM2_RX_PIN, EXT_BOOT_RESERVED, 0);
	      ExtCfg(COM2_TX_PIN, EXT_BOOT_RESERVED, 0);
		  MX_UART4_Init4();
		  ConsoleUartStart(&huart4);
	  }
}
void stop_console(void){
	  if(Option.ConsolePort==3){
			while(ConsoleTxBufTail != ConsoleTxBufHead);
			MM_Delay(2);
			HAL_UART_DeInit(&huart1);
	  } else if(Option.ConsolePort==1){
			while(ConsoleTxBufTail != ConsoleTxBufHead);
			MM_Delay(2);
			HAL_UART_DeInit(&huart2);
		    ExtCfg(COM1_RX_PIN, EXT_NOT_CONFIG, 0);
		    ExtCfg(COM1_TX_PIN, EXT_NOT_CONFIG, 0);
	  } else if(Option.ConsolePort==2){
			while(ConsoleTxBufTail != ConsoleTxBufHead);
			MM_Delay(2);
			HAL_UART_DeInit(&huart4);
		    ExtCfg(COM2_RX_PIN, EXT_NOT_CONFIG, 0);
		    ExtCfg(COM2_TX_PIN, EXT_NOT_CONFIG, 0);
	  }
}
int get_baudrate(int comport, int timeout){
    float rates[14]={110,300,600,1200,2400,4800,9600,14400,19200,38400,57600,115200,230400, 460800};
    int *t=GetTempMemory(1024),i,j,k,m;
    float a, b,c;
    int pin=0;
    switch(comport){
        case 1:
            if(com1) error("Already open");
            pin=COM1_RX_PIN;
            break;
        case 2:
            if(com2) error("Already open");
            pin=COM2_RX_PIN;
            break;
    }
    WriteCoreTimer(0); j=0;t[j]=0;
    k=PinRead(pin);
    while((ReadCoreTimer() < ticks_per_microsecond * timeout * 1000000) && (j<256)){
    	routinechecks(1);
        m=PinRead(pin);
        if(m!=k){
            t[j]=ReadCoreTimer()*10/ticks_per_microsecond;
            j++;
            k=m;
        }
    }
    if(j>=2){
        c=10000000;
        k=0;
        m=10000000;
        for(i=2;i<j;i++){ //the first pulse may be spurious so ignore
            if(t[i]-t[i-1]<m)m=t[i]-t[i-1];
        }
        a=10000000.0/(float)m;
        for(i=0;i<14;i++){
            b=a/rates[i];
            if(b<1)b=rates[i]/a;
            if(b<c){c=b;k=i;}
        }
        a=rates[k];
    } else a=0;
    return a;
}
void fun_baudrate(void){
    getargs(&ep, 3,",");
    int i= getint(argv[0] ,1 ,2), timeout;                     // (COM3's receive pin is not on a connector)
    if(argc==3)timeout=getint(argv[2],1,10);
    else timeout=1;
    iret = get_baudrate(i,timeout);
    targ = T_INT;
}
/***************************************************************************************************
Initialise the serial function including the timer and interrupts.
****************************************************************************************************/

void setupuart(UART_HandleTypeDef * huartx, USART_TypeDef  * USART_ID, int de, int inv,int s2,int parity, int b7, int baud){
	  huartx->Instance = USART_ID;
	  huartx->Init.BaudRate = baud;
	  huartx->Init.StopBits = (s2 ? UART_STOPBITS_2 :UART_STOPBITS_1);
	  if(parity){
		  huartx->Init.WordLength = (b7 ? UART_WORDLENGTH_8B :UART_WORDLENGTH_9B);
		  parity--;
		  huartx->Init.Parity = (parity ? UART_PARITY_ODD : UART_PARITY_EVEN ) ;
	  } else {
		  huartx->Init.WordLength = (b7 ? UART_WORDLENGTH_7B :UART_WORDLENGTH_8B);
		  huartx->Init.Parity = UART_PARITY_NONE;
	  }
	  huartx->Init.Mode = UART_MODE_TX_RX;
	  huartx->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	  huartx->Init.OverSampling = UART_OVERSAMPLING_8;
	  huartx->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	  huartx->Init.ClockPrescaler = UART_PRESCALER_DIV2;
	  if(inv){
		  huartx->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_TXINVERT_INIT|UART_ADVFEATURE_RXINVERT_INIT;
		  huartx->AdvancedInit.TxPinLevelInvert = UART_ADVFEATURE_TXINV_ENABLE;
		  huartx->AdvancedInit.RxPinLevelInvert = UART_ADVFEATURE_RXINV_ENABLE;
	  } else huartx->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXINVERT_INIT;
	  if(USART_ID == USART2 && de==1){
		  if (HAL_RS485Ex_Init(&huart2, UART_DE_POLARITY_HIGH, 0b11111, 0b11111) != HAL_OK)
	  	  	  {
	  		  	error("UART");
	  	  	  }
	  } else if(USART_ID == USART2 && de==2){
		  if (HAL_RS485Ex_Init(&huart2, UART_DE_POLARITY_LOW, 0b11111, 0b11111) != HAL_OK)
	  	  	  {
	  		  	error("UART");
	  	  	  }
	  } else {
		  if (HAL_UART_Init(huartx) != HAL_OK)
	  	  	  {
	  		  	error("UART");
	  	  	  }
	  }
	  if (HAL_UARTEx_SetTxFifoThreshold(huartx, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	  {
	    SystemError=1;Error_Handler();
	  }
	  if (HAL_UARTEx_SetRxFifoThreshold(huartx, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	  {
	    SystemError=1;Error_Handler();
	  }
	  if (HAL_UARTEx_EnableFifoMode(huartx) != HAL_OK)
	  {
	    SystemError=1;Error_Handler();
	  }
}
/***************************************************************************************************
Initialise the serial function including the timer and interrupts.
****************************************************************************************************/
void SerialOpen(char *spec) {
	int baud, i, inv, oc, s2, de, parity, b7, bufsize, ilevel;
	char *interrupt, *TXinterrupt;
	GPIO_InitTypeDef GPIO_InitStruct;

	getargs(&spec, 21, ":,");										// this is a macro and must be the first executable stmt
	if(argc != 2 && (argc & 0x01) == 0) error("COM specification");

    b7 = de = parity = inv = oc = s2 = false;
    for(i = 0; i < 6; i++) {
    	if(str_equal(argv[argc - 1], "OC")) { oc = true; argc -= 2; }	// get the open collector option
    	if(str_equal(argv[argc - 1], "DEP")) { de = 1; argc -= 2; }	// get the data enable option
    	if(str_equal(argv[argc - 1], "DEN")) {
    		if(de)error("DE Can't both be negative and positive");
    		de = 2;
    		argc -= 2;
    	}	// get the data enable option
    	if(str_equal(argv[argc - 1], "EVEN")) {
    		if(parity)error("Syntax");
    		else {parity = 1; argc -= 2; }	// set even parity
    	}
    	if(str_equal(argv[argc - 1], "ODD")) {
    		if(parity)error("Syntax");
    		else {parity = 2; argc -= 2; }	// set even parity
    	}
    	if(str_equal(argv[argc - 1], "S2")) { s2 = true; argc -= 2; }	// get the two stop bit option
    	if(str_equal(argv[argc - 1], "7BIT")) { b7 = true; argc -= 2; }	// set the 7 bit byte option
    	if(str_equal(argv[argc - 1], "INV")) { inv = true; argc -= 2; }	// get the invert option
    }

	if(argc < 1 || argc > 13) error("COM specification");

	if(argc >= 3 && *argv[2]) {
		baud = getinteger(argv[2]);									// get the baud rate as a number
		if(baud<1200)error("1200 baud is minimum supported");
	} else
		baud = COM_DEFAULT_BAUD_RATE;

	if(argc >= 5 && *argv[4])
		bufsize = getinteger(argv[4]);								// get the buffer size as a number
	else
		bufsize = COM_DEFAULT_BUF_SIZE;

	if(argc >= 7) {
    	IntSignal();
    	argv[6]=strupr(argv[6]);
		interrupt = GetIntAddress(argv[6]);							// get the interrupt location
	} else
		interrupt = NULL;

	if(argc >= 9) {
		ilevel = getinteger(argv[8]);								// get the buffer level for interrupt as a number
		if(ilevel < 1 || ilevel > bufsize) error("COM specification");
	} else
		ilevel = 1;

	if(argc >= 11) {
    	IntSignal();
    	argv[6]=strupr(argv[10]);
		TXinterrupt = GetIntAddress(argv[10]);							// get the interrupt location
	} else
		TXinterrupt = NULL;


	if(spec[3] == '1') {
	///////////////////////////////// this is COM1 ////////////////////////////////////

		if(com1) error("Already open");
        CheckPin(COM1_RX_PIN, CP_CHECKALL);
        CheckPin(COM1_TX_PIN, CP_CHECKALL);

 		com1_buf_size = bufsize;									// extracted from the comspec above
		com1_interrupt = interrupt;
		com1_ilevel	= ilevel;
		com1_TX_interrupt = TXinterrupt;
		com1_TX_complete = false;

		// setup for receive
		com1Rx_buf = GetMemory(com1_buf_size);						// setup the buffer
		com1Rx_head = com1Rx_tail = 0;
		ExtCfg(COM1_RX_PIN, EXT_COM_RESERVED, 0);                   // reserve the pin for com use


		// setup for transmit
		com1Tx_buf = GetMemory(TX_BUFFER_SIZE);						// setup the buffer
		com1Tx_head = com1Tx_tail = 0;
		ExtCfg(COM1_TX_PIN, EXT_COM_RESERVED, 0);
		com1_bit9 = com1_mode = 0;
        if(parity)  com1_mode |= COM1_9B;
        if(de) {
            CheckPin(COM1_EN_PIN, CP_CHECKALL);
            ExtCfg(COM1_EN_PIN, EXT_COM_RESERVED, 0);               // reserve the pin for com use
            com1_mode |= COM1_DE;
       }

        setupuart(&huart2, USART2, de, inv, s2, parity, b7, baud);
        if(oc){
        	GPIO_InitStruct.Pin = COM1_TX_Pin;
        	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        	GPIO_InitStruct.Pull = GPIO_NOPULL;
        	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        	HAL_GPIO_Init(COM1_TX_GPIO_Port, &GPIO_InitStruct);
        }
        if(Option.SerialPullup){
        	GPIO_InitStruct.Pin = COM1_RX_Pin;
        	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        	GPIO_InitStruct.Pull = GPIO_PULLUP;
        	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        	HAL_GPIO_Init(COM1_RX_GPIO_Port, &GPIO_InitStruct);
        }
        if(de){
        	GPIO_InitStruct.Pin = COM1_DE_Pin;
        	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        	HAL_GPIO_Init(COM1_DE_GPIO_Port, &GPIO_InitStruct);
        }
        HAL_NVIC_SetPriority(USART2_IRQn, IRQ_PRI(0), 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        huart2.Instance->CR1 |= USART_CR1_RXNEIE;
        com1 = true;
	}
	else if (spec[3] == '2') {
	///////////////////////////////// this is COM2 ////////////////////////////////////

		if(com2) error("Already open");
        CheckPin(COM2_RX_PIN, CP_CHECKALL);
        CheckPin(COM2_TX_PIN, CP_CHECKALL);

 		com2_buf_size = bufsize;									// extracted from the comspec above
		com2_interrupt = interrupt;
		com2_ilevel	= ilevel;
		com2_TX_interrupt = TXinterrupt;
		com2_TX_complete = false;

		// setup for receive
		com2Rx_buf = GetMemory(com2_buf_size);						// setup the buffer
		com2Rx_head = com2Rx_tail = 0;
		ExtCfg(COM2_RX_PIN, EXT_COM_RESERVED, 0);                   // reserve the pin for com use


		// setup for transmit
		com2Tx_buf = GetMemory(TX_BUFFER_SIZE);						// setup the buffer
		com2Tx_head = com2Tx_tail = 0;
		ExtCfg(COM2_TX_PIN, EXT_COM_RESERVED, 0);                   // reserve the pin for com use
		com2_bit9 = com2_mode = 0;
        if(de) {
        	error("Not available on COM2");
        }
        setupuart(&huart4, UART4, de, inv, s2, parity, b7, baud);
        if(oc){
        	GPIO_InitStruct.Pin = COM2_TX_Pin;
        	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        	GPIO_InitStruct.Pull = GPIO_NOPULL;
        	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        	GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
        	HAL_GPIO_Init(COM2_TX_GPIO_Port, &GPIO_InitStruct);
        }
        if(Option.SerialPullup){
        	GPIO_InitStruct.Pin = COM2_RX_Pin;
        	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        	GPIO_InitStruct.Pull = GPIO_PULLUP;
        	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        	GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
        	HAL_GPIO_Init(COM2_RX_GPIO_Port, &GPIO_InitStruct);
        }
        HAL_NVIC_SetPriority(UART4_IRQn, IRQ_PRI(0), 0);
        HAL_NVIC_EnableIRQ(UART4_IRQn);
        huart4.Instance->CR1 |= USART_CR1_RXNEIE;
        com2 = true;
	}
	else if (spec[3] == '3') {
///////////////////////////////// this is COM3 ////////////////////////////////////

	if(com3) error("Already open");
	if(Option.ConsolePort==3 && (Option.Console & 1 || OptionConsole & 1))error("Serial Console must be disabled");
	com3_buf_size = bufsize;									// extracted from the comspec above
	com3_interrupt = interrupt;
	com3_ilevel	= ilevel;
	com3_TX_interrupt = TXinterrupt;
	com3_TX_complete = false;

	// setup for receive
	com3Rx_buf = GetMemory(com3_buf_size);						// setup the buffer
	com3Rx_head = com3Rx_tail = 0;


	// setup for transmit
	com3Tx_buf = GetMemory(TX_BUFFER_SIZE);						// setup the buffer
	com3Tx_head = com3Tx_tail = 0;
	com3_bit9 = com3_mode = 0;
    if(de) {
    	error("Not available on COM3");
    }
    setupuart(&huart1, USART1, de, inv, s2, parity, b7, baud);
    if(oc){
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if(Option.SerialPullup){
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    HAL_NVIC_SetPriority(USART1_IRQn, IRQ_PRI(0), 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    huart1.Instance->CR1 |= USART_CR1_RXNEIE;
    com3 = true;
	} else if (spec[3] >= '0' + CDC_FIRST_COM && spec[3] < '0' + CDC_FIRST_COM + CDC_PORTS) {
	///////////////////////////////// this is COM4-COM7, a USB serial device ////////////////////////////////////
		int comnbr = spec[3] - '0', idx = comnbr - CDC_FIRST_COM;
		struct s_cdccom *p = &CdcCom[idx];
		char m[64];
		if(p->open) error("Already open");
		if(oc || de || inv) error("Not available on COM%", comnbr);
		if(!tuh_cdc_mounted(idx)) error("No USB serial device for COM%", comnbr);
		p->buf_size = bufsize;
		p->interrupt = interrupt;
		p->ilevel = ilevel;
		p->TX_interrupt = TXinterrupt;
		p->TX_complete = false;
		p->Rx_buf = GetMemory(bufsize);
		p->Rx_head = p->Rx_tail = 0;
		p->baud = baud;
		p->stop_bits = s2 ? CDC_LINE_CODING_STOP_BITS_2 : CDC_LINE_CODING_STOP_BITS_1;
		p->parity = parity == 1 ? CDC_LINE_CODING_PARITY_EVEN : parity == 2 ? CDC_LINE_CODING_PARITY_ODD : CDC_LINE_CODING_PARITY_NONE;
		p->data_bits = b7 ? 7 : 8;
		p->setup = false;
		p->open = true;                                             // (CdcLineSetup arms the device's IN)
		xfer_result_t res;
		const char *refused = CdcLineSetup(idx, &res);
		p->Rx_head = p->Rx_tail = 0;                                // nothing from before it was opened
		if(refused) {                                               // undone without asking the device anything more
			p->open = false;
			tuh_cdc_rx_enable(idx, false);
			p->interrupt = p->TX_interrupt = NULL;
			FreeMemorySafe((void *)&p->Rx_buf);
			strcpy(m, "COM");
			IntToStr(m + strlen(m), comnbr, 10);
			strcat(m, ": the USB device did not accept ");
			strcat(m, refused);
			strcat(m, res == XFER_RESULT_STALLED ? " (it stalled)" : res == XFER_RESULT_TIMEOUT ? " (no answer)" : " (it failed)");
			error(m);
		}
	} else error("Invalid COM port");
}




/***************************************************************************************************
Close a serial port.
****************************************************************************************************/
void SerialClose(int comnbr) {

	if(comnbr == 1 && com1) {
		HAL_UART_DeInit(&huart2);
		if(com1_mode & COM1_DE) {
			ExtCfg(COM1_EN_PIN, EXT_NOT_CONFIG, 0);
		}
		com1 = false;
		com1_interrupt = NULL;
        PinSetBit(COM1_RX_PIN, CNPUCLR);                            // clear the pullup or pulldown on Rx
        PinSetBit(COM1_RX_PIN, CNPDCLR);
		ExtCfg(COM1_RX_PIN, EXT_NOT_CONFIG, 0);
		ExtCfg(COM1_TX_PIN, EXT_NOT_CONFIG, 0);
		FreeMemorySafe((void *)&com1Rx_buf);
		FreeMemorySafe((void *)&com1Tx_buf);
	}

	else if(comnbr == 2 && com2) {
		HAL_UART_DeInit(&huart4);
		com2 = false;
		com2_interrupt = NULL;
        PinSetBit(COM2_RX_PIN, CNPUCLR);                            // clear the pullup or pulldown on Rx
        PinSetBit(COM2_RX_PIN, CNPDCLR);
		ExtCfg(COM2_RX_PIN, EXT_NOT_CONFIG, 0);
		ExtCfg(COM2_TX_PIN, EXT_NOT_CONFIG, 0);
		FreeMemorySafe((void *)&com2Rx_buf);
		FreeMemorySafe((void *)&com2Tx_buf);
	}

	else if(comnbr == 3 && com3) {
		HAL_UART_DeInit(&huart1);
		com3 = false;
		com3_interrupt = NULL;
		FreeMemorySafe((void *)&com3Rx_buf);
		FreeMemorySafe((void *)&com3Tx_buf);
	}

	else if(comnbr >= CDC_FIRST_COM && comnbr < CDC_FIRST_COM + CDC_PORTS && CdcCom[comnbr - CDC_FIRST_COM].open) {
		int idx = comnbr - CDC_FIRST_COM;
		struct s_cdccom *p = &CdcCom[idx];
		p->open = false;
		p->interrupt = p->TX_interrupt = NULL;
		p->setup = false;
		if(tuh_cdc_mounted(idx)) {
			tuh_cdc_rx_enable(idx, false);                          // the device's IN stopped: not read while closed
			tuh_cdc_set_control_line_state(idx, 0, NULL, 0);        // DTR and RTS down
		}
		FreeMemorySafe((void *)&p->Rx_buf);
	}
}



/***************************************************************************************************
Add a character to the serial output buffer.
****************************************************************************************************/
unsigned char SerialPutchar(int comnbr, unsigned char c) {
	if(comnbr == 1) {
        int empty=(huart2.Instance->ICR & USART_ICR_TCCF) | !(huart2.Instance->CR1 & USART_CR1_TCIE) ;
		while(com1Tx_tail == ((com1Tx_head + 1) % TX_BUFFER_SIZE)); //wait if buffer full
		com1Tx_buf[com1Tx_head] = c;							// add the char
		com1Tx_head = (com1Tx_head + 1) % TX_BUFFER_SIZE;		   // advance the head of the queue
		if(empty){
	        huart2.Instance->CR1 |= USART_CR1_TCIE;
		}
	}
	else if(comnbr == 2) {
        int empty=(huart4.Instance->ICR & USART_ICR_TCCF) | !(huart4.Instance->CR1 & USART_CR1_TCIE) ;
		while(com2Tx_tail == ((com2Tx_head + 1) % TX_BUFFER_SIZE)); //wait if buffer full
		com2Tx_buf[com2Tx_head] = c;							// add the char
		com2Tx_head = (com2Tx_head + 1) % TX_BUFFER_SIZE;		   // advance the head of the queue
		if(empty){
	        huart4.Instance->CR1 |= USART_CR1_TCIE;
		}
	}
	else if(comnbr == 3) {
        int empty=(huart1.Instance->ICR & USART_ICR_TCCF) | !(huart1.Instance->CR1 & USART_CR1_TCIE) ;
		while(com3Tx_tail == ((com3Tx_head + 1) % TX_BUFFER_SIZE)); //wait if buffer full
		com3Tx_buf[com3Tx_head] = c;							// add the char
		com3Tx_head = (com3Tx_head + 1) % TX_BUFFER_SIZE;		   // advance the head of the queue
		if(empty){
	        huart1.Instance->CR1 |= USART_CR1_TCIE;
		}
	}
	else if(comnbr >= CDC_FIRST_COM && comnbr < CDC_FIRST_COM + CDC_PORTS) {
		int idx = comnbr - CDC_FIRST_COM;                           // written as it comes, a byte lost while unplugged
		while(tuh_cdc_mounted(idx) && tuh_cdc_write_available(idx) == 0) {
			tuh_task();                                             // the device takes what was written
			CheckAbort();
		}
		if(tuh_cdc_mounted(idx)) {
			tuh_cdc_write(idx, &c, 1);
			tuh_cdc_write_flush(idx);
		}
	}
	return c;
}



/***************************************************************************************************
Get the status the serial receive buffer.
Returns the number of characters waiting in the buffer
****************************************************************************************************/
int SerialRxStatus(int comnbr) {
	int i = 0;
	if(comnbr == 1) {
	    huart2.Instance->CR1 &= ~USART_CR1_RXNEIE;
		i = com1Rx_head - com1Rx_tail;
	    huart2.Instance->CR1 |= USART_CR1_RXNEIE;
		if(i < 0) i += com1_buf_size;
	}
	else if(comnbr == 2) {
	    huart4.Instance->CR1 &= ~USART_CR1_RXNEIE;
		i = com2Rx_head - com2Rx_tail;
	    huart4.Instance->CR1 |= USART_CR1_RXNEIE;
		if(i < 0) i += com2_buf_size;
	}
	else if(comnbr == 3) {
	    huart1.Instance->CR1 &= ~USART_CR1_RXNEIE;
		i = com3Rx_head - com3Rx_tail;
	    huart1.Instance->CR1 |= USART_CR1_RXNEIE;
		if(i < 0) i += com3_buf_size;
	}
	else if(comnbr >= CDC_FIRST_COM && comnbr < CDC_FIRST_COM + CDC_PORTS) {
		struct s_cdccom *p = &CdcCom[comnbr - CDC_FIRST_COM];
		if(p->open && p->Rx_buf != NULL) {
			i = p->Rx_head - p->Rx_tail;
			if(i < 0) i += p->buf_size;
		}
	}

	return i;
}


/***************************************************************************************************
Get the status the serial transmit buffer.
Returns the number of characters waiting in the buffer
****************************************************************************************************/
int SerialTxStatus(int comnbr) {
	int i = 0;
	if(comnbr == 0) {
		i = ConsoleTxBufHead - ConsoleTxBufTail;
		if(i < 0) i += CONSOLE_TX_BUF_SIZE;
	}
	if(comnbr == 1) {
		i = com1Tx_head - com1Tx_tail;
		if(i < 0) i += TX_BUFFER_SIZE;
	}
	else if(comnbr == 2) {
		i = com2Tx_head - com2Tx_tail;
		if(i < 0) i += TX_BUFFER_SIZE;
	}
	else if(comnbr == 3) {
		i = com3Tx_head - com3Tx_tail;
		if(i < 0) i += TX_BUFFER_SIZE;
	}
	else if(comnbr >= CDC_FIRST_COM && comnbr < CDC_FIRST_COM + CDC_PORTS) {
		int idx = comnbr - CDC_FIRST_COM;
		if(tuh_cdc_mounted(idx)) i = CFG_TUH_CDC_TX_BUFSIZE - tuh_cdc_write_available(idx);   // (TX_BUFFER_SIZE, tusb_config.h)
	}
	return i;
}



/***************************************************************************************************
Get a character from the serial receive buffer.
Note that this is returned as an integer and -1 means that there are no characters available
****************************************************************************************************/
int SerialGetchar(int comnbr) {
	int c;
    c = -1;                                                         // -1 is no data
	if(comnbr == 1) {
	    huart2.Instance->CR1 &= ~USART_CR1_RXNEIE;
		if(com1Rx_head != com1Rx_tail) {                            // if the queue has something in it
			c = com1Rx_buf[com1Rx_tail];                            // get the char
 			com1Rx_tail = (com1Rx_tail + 1) % com1_buf_size;        // and remove from the buffer
		}
	    huart2.Instance->CR1 |= USART_CR1_RXNEIE;
	}
	else if(comnbr == 2) {

	    huart4.Instance->CR1 &= ~USART_CR1_RXNEIE;
		if(com2Rx_head != com2Rx_tail) {                            // if the queue has something in it
			c = com2Rx_buf[com2Rx_tail];                            // get the char
 			com2Rx_tail = (com2Rx_tail + 1) % com2_buf_size;        // and remove from the buffer
		}
	    huart4.Instance->CR1 |= USART_CR1_RXNEIE;
	}
	else if(comnbr == 3) {

	    huart1.Instance->CR1 &= ~USART_CR1_RXNEIE;
		if(com3Rx_head != com3Rx_tail) {                            // if the queue has something in it
			c = com3Rx_buf[com3Rx_tail];                            // get the char
 			com3Rx_tail = (com3Rx_tail + 1) % com3_buf_size;        // and remove from the buffer
		}
	    huart1.Instance->CR1 |= USART_CR1_RXNEIE;
	}
	else if(comnbr >= CDC_FIRST_COM && comnbr < CDC_FIRST_COM + CDC_PORTS) {
		struct s_cdccom *p = &CdcCom[comnbr - CDC_FIRST_COM];
		if(p->open && p->Rx_buf != NULL && p->Rx_head != p->Rx_tail) {
			c = p->Rx_buf[p->Rx_tail];
			p->Rx_tail = (p->Rx_tail + 1) % p->buf_size;
		}
	}
	return c;
}

