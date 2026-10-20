/************************************************************************************************************************
Micromite

I2C.c

Routines to handle I2C access.

Copyright 2011 Gerard Sexton
Copyright 2011-2020 Geoff Graham
Copyright 2019-2020 Peter Mather

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  *  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  *  Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************************************************************************************/


#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
extern I2C_HandleTypeDef hi2c1, hi2c2, hi2c4;

// Declare functions
void i2cEnable(char *p);
void i2cDisable(char *p);
void i2cSend(char *p);
void i2cReceive(char *p);
void i2c_disable(void);
void i2c_enable(int bps);
void i2c_masterCommand(int timer, unsigned char *rbuf);
void i2cCheck(char *p);
//
void i2c2Enable(char *p);
void i2c2Disable(char *p);
void i2c2Send(char *p);
void i2c2Receive(char *p);
void i2c2_disable(void);
void i2c2_enable(int bps);
void i2c2_masterCommand(int timer, unsigned char *rbuf);
void i2c2Check(char *p);
//
void i2c3Enable(char *p);
void i2c3Disable(char *p);
void i2c3Send(char *p);
void i2c3Receive(char *p);
void i2c3_disable(void);
void i2c3_enable(int bps);
void i2c3_masterCommand(int timer, unsigned char *rbuf);
void i2c3Check(char *p);
//
static unsigned int I2C_Addr;										// I2C device address
static volatile unsigned int I2C_Sendlen;							// length of the master send buffer
static volatile unsigned int I2C_Rcvlen;							// length of the master receive buffer
static unsigned char I2C_Send_Buffer[256];                                   // I2C send buffer
unsigned int I2C1_enabled;									// I2C enable marker
unsigned int I2C_Timeout;									// master timeout value
static unsigned int I2C_Status;										// status flags
int mmI2Cvalue;
	// value of MM.I2C
	static unsigned int I2C2_Addr;										// I2C device address
	static volatile unsigned int I2C2_Sendlen;							// length of the master send buffer
	static volatile unsigned int I2C2_Rcvlen;							// length of the master receive buffer
	unsigned int I2C2_enabled;									// I2C enable marker
	unsigned int I2C2_Timeout;									// master timeout value
	static unsigned int I2C2_Status;										// status flags
	// value of MM.I2C
	static unsigned int I2C3_Addr;										// I2C device address
	static volatile unsigned int I2C3_Sendlen;							// length of the master send buffer
	static volatile unsigned int I2C3_Rcvlen;							// length of the master receive buffer
	unsigned int I2C3_enabled;									// I2C enable marker
	unsigned int I2C3_Timeout;									// master timeout value
	static unsigned int I2C3_Status;										// status flags
static int I2C_Held, I2C2_Held, I2C3_Held;                         // a bus kept by option 1 (i2c_transfer)
static void i2c_release(I2C_HandleTypeDef *h, int *held);
extern RTC_HandleTypeDef hrtc;
void cmd_i2c1(char *i2c);
void cmd_i2c2(char *i2c);
void cmd_i2c3(char *i2c);
/*******************************************************************************************
							  I2C related commands in MMBasic
                              ===============================
These are the functions responsible for executing the I2C related commands in MMBasic
They are supported by utility functions that are grouped at the end of this file

********************************************************************************************/
void cmd_i2c(void){ cmd_i2c1(cmdline); }                        // I2C, I2C2 and I2C3 are commands of their own (they were I2C 1, 2 and 3)
void cmd_i2c_2(void){ cmd_i2c2(cmdline); }
void cmd_i2c_3(void){ cmd_i2c3(cmdline); }
void cmd_i2c1(char *i2c) {
    char *p;//, *pp;

    if((p = checkstring(i2c, "OPEN")) != NULL)
        i2cEnable(p);
    else if((p = checkstring(i2c, "CLOSE")) != NULL)
        i2cDisable(p);
    else if((p = checkstring(i2c, "WRITE")) != NULL)
        i2cSend(p);
    else if((p = checkstring(i2c, "READ")) != NULL)
        i2cReceive(p);
    else if((p = checkstring(i2c, "CHECK")) != NULL)
        i2cCheck(p);
    else
        error("Unknown command");
}
void cmd_i2c2(char *i2c) {
    char *p;//, *pp;

    if((p = checkstring(i2c, "OPEN")) != NULL)
        i2c2Enable(p);
    else if((p = checkstring(i2c, "CLOSE")) != NULL)
        i2c2Disable(p);
    else if((p = checkstring(i2c, "WRITE")) != NULL)
        i2c2Send(p);
    else if((p = checkstring(i2c, "READ")) != NULL)
        i2c2Receive(p);
    else if((p = checkstring(i2c, "CHECK")) != NULL)
        i2c2Check(p);
    else
        error("Unknown command");
}

void cmd_i2c3(char *i2c) {
    char *p;//, *pp;

    if((p = checkstring(i2c, "OPEN")) != NULL)
        i2c3Enable(p);
    else if((p = checkstring(i2c, "CLOSE")) != NULL)
        i2c3Disable(p);
    else if((p = checkstring(i2c, "WRITE")) != NULL)
        i2c3Send(p);
    else if((p = checkstring(i2c, "READ")) != NULL)
        i2c3Receive(p);
    else if((p = checkstring(i2c, "CHECK")) != NULL)
        i2c3Check(p);
    else
        error("Unknown command");
}
/*
 * *Initialize RTC and set the Time and Date
*/
void RtcGetTime(void){
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	int localsecs;
	if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		error("RTC hardware error");
	}
	localsecs=sTime.Seconds;
	if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		error("RTC hardware error");
	}
	milliseconds=1000-((sTime.SubSeconds*1000)/sTime.SecondFraction);
	if(sTime.SubSeconds<5){
		do {
			if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
			{
				error("RTC hardware error");
			}
			if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
			{
				error("RTC hardware error");
			}
			milliseconds=1000-((sTime.SubSeconds*1000)/sTime.SecondFraction);
		} while(localsecs==sTime.Seconds);
	}
	second = sTime.Seconds;
	minute = sTime.Minutes;
	hour = sTime.Hours;
	day = sDate.Date;
	month = sDate.Month;
	year = sDate.Year+2000;
	day_of_week=sDate.WeekDay;
}

// enable the I2C1 module - master mode
void i2cEnable(char *p) {
	int speed, timeout;
	getargs(&p, 3, ",");
	if(argc != 3) error("Invalid syntax");
	speed = getinteger(argv[0]);
	if(!(speed ==100 || speed == 400 || speed==1000)) error("Valid speeds 100, 400, 1000");
	timeout = getinteger(argv[2]);
	if(timeout < 0 || (timeout > 0 && timeout < 100)) error("Number out of bounds" );
	if(I2C1_enabled || nun1 || classic1 || mouse1) error("I2C already OPEN");
	I2C_Timeout = timeout;
	i2c_enable(speed);

}
// enable the I2C2 module - master mode
void i2c2Enable(char *p) {
	int speed, timeout;
	getargs(&p, 3, ",");
	if(argc != 3) error("Invalid syntax");
	speed = getinteger(argv[0]);
	if(!(speed ==100 || speed == 400 || speed==1000)) error("Valid speeds 100, 400, 1000");
	timeout = getinteger(argv[2]);
	if(timeout < 0 || (timeout > 0 && timeout < 100)) error("Number out of bounds" );
	if(I2C2_enabled || nun2 || classic2 || mouse2) error("I2C already OPEN");
	I2C2_Timeout = timeout;
	i2c2_enable(speed);

}
// enable the I2C3 module - master mode
void i2c3Enable(char *p) {
	int speed, timeout;
	getargs(&p, 3, ",");
	if(argc != 3) error("Invalid syntax");
	speed = getinteger(argv[0]);
	if(!(speed ==100 || speed == 400 || speed==1000)) error("Valid speeds 100, 400, 1000");
	timeout = getinteger(argv[2]);
	if(timeout < 0 || (timeout > 0 && timeout < 100)) error("Number out of bounds" );
	if(I2C3_enabled || nun3 || classic3 || mouse3) error("I2C already OPEN");
	I2C3_Timeout = timeout;
	i2c3_enable(speed);

}


// disable the I2C1 module - master mode
void i2cDisable(char *p) {
	if(!I2C1_enabled)error("Not open");
	if(nun1 || classic1 || mouse1)error("In use for Controller");
	i2c_disable();
}

// disable the I2C2 module - master mode
void i2c2Disable(char *p) {
	if(!I2C2_enabled)error("Not open");
	if(nun2 || classic2 || mouse2)error("In use for Controller");
	i2c2_disable();
}

// disable the I2C2 module - master mode
void i2c3Disable(char *p) {
	if(!I2C3_enabled)error("Not open");
	if(nun3 || classic3 || mouse3)error("In use for Controller");
	i2c3_disable();
}
// send data to an I2C slave - master mode
void i2cSend(char *p) {
	int addr, options, sendlen, i;
	unsigned int buf[256];

	getargs(&p, 99, ",");
	if(!(argc & 0x01) || (argc < 7)) error("Invalid syntax");
	if(!(I2C1_enabled || nun1 || classic1 || mouse1))error("I2C not open");
	addr = getinteger(argv[0]);
	options = getint(argv[2],0,3);                                  // (as the PicoMite's: bit 1 is taken, not used)
	I2C_Status = 0;
	if(options & 0x01) I2C_Status = I2C_Status_BusHold;
	I2C_Addr = addr;
	sendlen = getint(argv[4], 1, 256);
	GetCommsTxData(argv, argc, 6, sendlen, buf);                    // a list, a whole array or a string (MMBasic.c)
	for(i = 0; i < sendlen; i++) I2C_Send_Buffer[i] = buf[i];
	I2C_Sendlen = sendlen;
	I2C_Rcvlen = 0;

	i2c_masterCommand(1, NULL);
}
// send data to an I2C slave - master mode
void i2c2Send(char *p) {
	int addr, options, sendlen, i;
	unsigned int buf[256];

	getargs(&p, 99, ",");
	if(!(argc & 0x01) || (argc < 7)) error("Invalid syntax");
	if(!(I2C2_enabled || nun2 || classic2 || mouse2))error("I2C not open");
	addr = getinteger(argv[0]);
	options = getint(argv[2],0,3);                                  // (as the PicoMite's: bit 1 is taken, not used)
	I2C2_Status = 0;
	if(options & 0x01) I2C2_Status = I2C_Status_BusHold;
	I2C2_Addr = addr;
	sendlen = getint(argv[4], 1, 256);
	GetCommsTxData(argv, argc, 6, sendlen, buf);                    // a list, a whole array or a string (MMBasic.c)
	for(i = 0; i < sendlen; i++) I2C_Send_Buffer[i] = buf[i];
	I2C2_Sendlen = sendlen;
	I2C2_Rcvlen = 0;

	i2c2_masterCommand(1, NULL);
}
// send data to an I2C slave - master mode
void i2c3Send(char *p) {
	int addr, options, sendlen, i;
	unsigned int buf[256];

	getargs(&p, 99, ",");
	if(!(argc & 0x01) || (argc < 7)) error("Invalid syntax");
	if(!(I2C3_enabled || nun3 || classic3 || mouse3))error("I2C not open");
	addr = getinteger(argv[0]);
	options = getint(argv[2],0,3);                                  // (as the PicoMite's: bit 1 is taken, not used)
	I2C3_Status = 0;
	if(options & 0x01) I2C3_Status = I2C_Status_BusHold;
	I2C3_Addr = addr;
	sendlen = getint(argv[4], 1, 256);
	GetCommsTxData(argv, argc, 6, sendlen, buf);                    // a list, a whole array or a string (MMBasic.c)
	for(i = 0; i < sendlen; i++) I2C_Send_Buffer[i] = buf[i];
	I2C3_Sendlen = sendlen;
	I2C3_Rcvlen = 0;

	i2c3_masterCommand(1, NULL);
}

static void i2c_wait_idle(I2C_HandleTypeDef *h);
void i2cCheck(char *p) {
	int addr;
	getargs(&p, 1, ",");
	if(!(I2C1_enabled || nun1 || classic1 || mouse1))error("I2C not open");
	addr = getinteger(argv[0]);
    if(addr<1 || addr>0x7F)error("Invalid I2C address");
    addr<<=1;
	mT4IntEnable(0);                                                // as WRITE and READ: no controller transfer starts during it,
	i2c_release(&hi2c1, &I2C_Held);                                // (a kept bus let go first)
	i2c_wait_idle(&hi2c1);                                         // and one under way is waited for
	mmI2Cvalue = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)addr, 2, 10);
	mT4IntEnable(1);
}
void i2c2Check(char *p) {
	int addr;
	getargs(&p, 1, ",");
	if(!(I2C2_enabled || nun2 || classic2 || mouse2))error("I2C not open");
	addr = getinteger(argv[0]);
    if(addr<1 || addr>0x7F)error("Invalid I2C address");
    addr<<=1;
	mT4IntEnable(0);                                                // as WRITE and READ: no controller transfer starts during it,
	i2c_release(&hi2c4, &I2C2_Held);                                // (a kept bus let go first)
	i2c_wait_idle(&hi2c4);                                         // and one under way is waited for
	mmI2Cvalue = HAL_I2C_IsDeviceReady(&hi2c4, (uint16_t)addr, 2, 10);
	mT4IntEnable(1);
}
void i2c3Check(char *p) {
	int addr;
	getargs(&p, 1, ",");
	if(!(I2C3_enabled || nun3 || classic3 || mouse3))error("I2C not open");
	addr = getinteger(argv[0]);
    if(addr<1 || addr>0x7F)error("Invalid I2C address");
    addr<<=1;
	mT4IntEnable(0);                                                // as WRITE and READ: no controller transfer starts during it,
	i2c_release(&hi2c2, &I2C3_Held);                                // (a kept bus let go first)
	i2c_wait_idle(&hi2c2);                                         // and one under way is waited for
	mmI2Cvalue = HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)addr, 2, 10);
	mT4IntEnable(1);
}
// receive data from an I2C slave - master mode
void i2cReceive(char *p) {
	int addr, options, rcvlen, i;
	unsigned char *raw;
	unsigned int *buf;
	CommsRxDest dest;
	getargs(&p, 99, ",");
	if(!(argc & 0x01) || (argc < 7)) error("Invalid syntax");
	if(!(I2C1_enabled || nun1 || classic1 || mouse1))error("I2C not open");
	addr = getinteger(argv[0]);
	options = getint(argv[2],0,1);
	I2C_Status = 0;
	if(options & 0x01) I2C_Status = I2C_Status_BusHold;
	I2C_Addr = addr;
	rcvlen = getinteger(argv[4]);
	if(rcvlen < 1) error("Number out of bounds");
	GetCommsRxDest(argv, argc, 6, rcvlen, &dest);                   // a list, a whole array or a string (MMBasic.c)
	raw = GetTempMemory(rcvlen);
	buf = GetTempMemory(rcvlen * sizeof(unsigned int));
	I2C_Rcvlen = rcvlen;
	I2C_Sendlen = 0;

	i2c_masterCommand(1, raw);
	for(i = 0; i < rcvlen; i++) buf[i] = raw[i];
	PutCommsRxData(&dest, buf);
}
// receive data from an I2C slave - master mode
void i2c2Receive(char *p) {
	int addr, options, rcvlen, i;
	unsigned char *raw;
	unsigned int *buf;
	CommsRxDest dest;
	getargs(&p, 99, ",");
	if(!(argc & 0x01) || (argc < 7)) error("Invalid syntax");
	if(!(I2C2_enabled || nun2 || classic2 || mouse2))error("I2C not open");
	addr = getinteger(argv[0]);
	options = getint(argv[2],0,1);
	I2C2_Status = 0;
	if(options & 0x01) I2C2_Status = I2C_Status_BusHold;
	I2C2_Addr = addr;
	rcvlen = getinteger(argv[4]);
	if(rcvlen < 1) error("Number out of bounds");
	GetCommsRxDest(argv, argc, 6, rcvlen, &dest);                   // a list, a whole array or a string (MMBasic.c)
	raw = GetTempMemory(rcvlen);
	buf = GetTempMemory(rcvlen * sizeof(unsigned int));
	I2C2_Rcvlen = rcvlen;
	I2C2_Sendlen = 0;

	i2c2_masterCommand(1, raw);
	for(i = 0; i < rcvlen; i++) buf[i] = raw[i];
	PutCommsRxData(&dest, buf);
}
// receive data from an I2C slave - master mode
void i2c3Receive(char *p) {
	int addr, options, rcvlen, i;
	unsigned char *raw;
	unsigned int *buf;
	CommsRxDest dest;
	getargs(&p, 99, ",");
	if(!(argc & 0x01) || (argc < 7)) error("Invalid syntax");
	if(!(I2C3_enabled || nun3 || classic3 || mouse3))error("I2C not open");
	addr = getinteger(argv[0]);
	options = getint(argv[2],0,1);
	I2C3_Status = 0;
	if(options & 0x01) I2C3_Status = I2C_Status_BusHold;
	I2C3_Addr = addr;
	rcvlen = getinteger(argv[4]);
	if(rcvlen < 1) error("Number out of bounds");
	GetCommsRxDest(argv, argc, 6, rcvlen, &dest);                   // a list, a whole array or a string (MMBasic.c)
	raw = GetTempMemory(rcvlen);
	buf = GetTempMemory(rcvlen * sizeof(unsigned int));
	I2C3_Rcvlen = rcvlen;
	I2C3_Sendlen = 0;

	i2c3_masterCommand(1, raw);
	for(i = 0; i < rcvlen; i++) buf[i] = raw[i];
	PutCommsRxData(&dest, buf);
}

/**************************************************************************************************
Enable the I2C1 module - master mode
***************************************************************************************************/
void i2c_enable(int bps) {
    HAL_I2C_DeInit(&hi2c1);
    ExtCfg(P_I2C_SDA, EXT_NOT_CONFIG, 0);
    ExtCfg(P_I2C_SCL, EXT_NOT_CONFIG, 0);
    PinSetBit(P_I2C_SDA, TRISSET);
    if(PinRead(P_I2C_SDA) == 0) {
    	int i;
        // it appears as if something is holding SLA low
        // try pulsing the clock to get rid of it
        PinSetBit(P_I2C_SCL, TRISCLR);
        PinSetBit(P_I2C_SCL, LATCLR);
        for(i = 0; i < 20; i++) {
            uSec(25);
            PinSetBit(P_I2C_SCL, LATINV);
            if(PinRead(P_I2C_SDA) == 0) break;
        }
        PinSetBit(P_I2C_SCL, TRISSET);
    }
    ExtCfg(P_I2C_SDA, EXT_COM_RESERVED, 0);
    ExtCfg(P_I2C_SCL, EXT_COM_RESERVED, 0);
    if(bps==100) hi2c1.Init.Timing = 0x10707DBC;
    if(bps==400) hi2c1.Init.Timing = 0x00602173;
    if(bps==1000)hi2c1.Init.Timing = 0x00300B29;
    HAL_I2C_Init(&hi2c1);
	I2C1_enabled=1;
}
void i2c2_enable(int bps) {
    HAL_I2C_DeInit(&hi2c4);
    ExtCfg(P_I2C2_SDA, EXT_NOT_CONFIG, 0);
    ExtCfg(P_I2C2_SCL, EXT_NOT_CONFIG, 0);
    PinSetBit(P_I2C2_SDA, TRISSET);
    if(PinRead(P_I2C2_SDA) == 0) {
        int i;
        // it appears as if something is holding SLA low
        // try pulsing the clock to get rid of it
        PinSetBit(P_I2C2_SCL, TRISCLR);
        PinSetBit(P_I2C2_SCL, LATCLR);
        for(i = 0; i < 20; i++) {
           uSec(25);
           PinSetBit(P_I2C2_SCL, LATINV);
           if(PinRead(P_I2C2_SDA) == 0) break;
        }
        PinSetBit(P_I2C2_SCL, TRISSET);
    }
    ExtCfg(P_I2C2_SDA, EXT_COM_RESERVED, 0);
    ExtCfg(P_I2C2_SCL, EXT_COM_RESERVED, 0);
    if(bps==100) hi2c4.Init.Timing = 0x10707DBC;
    if(bps==400) hi2c4.Init.Timing = 0x00602173;
    if(bps==1000)hi2c4.Init.Timing = 0x00300B29;
    HAL_I2C_Init(&hi2c4);
	I2C2_enabled=1;
}
void i2c3_enable(int bps) {
    HAL_I2C_DeInit(&hi2c2);
    ExtCfg(P_I2C3_SDA, EXT_NOT_CONFIG, 0);
    ExtCfg(P_I2C3_SCL, EXT_NOT_CONFIG, 0);
    PinSetBit(P_I2C3_SDA, TRISSET);
    if(PinRead(P_I2C3_SDA) == 0) {
        int i;
        // it appears as if something is holding SLA low
        // try pulsing the clock to get rid of it
        PinSetBit(P_I2C3_SCL, TRISCLR);
        PinSetBit(P_I2C3_SCL, LATCLR);
        for(i = 0; i < 20; i++) {
           uSec(25);
           PinSetBit(P_I2C3_SCL, LATINV);
           if(PinRead(P_I2C3_SDA) == 0) break;
        }
        PinSetBit(P_I2C3_SCL, TRISSET);
    }
    ExtCfg(P_I2C3_SDA, EXT_COM_RESERVED, 0);
    ExtCfg(P_I2C3_SCL, EXT_COM_RESERVED, 0);
    if(bps==100) hi2c2.Init.Timing = 0x10707DBC;
    if(bps==400) hi2c2.Init.Timing = 0x00602173;
    if(bps==1000)hi2c2.Init.Timing = 0x00300B29;
    HAL_I2C_Init(&hi2c2);
	I2C3_enabled=1;
}

/**************************************************************************************************
Disable the I2C1 module - master mode
***************************************************************************************************/
void i2c_disable(void) {
    I2C_Status = I2C_Status_Disable;
	I2C_Sendlen = 0;												// length of the master send buffer
	I2C_Rcvlen = 0;													// length of the master receive buffer
	I2C_Addr = 0;													// I2C device address
	I2C_Timeout = 0;												// master timeout value
	i2c_release(&hi2c1, &I2C_Held);
	HAL_I2C_DeInit(&hi2c1);
	I2C1_enabled = 0;
    ExtCfg(P_I2C_SDA, EXT_NOT_CONFIG, 0);
    ExtCfg(P_I2C_SCL, EXT_NOT_CONFIG, 0);
}
void i2c2_disable(void) {
    I2C2_Status = I2C_Status_Disable;
	I2C2_Sendlen = 0;												// length of the master send buffer
	I2C2_Rcvlen = 0;													// length of the master receive buffer
	I2C2_Addr = 0;													// I2C device address
	I2C2_Timeout = 0;												// master timeout value
	i2c_release(&hi2c4, &I2C2_Held);
	HAL_I2C_DeInit(&hi2c4);
	I2C2_enabled = 0;
    ExtCfg(P_I2C2_SDA, EXT_NOT_CONFIG, 0);
    ExtCfg(P_I2C2_SCL, EXT_NOT_CONFIG, 0);
}
void i2c3_disable(void) {
    I2C3_Status = I2C_Status_Disable;
	I2C3_Sendlen = 0;												// length of the master send buffer
	I2C3_Rcvlen = 0;													// length of the master receive buffer
	I2C3_Addr = 0;													// I2C device address
	I2C3_Timeout = 0;												// master timeout value
	i2c_release(&hi2c2, &I2C3_Held);
	HAL_I2C_DeInit(&hi2c2);
	I2C3_enabled = 0;
    ExtCfg(P_I2C3_SDA, EXT_NOT_CONFIG, 0);
    ExtCfg(P_I2C3_SCL, EXT_NOT_CONFIG, 0);
}
// A program's transfer on a bus that a Wii controller or I2C mouse is polled on (from the 1 ms tick, Timers.c): the
// caller has stopped the polling starting another (mT4IntEnable(0)); this waits for one already under way to finish,
// up to 5 ms. Without it the HAL said the handle was busy, and MM.I2C reported a failure that was not the device's.
static void i2c_wait_idle(I2C_HandleTypeDef *h) {
	uint32_t t = HAL_GetTick();
	while(HAL_I2C_GetState(h) != HAL_I2C_STATE_READY && HAL_GetTick() - t < 5) {}
}
// A program's transfer as the bus master, by the registers. The HAL's blocking transfers always end with a stop and
// first wait for the bus to be free, so I2C WRITE and READ's option 1 (keep the bus: no stop, and the next transfer
// begins with a repeated start) could not be done with them. 'n' bytes are written from or read into 'buf' (over 255
// by reloads); 'timeout' is in ms, 0 for none (I2C OPEN). It returns MM.I2C: 0 done, 1 a NACK, 2 timed out (the HAL
// gave 1 for both). '*held' is true while the bus is kept.
static int i2c_fail(I2C_TypeDef *r, int *held, int result) {
	uint32_t t = HAL_GetTick();
	if(result == 1) {                                               // a NACK: a stop follows it, sent here when the
		while(!(r->ISR & I2C_ISR_STOPF) && HAL_GetTick() - t < 2) {}    // peripheral does not (a transfer keeping the bus)
		if(!(r->ISR & I2C_ISR_STOPF) && (r->ISR & I2C_ISR_BUSY)) {
			r->CR2 |= I2C_CR2_STOP;
			t = HAL_GetTick();
			while(!(r->ISR & I2C_ISR_STOPF) && HAL_GetTick() - t < 2) {}
		}
	}
	if(!(r->ISR & I2C_ISR_STOPF)) {                                 // no stop (timed out): resetting the peripheral
		r->CR1 &= ~I2C_CR1_PE;                                      // lets the lines go (PE low for 3 APB clocks)
		(void)r->CR1; (void)r->CR1; (void)r->CR1;
		r->CR1 |= I2C_CR1_PE;
	}
	r->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF;
	if(r->ISR & I2C_ISR_TXIS) r->TXDR = 0;                          // (the HAL's flush of the transmit register)
	if(!(r->ISR & I2C_ISR_TXE)) r->ISR |= I2C_ISR_TXE;
	r->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND);
	*held = 0;
	return result;
}

static int i2c_transfer(I2C_HandleTypeDef *h, int *held, uint16_t addr8, uint8_t *buf, int n, int rd, int hold, uint32_t timeout) {
	I2C_TypeDef *r = h->Instance;
	uint32_t t0 = HAL_GetTick();
	int i, chunk = n > 255 ? 255 : n;
#define I2C_LATE (timeout && HAL_GetTick() - t0 > timeout)
	if(!*held) {                                                    // (a kept bus is ours: busy, and the start repeated)
		while(r->ISR & I2C_ISR_BUSY) if(I2C_LATE) return i2c_fail(r, held, 2);
	}
	r->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
	MODIFY_REG(r->CR2, I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_ADD10
	           | I2C_CR2_HEAD10R | I2C_CR2_START | I2C_CR2_STOP,
	           (addr8 & I2C_CR2_SADD) | ((uint32_t)chunk << I2C_CR2_NBYTES_Pos) | (rd ? I2C_CR2_RD_WRN : 0)
	           | (n > 255 ? I2C_CR2_RELOAD : hold ? 0 : I2C_CR2_AUTOEND) | I2C_CR2_START);
	for(i = 0; i < n; i++) {
		if(i && i % 255 == 0) {                                     // the next 255 of a long transfer
			while(!(r->ISR & I2C_ISR_TCR)) {
				if(r->ISR & I2C_ISR_NACKF) return i2c_fail(r, held, 1);
				if(I2C_LATE) return i2c_fail(r, held, 2);
			}
			chunk = n - i > 255 ? 255 : n - i;
			MODIFY_REG(r->CR2, I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND,
			           ((uint32_t)chunk << I2C_CR2_NBYTES_Pos) | (n - i > 255 ? I2C_CR2_RELOAD : hold ? 0 : I2C_CR2_AUTOEND));
		}
		while(!(r->ISR & (rd ? I2C_ISR_RXNE : I2C_ISR_TXIS))) {
			if(r->ISR & I2C_ISR_NACKF) return i2c_fail(r, held, 1);
			if(I2C_LATE) return i2c_fail(r, held, 2);
		}
		if(rd) buf[i] = r->RXDR; else r->TXDR = buf[i];
	}
	while(!(r->ISR & (hold ? I2C_ISR_TC : I2C_ISR_STOPF))) {        // the end: the bus kept (TC), or the stop sent
		if(r->ISR & I2C_ISR_NACKF) return i2c_fail(r, held, 1);
		if(I2C_LATE) return i2c_fail(r, held, 2);
	}
#undef I2C_LATE
	r->ICR = I2C_ICR_STOPCF;
	if(!hold) r->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND);
	*held = hold;
	return 0;
}

// a bus kept by option 1 let go: the stop it was waiting for (before CHECK and CLOSE, and at the end of a program)
static void i2c_release(I2C_HandleTypeDef *h, int *held) {
	I2C_TypeDef *r = h->Instance;
	uint32_t t = HAL_GetTick();
	if(!*held) return;
	r->CR2 |= I2C_CR2_STOP;
	while(!(r->ISR & I2C_ISR_STOPF) && HAL_GetTick() - t < 2) {}
	r->ICR = I2C_ICR_STOPCF;
	r->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND);
	*held = 0;
}

/**************************************************************************************************
Send and/or Receive data - master mode
***************************************************************************************************/
void i2c_masterCommand(int timer, unsigned char *rbuf) {
	uint16_t addr8 = I2C_Addr << 1;
	int hold = (I2C_Status & I2C_Status_BusHold) != 0;
	mT4IntEnable(0);
	i2c_wait_idle(&hi2c1);                                         // (a controller's transfer under way)
	mmI2Cvalue = 0;
	if(I2C_Sendlen)                                              // (a read after it: a repeated start between)
		mmI2Cvalue = i2c_transfer(&hi2c1, &I2C_Held, addr8, I2C_Send_Buffer, I2C_Sendlen, 0, hold || I2C_Rcvlen, I2C_Timeout);
	if(I2C_Rcvlen && !mmI2Cvalue)
		mmI2Cvalue = i2c_transfer(&hi2c1, &I2C_Held, addr8, rbuf, I2C_Rcvlen, 1, hold, I2C_Timeout);
	mT4IntEnable(1);
}

void i2c2_masterCommand(int timer, unsigned char *rbuf) {
	uint16_t addr8 = I2C2_Addr << 1;
	int hold = (I2C2_Status & I2C_Status_BusHold) != 0;
	mT4IntEnable(0);
	i2c_wait_idle(&hi2c4);                                         // (a controller's transfer under way)
	mmI2Cvalue = 0;
	if(I2C2_Sendlen)                                              // (a read after it: a repeated start between)
		mmI2Cvalue = i2c_transfer(&hi2c4, &I2C2_Held, addr8, I2C_Send_Buffer, I2C2_Sendlen, 0, hold || I2C2_Rcvlen, I2C2_Timeout);
	if(I2C2_Rcvlen && !mmI2Cvalue)
		mmI2Cvalue = i2c_transfer(&hi2c4, &I2C2_Held, addr8, rbuf, I2C2_Rcvlen, 1, hold, I2C2_Timeout);
	mT4IntEnable(1);
}
void i2c3_masterCommand(int timer, unsigned char *rbuf) {
	uint16_t addr8 = I2C3_Addr << 1;
	int hold = (I2C3_Status & I2C_Status_BusHold) != 0;
	mT4IntEnable(0);
	i2c_wait_idle(&hi2c2);                                         // (a controller's transfer under way)
	mmI2Cvalue = 0;
	if(I2C3_Sendlen)                                              // (a read after it: a repeated start between)
		mmI2Cvalue = i2c_transfer(&hi2c2, &I2C3_Held, addr8, I2C_Send_Buffer, I2C3_Sendlen, 0, hold || I2C3_Rcvlen, I2C3_Timeout);
	if(I2C3_Rcvlen && !mmI2Cvalue)
		mmI2Cvalue = i2c_transfer(&hi2c2, &I2C3_Held, addr8, rbuf, I2C3_Rcvlen, 1, hold, I2C3_Timeout);
	mT4IntEnable(1);
}

//void fun_mmi2c(void) {
//	iret = mmI2Cvalue;
//    targ = T_INT;
//}

