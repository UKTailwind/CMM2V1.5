/***************************************************************************

CMM2 MMBasic
SPI.c
handles SPI commands.

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

unsigned int *GetSendDataList(char *p, unsigned int *nbr);
unsigned int *GetReceiveDataBuffer(char *p, unsigned int *nbr, CommsRxDest *dest);
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;

void cmd_spi1(char *spi) {
	    int speed;
    char *p;
    unsigned int nbr, *d;

    if(checkstring(spi, "CLOSE")) {
        SPIClose();
        return;
    }

    if((p = checkstring(spi, "WRITE")) != NULL) {
    	union car
    	{
    		uint32_t aTxBuffer;
    		uint8_t cTxBuffer[4];
    	}mybuff;
       if(ExtCurrentConfig[SPI_OUT_PIN] != EXT_COM_RESERVED) error("Not open");
        d = GetSendDataList(p, &nbr);
        while(nbr--) {
        	mybuff.aTxBuffer=*d++;
        	HAL_SPI_Transmit(&hspi1, mybuff.cTxBuffer,1, 5000);
        }
        return;
    }

    if((p = checkstring(spi, "READ")) != NULL) {
    	union car
    	{
    		uint32_t aRxBuffer;
    		uint8_t cRxBuffer[4];
    	}mybuff;
        CommsRxDest dest;
        unsigned int *rbuf, i;
        if(ExtCurrentConfig[SPI_OUT_PIN] != EXT_COM_RESERVED) error("Not open");
        rbuf = GetReceiveDataBuffer(p, &nbr, &dest);
        for(i = 0; i < nbr; i++) {
        	mybuff.aRxBuffer=0;
        	HAL_SPI_TransmitReceive(&hspi1, mybuff.cRxBuffer, mybuff.cRxBuffer, 1, 5000);   // 0 out (HAL 1.11: Receive no longer sends)
        	rbuf[i] = mybuff.aRxBuffer;
        }
        PutCommsRxData(&dest, rbuf);
        return;
    }

    p = checkstring(spi, "OPEN");
    if(p == NULL) error("Invalid syntax");
    if(ExtCurrentConfig[SPI_OUT_PIN] == EXT_COM_RESERVED) error("Already open");

    { // start a new block for getargs()
    	int mode,bits=8;
    	getargs(&p, 5, ",");
        if(argc < 3) error("Incorrect argument count");
        mode=getinteger(argv[2]);
        speed = getinteger(argv[0]);
        if(argc==5)bits=getint(argv[4],4,32);
       hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
       hspi1.Init.DataSize = bits-1;
        if(speed<390625)
		   hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_256;
        else if(speed<781250)
		   hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_128;
        else if(speed<1562500)
		   hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_64;
        else if(speed<3125000)
		   hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_32;
        else if(speed<6250000)
		   hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_16;
        else if(speed<12500000)
		   hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_8;
        else if(speed<25000000)
		   hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_4;
        else
           hspi1.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_2;
           /* you can have any number of case statements */
        hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
        if(mode &2)hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
        hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
        if(mode &1)hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;

        ExtCfg(SPI_OUT_PIN, EXT_COM_RESERVED, 0);
        ExtCfg(SPI_INP_PIN, EXT_COM_RESERVED, 0);
        ExtCfg(SPI_CLK_PIN, EXT_COM_RESERVED, 0);
        HAL_SPI_DeInit(&hspi1);
        HAL_SPI_Init(&hspi1);
    }
}





// output and get a byte via SPI
void fun_spi(void) {
	union car
	{
		uint64_t aTxBuffer;
		uint8_t cTxBuffer[8];
	}mybufft;
	union car1
	{
		uint64_t aRxBuffer;
		uint8_t cRxBuffer[8];
	}mybuffr;
	mybuffr.aRxBuffer=0;
	mybufft.aTxBuffer=getinteger(ep);
    if(ExtCurrentConfig[SPI_OUT_PIN] != EXT_COM_RESERVED) error("Not open");
	HAL_SPI_TransmitReceive(&hspi1, mybufft.cTxBuffer, mybuffr.cRxBuffer, 1, 5000);
	iret=mybuffr.aRxBuffer;
    targ = T_INT;

}



void SPIClose(void){
	HAL_SPI_DeInit(&hspi1);
	if(ExtCurrentConfig[SPI_OUT_PIN] != EXT_BOOT_RESERVED) ExtCfg(SPI_OUT_PIN, EXT_NOT_CONFIG, 0);
	if(ExtCurrentConfig[SPI_INP_PIN] != EXT_BOOT_RESERVED)  ExtCfg(SPI_INP_PIN, EXT_NOT_CONFIG, 0);      // reset to not in use
	if(ExtCurrentConfig[SPI_CLK_PIN] != EXT_BOOT_RESERVED)  ExtCfg(SPI_CLK_PIN, EXT_NOT_CONFIG, 0);
}

void cmd_spi2(char *spi) {
	    int speed;
    char *p;
    unsigned int nbr, *d;

    if(checkstring(spi, "CLOSE")) {
        SPI2Close();
        return;
    }

    if((p = checkstring(spi, "WRITE")) != NULL) {
    	union car
    	{
    		uint32_t aTxBuffer;
    		uint8_t cTxBuffer[4];
    	}mybuff;
       if(ExtCurrentConfig[SPI2_OUT_PIN] != EXT_COM_RESERVED) error("Not open");
        d = GetSendDataList(p, &nbr);
        while(nbr--) {
        	mybuff.aTxBuffer=*d++;
        	HAL_SPI_Transmit(&hspi2, mybuff.cTxBuffer,1, 5000);
        }
        return;
    }

    if((p = checkstring(spi, "READ")) != NULL) {
    	union car
    	{
    		uint32_t aRxBuffer;
    		uint8_t cRxBuffer[4];
    	}mybuff;
        CommsRxDest dest;
        unsigned int *rbuf, i;
        if(ExtCurrentConfig[SPI2_OUT_PIN] != EXT_COM_RESERVED) error("Not open");
        rbuf = GetReceiveDataBuffer(p, &nbr, &dest);
        for(i = 0; i < nbr; i++) {
        	mybuff.aRxBuffer=0;
        	HAL_SPI_TransmitReceive(&hspi2, mybuff.cRxBuffer, mybuff.cRxBuffer, 1, 5000);   // 0 out (HAL 1.11: Receive no longer sends)
        	rbuf[i] = mybuff.aRxBuffer;
        }
        PutCommsRxData(&dest, rbuf);
        return;
    }

    p = checkstring(spi, "OPEN");
    if(p == NULL) error("Invalid syntax");
    if(ExtCurrentConfig[SPI2_CLK_PIN] == EXT_COM_RESERVED) error("Already open");
    if(ExtCurrentConfig[SPI2_OUT_PIN] == EXT_COM_RESERVED) error("In use for COM2");

    { // start a new block for getargs()
    	int mode,bits=8;
    	getargs(&p, 5, ",");
        if(argc < 3) error("Incorrect argument count");
        mode=getinteger(argv[2]);
        speed = getinteger(argv[0]);
        if(argc==5)bits=getint(argv[4],4,32);
        hspi2.Init.DataSize = bits-1;
        hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
        if(speed<390625)
		   hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_256;
        else if(speed<781250)
		   hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_128;
        else if(speed<1562500)
		   hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_64;
        else if(speed<3125000)
		   hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_32;
        else if(speed<6250000)
		   hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_16;
        else if(speed<12500000)
		   hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_8;
        else if(speed<25000000)
		   hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_4;
        else
           hspi2.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_2;
           /* you can have any number of case statements */
        hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
        if(mode &2)hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
        hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
        if(mode &1)hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;

        ExtCfg(SPI2_OUT_PIN, EXT_COM_RESERVED, 0);
        ExtCfg(SPI2_INP_PIN, EXT_COM_RESERVED, 0);
        ExtCfg(SPI2_CLK_PIN, EXT_COM_RESERVED, 0);
        HAL_SPI_DeInit(&hspi2);
        HAL_SPI_Init(&hspi2);
    }
}





// output and get a byte via SPI
void fun_spi2(void) {
	union car
	{
		uint64_t aTxBuffer;
		uint8_t cTxBuffer[8];
	}mybufft;
	union car1
	{
		uint64_t aRxBuffer;
		uint8_t cRxBuffer[8];
	}mybuffr;
	mybuffr.aRxBuffer=0;
	mybufft.aTxBuffer=getinteger(ep);
    if(ExtCurrentConfig[SPI2_OUT_PIN] != EXT_COM_RESERVED) error("Not open");
	HAL_SPI_TransmitReceive(&hspi2, mybufft.cTxBuffer, mybuffr.cRxBuffer, 1, 5000);
	iret=mybuffr.aRxBuffer;
    targ = T_INT;

}



void SPI2Close(void){
	HAL_SPI_DeInit(&hspi2);
	if(ExtCurrentConfig[SPI2_OUT_PIN] != EXT_BOOT_RESERVED) ExtCfg(SPI2_OUT_PIN, EXT_NOT_CONFIG, 0);
	if(ExtCurrentConfig[SPI2_INP_PIN] != EXT_BOOT_RESERVED)  ExtCfg(SPI2_INP_PIN, EXT_NOT_CONFIG, 0);      // reset to not in use
	if(ExtCurrentConfig[SPI2_CLK_PIN] != EXT_BOOT_RESERVED)  ExtCfg(SPI2_CLK_PIN, EXT_NOT_CONFIG, 0);

}
unsigned int *GetSendDataList(char *p, unsigned int *nbr) {
    unsigned int *buf;

    getargs(&p, MAX_ARG_COUNT, ",");
    if(!(argc & 1)) error("Invalid syntax");
    *nbr = getint(argv[0], 0, 9999999);
    if(!*nbr) return NULL;
    buf = GetTempMemory(*nbr * sizeof(unsigned int));
    GetCommsTxData(argv, argc, 2, *nbr, buf);                      // a list, a whole array or a string (MMBasic.c)
    return buf;
}
void cmd_spi(void){ cmd_spi1(cmdline); }                        // SPI and SPI2 are commands of their own (they were SPI 1 and 2)
void cmd_spi_2(void){ cmd_spi2(cmdline); }


unsigned int *GetReceiveDataBuffer(char *p, unsigned int *nbr, CommsRxDest *dest) {
    getargs(&p, MAX_ARG_COUNT, ",");
    if(argc < 3 || !(argc & 1)) error("Invalid syntax");
    int n = getinteger(argv[0]);
    if(n < 1) error("Number out of bounds");                      // (a negative count became a huge unsigned one)
    *nbr = n;
    GetCommsRxDest(argv, argc, 2, n, dest);                        // a list, a whole array or a string (MMBasic.c)
    return GetTempMemory(n * sizeof(unsigned int));
}
