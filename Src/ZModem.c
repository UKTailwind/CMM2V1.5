/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ZMODEM for the Colour Maximite 2, ported from the PicoMite's misc/ZModem.c (V7.00.00RC1), which is
 * derived from TeraTerm's.  The protocol engine is unchanged; only the platform surface differs:
 *
 *   console bytes      SerialConsolePutC() and the ConsoleRxBuf ring / getConsole()
 *   microsecond clock  Getuptime()
 *   CRC-32             a table here (the PicoMite used littlefs's lfs_crc, which the CMM2 does not have)
 *   files              FatFs through FileTable[fnbr].fptr (the CMM2 has no flash file system)
 *   programs           the CMM2 runs programs from the SD card, so there is no program memory to send
 *                      or receive: ZMODEM SEND with no name sends the current program's file, ZMODEM
 *                      RECEIVE with no name is a batch into the current directory, and ZMODEM CRUNCH
 *                      does not exist (crunching is a load-time option here)
 *
 * The command table is full, so ZMODEM is not a token of its own: tokenise() rewrites "ZMODEM" as
 * "XMODEM Z" and cmd_xmodem() hands a leading Z to cmd_zmodem().  The syntax is the PicoMite's:
 *
 *   ZMODEM SEND [file$]        ZMODEM RECEIVE [file$]        ZMODEM RECEIVE "dir/" + "*"
 *
 * SEND and RECEIVE can be shortened to S and R.
 */

#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"

#define MAXERRMSG 64

extern void SerialConsolePutC(int c);
extern volatile char ConsoleRxBuf[CONSOLE_RX_BUF_SIZE];
extern volatile int ConsoleRxBufHead;
extern volatile int ConsoleRxBufTail;
extern char BreakKey;
// OptionFileErrorAbort, FSerror (an FRESULT), ErrorCheck, InitSDCard, FindFreeFileNbr, BasicFileOpen,
// FileClose and FileTable come from fileIO.h and SerialFileIO.h through Hardware_Includes.h

#define ZPAD '*'
#define ZDLE 0x18
#define ZBIN 'A'
#define ZHEX 'B'
#define ZBIN32 'C'
#define XON 0x11

#define ZRQINIT 0
#define ZRINIT 1
#define ZSINIT 2
#define ZACK 3
#define ZFILE 4
#define ZSKIP 5
#define ZNAK 6
#define ZABORT 7
#define ZFIN 8
#define ZRPOS 9
#define ZDATA 10
#define ZEOF 11
#define ZFERR 12

#define ZCBIN 1 // ZFILE: a binary file

#define ZCRCE 'h'
#define ZCRCG 'i'
#define ZCRCQ 'j'
#define ZCRCW 'k'
#define ZRUB0 'l'
#define ZRUB1 'm'

#define ZF0 3 // header byte order: flags count down, position counts up
#define ZP0 0
#define ZP1 1
#define ZP2 2
#define ZP3 3

#define CANFDX 0x01
#define CANFC32 0x20
#define ESCCTL 0x40

// File data is kept in RAM until the sender pauses for a ZACK (ZCRCQ/ZCRCW). Tera Term pauses after its
// window (32 KB by default). lrzsz streams on regardless of the buffer length in ZRINIT, so when the RAM
// is full the receiver asks for the data again from its position (ZRPOS): lrzsz restarts with a ZCRCW
// subpacket and waits for the ZACK, and the RAM is written while it waits. A sender that streams on
// after that is written to mid-stream; the bytes lost meanwhile fail a CRC and ZRPOS recovers them.
// The CMM2's console is always a UART with a 256-byte receive ring, so the sender is never written to
// while it may be streaming.
#define ZSTAGE (36 * 1024)
#define ZRXBUFLEN 32768
#define ZMAXDATA 1024 // largest data subpacket the spec allows

#define ZT_INIT 3000000 // us between ZRINITs while waiting for a sender
#define ZT_INIT_TRIES 20
#define ZT_DATA 3000000 // us of silence in the middle of data before asking for it again
#define ZT_REPLY 1000000 // us to wait for the answer to a ZACK, ZRPOS or a later ZRINIT: a lost reply costs this
#define ZT_DATA_TRIES 30
#define ZT_FIN 1000000 // us to wait for the sender's "OO"

// Sending: the receiver is asked for a ZACK (ZCRCW) after this much, or after its buffer length if that is
// smaller, so a receiver that has fallen behind or lost data is never more than this far behind.
#define ZTXWINDOW (16 * 1024)

enum
{
    Z_RecvInit = 1,
    Z_RecvInit2,
    Z_RecvData,
    Z_RecvFIN,
    Z_Send,
    Z_End
};

enum
{
    Z_PktGetPAD = 1,
    Z_PktGetDLE,
    Z_PktHdrFrm,
    Z_PktGetBin,
    Z_PktGetHex,
    Z_PktGetHexEOL,
    Z_PktGetData,
    Z_PktGetCRC
};

typedef struct
{
    uint8_t RxHdr[4], TxHdr[4];
    uint8_t RxType, TERM;
    uint8_t PktIn[ZMAXDATA + 8];
    int PktInPtr, PktInCount;
    bool use32, HexLo, Quoted;   // (CRC is a peripheral macro in the STM32 header, so these are not called CRC)
    uint16_t rxcrc;
    uint32_t rxcrc32;
    int ZState, ZPktState, CanCount;
    uint32_t Pos;      // bytes of the current file accepted so far
    uint8_t *stage;    // accepted bytes not yet written to the file
    int stagelen;
    int fnbr;
    bool fileopen;
    bool held;            // the RAM was full and a ZRPOS asked the sender to restart and pause
    const char *target;   // receiving: the file to write
    bool batch;           // receiving: "*" or "dir/*", every file under the sender's name
    int dirlen;           // receiving a batch: the length of the directory in name, before the sender's name
    char name[FF_MAX_LFN];
    bool gothdr;      // sending: a header from the receiver is in RxType/RxHdr
    bool txcrc32;     // sending: the receiver takes CRC-32
    bool escctl;      // sending: the receiver wants every control character escaped
    uint8_t lastsent; // sending: the last byte out, for the escape of "@ CR"
    uint32_t window;  // sending: bytes between ZACKs
    int files;        // files completed
    int tries;        // timeouts in a row
    uint32_t timeout; // us of silence before ZTimeOut
    uint32_t bytes;   // total bytes received or sent
    char err[MAXERRMSG];
} zrx_t;

// ---- the platform surface ----

static inline uint64_t ztime(void)
{
    return (uint64_t)Getuptime();
}

// one byte to the console; the second argument (flush) is what the PicoMite's USB console needed
static inline void zputc(int c, int flush)
{
    (void)flush;
    SerialConsolePutC(c);
}

static uint16_t zcrc16(uint8_t b, uint16_t crc)
{
    crc ^= (uint16_t)b << 8;
    for (int i = 0; i < 8; i++)
        crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    return crc;
}

// CRC-32 as littlefs's lfs_crc computes it (the reflected 0xEDB88320 polynomial, no pre- or
// post-conditioning: the caller starts at 0xFFFFFFFF and ZMODEM's residue is 0xDEBB20E3)
static uint32_t zcrctab[256];
static void zcrcinit(void)
{
    if (zcrctab[1])
        return;
    for (uint32_t n = 0; n < 256; n++)
    {
        uint32_t c = n;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        zcrctab[n] = c;
    }
}
static inline uint32_t zcrc32(uint8_t b, uint32_t crc)
{
    return zcrctab[(crc ^ b) & 0xFF] ^ (crc >> 8);
}

// next byte from the console, or -1 after timeout_us of silence
static int zgetc(uint32_t timeout_us)
{
    uint64_t end = ztime() + timeout_us;
    while (1)
    {
        if (ConsoleRxBufHead != ConsoleRxBufTail)
        {
            int c = (uint8_t)ConsoleRxBuf[ConsoleRxBufTail];
            ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE;
            return c;
        }
        int c = getConsole(); // the line is idle: let the background work (keyboard, SD card) run
        if (c != -1)
            return c & 0xFF;
        if (ztime() >= end)
            return -1;
    }
}

// a file is open on fnbr (ErrorCheck closes a file whose operation failed)
static inline bool zfileisopen(zrx_t *z)
{
    return FileTable[z->fnbr].com != 0;
}

// write n bytes to the file; false (and the session failed) on a file error
static bool zfilewrite(zrx_t *z, const uint8_t *buf, unsigned int n);
// read up to n bytes from the file into buf; the count read, or 0 on a file error
static unsigned int zfileread(zrx_t *z, uint8_t *buf, unsigned int n);

// ---- receiving ----

static void zputhex(uint8_t b)
{
    static const char hex[] = "0123456789abcdef";
    zputc(hex[b >> 4], 0);
    zputc(hex[b & 15], 0);
}

static void ZStoHdr(zrx_t *z, uint32_t pos)
{
    z->TxHdr[ZP0] = pos;
    z->TxHdr[ZP1] = pos >> 8;
    z->TxHdr[ZP2] = pos >> 16;
    z->TxHdr[ZP3] = pos >> 24;
}

static uint32_t ZRclHdr(zrx_t *z)
{
    return z->RxHdr[ZP0] | (z->RxHdr[ZP1] << 8) | (z->RxHdr[ZP2] << 16) | ((uint32_t)z->RxHdr[ZP3] << 24);
}

// send a hex header (what a receiver always uses)
static void ZShHdr(zrx_t *z, uint8_t type)
{
    uint16_t crc = zcrc16(type, 0);
    zputc(ZPAD, 0);
    zputc(ZPAD, 0);
    zputc(ZDLE, 0);
    zputc(ZHEX, 0);
    zputhex(type);
    for (int i = 0; i < 4; i++)
    {
        zputhex(z->TxHdr[i]);
        crc = zcrc16(z->TxHdr[i], crc);
    }
    zputhex(crc >> 8);
    zputhex(crc);
    zputc(0x0D, 0);
    if (type == ZFIN || type == ZACK)
        zputc(0x8A, 1);
    else
    {
        zputc(0x8A, 0);
        zputc(XON, 1);
    }
}

static void ZSendRInit(zrx_t *z)
{
    z->Pos = 0;
    ZStoHdr(z, 0);
    z->TxHdr[ZP0] = ZRXBUFLEN & 0xFF; // receive buffer length: lrzsz pauses for a ZACK after this much
    z->TxHdr[ZP1] = ZRXBUFLEN >> 8;
    z->TxHdr[ZF0] = CANFDX | CANFC32;
    ZShHdr(z, ZRINIT);
    z->timeout = z->files ? ZT_REPLY : ZT_INIT; // the first waits for a person to start the sender
}

static void ZSendRPOS(zrx_t *z)
{
    ZStoHdr(z, z->Pos);
    ZShHdr(z, ZRPOS);
    z->timeout = ZT_REPLY;
}

static void ZSendACK(zrx_t *z)
{
    ZStoHdr(z, z->Pos);
    ZShHdr(z, ZACK);
    z->timeout = ZT_REPLY;
}

static void ZSendCancel(void)
{
    for (int i = 0; i < 8; i++)
        zputc(ZDLE, 0);
    for (int i = 0; i < 10; i++)
        zputc(0x08, i == 9);
}

// end the session with an error: the sender is told, the reason is reported when the command returns
static void zfail(zrx_t *z, const char *msg)
{
    if (!*z->err)
        strncpy(z->err, msg, MAXERRMSG - 1);
    ZSendCancel();
    z->ZState = Z_End;
}

// the message of the last file error, or a default
static const char *zfilemsg(const char *dflt)
{
    return (MMerrno && MMErrMsg && *MMErrMsg) ? MMErrMsg : dflt;
}

static bool zfilewrite(zrx_t *z, const uint8_t *buf, unsigned int n)
{
    UINT written = 0;
    if (!zfileisopen(z))
        return false;
    MMerrno = 0;
    FSerror = f_write(FileTable[z->fnbr].fptr, buf, n, &written);
    if (FSerror == 0 && written != n)
        FSerror = FR_DENIED; // the card is full
    if (FSerror)
    {
        ErrorCheck(z->fnbr); // sets the message and closes the file; no abort while OptionFileErrorAbort is 0
        z->fileopen = false;
        zfail(z, zfilemsg("File write failed"));
        return false;
    }
    return true;
}

static unsigned int zfileread(zrx_t *z, uint8_t *buf, unsigned int n)
{
    UINT got = 0;
    if (!zfileisopen(z))
        return 0;
    MMerrno = 0;
    FSerror = f_read(FileTable[z->fnbr].fptr, buf, n, &got);
    if (FSerror)
    {
        ErrorCheck(z->fnbr);
        z->fileopen = false;
        return 0;
    }
    return got;
}

// write the bytes held in RAM to the file (the sender is paused, or has run past the buffer)
static void zflush(zrx_t *z)
{
    if (z->stagelen == 0 || !z->fileopen)
        return;
    int n = z->stagelen;
    z->stagelen = 0;
    zfilewrite(z, z->stage, n);
}

// close the file, unless a file error has closed it already (ErrorCheck does)
static void zfileclose(zrx_t *z)
{
    z->fileopen = false;
    FSerror = 0;
    if (zfileisopen(z))
        FileClose(z->fnbr);
}

static void zclose(zrx_t *z)
{
    if (!z->fileopen)
        return;
    zflush(z);
    if (z->ZState == Z_End)
        return;
    MMerrno = 0;
    zfileclose(z);
    if (FSerror)
        zfail(z, zfilemsg("File close failed"));
}

// ZFILE data: "name\0size mtime ..."; open the file the data will go to
static bool ZParseFile(zrx_t *z)
{
    if (z->ZState != Z_RecvInit && z->ZState != Z_RecvInit2)
        return false;
    z->PktIn[z->PktInPtr] = 0; // for safety
    if (z->files && !z->batch)
        return false; // one file was asked for: skip the rest of a batch
    if (z->batch)
    {
        // the sender's name, without any directory it sent, after the directory asked for
        char *n = (char *)z->PktIn, *p;
        for (p = n; *p; p++)
            if (*p == '/' || *p == '\\')
                n = p + 1;
        if (!*n || z->dirlen + strlen(n) >= FF_MAX_LFN)
        {
            zfail(z, *n ? "File name too long" : "Invalid file name");
            return false;
        }
        strcpy(z->name + z->dirlen, n);
    }
    else
        strncpy(z->name, z->target, FF_MAX_LFN - 1);
    MMerrno = 0;
    if (!BasicFileOpen(z->name, z->fnbr, FA_WRITE | FA_CREATE_ALWAYS))
    {
        zfail(z, zfilemsg("Cannot create the file"));
        return false;
    }
    z->fileopen = true;
    z->held = false;
    z->stagelen = 0;
    z->Pos = 0;
    ZStoHdr(z, 0);
    z->ZState = Z_RecvData;
    z->timeout = ZT_DATA;
    return true;
}

// a good data subpacket: hold its bytes until the sender pauses
static bool ZWriteData(zrx_t *z)
{
    if (z->ZState != Z_RecvData)
        return false;
    if (z->stagelen + z->PktInPtr > ZSTAGE)
    {
        if (z->TERM == ZCRCG && !z->held)
        {
            // full, and the sender is still streaming: drop this subpacket and ask for it again; lrzsz
            // restarts with a ZCRCW and waits, so the write below happens while it waits
            z->held = true;
            ZSendRPOS(z);
            return false;
        }
        zflush(z); // the sender waits (ZCRCW/ZCRCQ), or streamed on after the ZRPOS: write now
        if (z->ZState == Z_End)
            return false;
    }
    z->held = false;
    memcpy(z->stage + z->stagelen, z->PktIn, z->PktInPtr);
    z->stagelen += z->PktInPtr;
    z->Pos += z->PktInPtr;
    z->bytes += z->PktInPtr;
    ZStoHdr(z, z->Pos);
    z->timeout = ZT_DATA;
    return true;
}

static void ZResetPkt(zrx_t *z)
{
    z->Quoted = false;
    z->rxcrc = 0;
    z->rxcrc32 = 0xFFFFFFFF;
    z->PktInPtr = 0;
    z->PktInCount = 0;
}

// a data subpacket and its CRC have arrived
static void ZCheckData(zrx_t *z)
{
    bool ok;
    if ((z->use32 && z->rxcrc32 != 0xDEBB20E3) || (!z->use32 && z->rxcrc != 0))
    {
        if (z->ZState == Z_RecvData)
            ZSendRPOS(z);
        else if (z->ZState == Z_RecvInit || z->ZState == Z_RecvInit2)
        {
            ZStoHdr(z, 0);
            ZShHdr(z, ZNAK);
        }
        z->ZPktState = Z_PktGetPAD;
        return;
    }
    switch (z->RxType)
    {
    case ZSINIT:
        ok = (z->ZState == Z_RecvInit);
        if (ok)
            z->ZState = Z_RecvInit2;
        break;
    case ZFILE:
        ok = ZParseFile(z);
        if (!ok && z->ZState != Z_End)
        {
            ZStoHdr(z, 0);
            ZShHdr(z, ZSKIP);
        }
        break;
    case ZDATA:
        ok = ZWriteData(z);
        break;
    default:
        ok = false;
    }
    if (!ok || z->ZState == Z_End)
    {
        z->ZPktState = Z_PktGetPAD;
        return;
    }
    if (z->RxType == ZFILE)
        ZShHdr(z, ZRPOS); // start from 0 (ZParseFile set the position)

    switch (z->TERM)
    {
    case ZCRCG: // more data follows, no reply
        z->ZPktState = Z_PktGetData;
        break;
    case ZCRCQ: // more data follows, the sender waits for a ZACK
        z->ZPktState = Z_PktGetData;
        if (z->RxType != ZFILE)
        {
            zflush(z);
            if (z->ZState != Z_End)
                ZSendACK(z);
        }
        break;
    case ZCRCW: // end of frame, the sender waits for a ZACK
        z->ZPktState = Z_PktGetPAD;
        if (z->RxType != ZFILE)
        {
            zflush(z);
            if (z->ZState != Z_End)
                ZSendACK(z);
        }
        break;
    default: // ZCRCE: end of frame, a header follows
        z->ZPktState = Z_PktGetPAD;
    }
    if (z->ZPktState == Z_PktGetData)
        ZResetPkt(z);
}

// a header has arrived: check its CRC and pick it apart
static bool ZCheckHdr(zrx_t *z)
{
    bool ok;
    if (z->use32)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (int i = 0; i <= 8; i++)
            crc = zcrc32(z->PktIn[i], crc);
        ok = crc == 0xDEBB20E3;
    }
    else
    {
        uint16_t crc = 0;
        for (int i = 0; i <= 6; i++)
            crc = zcrc16(z->PktIn[i], crc);
        ok = crc == 0;
    }
    if (!ok)
    {
        if (z->ZState == Z_RecvInit)
            ZSendRInit(z);
        else if (z->ZState == Z_RecvData)
            ZSendRPOS(z);
    }
    z->RxType = z->PktIn[0];
    for (int i = 1; i <= 4; i++)
        z->RxHdr[i - 1] = z->PktIn[i];
    return ok;
}

static void ZParseHdr(zrx_t *z)
{
    if (z->ZState == Z_Send)
    {
        z->gothdr = true; // the sender acts on it
        return;
    }
    switch (z->RxType)
    {
    case ZRQINIT:
        if (z->ZState == Z_RecvInit)
            ZSendRInit(z);
        break;
    case ZSINIT:
        z->ZPktState = Z_PktGetData;
        break;
    case ZFILE:
        z->ZPktState = Z_PktGetData;
        break;
    case ZDATA:
        if (z->ZState != Z_RecvData)
            break;
        if (z->Pos != ZRclHdr(z))
        {
            ZSendRPOS(z); // not where we are: ask for the data from our position
            return;
        }
        z->ZPktState = Z_PktGetData;
        z->timeout = ZT_DATA;
        break;
    case ZEOF:
        if (z->ZState != Z_RecvData)
            break;
        if (z->Pos != ZRclHdr(z))
            break; // sent before our ZRPOS reached the sender, which is coming back for the data; answering
                   // with another ZRPOS makes it send the data twice (as lrzsz rz, a lost ZRPOS times out)
        zclose(z);
        if (z->ZState == Z_End)
            return;
        z->files++;
        z->ZState = Z_RecvInit;
        ZSendRInit(z); // ready for the next file, or ZFIN
        break;
    case ZFIN:
        zclose(z);
        if (z->ZState == Z_End)
            return;
        z->ZState = Z_RecvFIN;
        ZStoHdr(z, 0);
        ZShHdr(z, ZFIN);
        z->CanCount = 2; // now wait for "OO"
        z->timeout = ZT_FIN;
        break;
    case ZABORT:
    case ZFERR:
        zfail(z, "Cancelled by remote");
        break;
    default:
        break;
    }
    ZResetPkt(z);
}

// one byte from the line through the frame parser
static void ZParse(zrx_t *z, uint8_t b)
{
    if ((b & 0x7F) == 0x11 || (b & 0x7F) == 0x13)
        return; // flow control characters are escaped in data: a bare one is noise
    if (z->ZState == Z_RecvFIN)
    {
        if (b == 'O' && --z->CanCount <= 0)
            z->ZState = Z_End;
        return;
    }
    if (b == ZDLE)
    {
        if (--z->CanCount <= 0)
        {
            if (!*z->err)
                strcpy(z->err, "Cancelled by remote");
            z->ZState = Z_End;
            return;
        }
    }
    else
        z->CanCount = 5;

    switch (z->ZPktState)
    {
    case Z_PktGetPAD:
        if (b == ZPAD)
            z->ZPktState = Z_PktGetDLE;
        break;
    case Z_PktGetDLE:
        if (b == ZDLE)
            z->ZPktState = Z_PktHdrFrm;
        else if (b != ZPAD)
            z->ZPktState = Z_PktGetPAD;
        break;
    case Z_PktHdrFrm:
        switch (b)
        {
        case ZBIN:
            z->use32 = false;
            z->PktInCount = 7;
            z->ZPktState = Z_PktGetBin;
            break;
        case ZHEX:
            z->HexLo = false;
            z->use32 = false;
            z->PktInCount = 7;
            z->ZPktState = Z_PktGetHex;
            break;
        case ZBIN32:
            z->use32 = true;
            z->PktInCount = 9;
            z->ZPktState = Z_PktGetBin;
            break;
        default:
            z->ZPktState = Z_PktGetPAD;
        }
        z->Quoted = false;
        z->PktInPtr = 0;
        break;
    case Z_PktGetBin:
        if (b == ZDLE)
        {
            z->Quoted = true;
            break;
        }
        if (z->Quoted)
        {
            b = (b == ZRUB0) ? 0x7F : (b == ZRUB1) ? 0xFF : b ^ 0x40;
            z->Quoted = false;
        }
        z->PktIn[z->PktInPtr++] = b;
        if (--z->PktInCount == 0)
        {
            z->ZPktState = Z_PktGetPAD;
            if (ZCheckHdr(z))
                ZParseHdr(z);
        }
        break;
    case Z_PktGetHex:
        if (b >= '0' && b <= '9')
            b -= '0';
        else if (b >= 'a' && b <= 'f')
            b -= 'a' - 10;
        else
        {
            z->ZPktState = Z_PktGetPAD;
            break;
        }
        if (z->HexLo)
        {
            z->PktIn[z->PktInPtr++] += b;
            z->HexLo = false;
            if (--z->PktInCount <= 0)
            {
                z->ZPktState = Z_PktGetHexEOL;
                z->PktInCount = 2;
            }
        }
        else
        {
            z->PktIn[z->PktInPtr] = b << 4;
            z->HexLo = true;
        }
        break;
    case Z_PktGetHexEOL:
        if (--z->PktInCount <= 0)
        {
            z->ZPktState = Z_PktGetPAD;
            if (ZCheckHdr(z))
                ZParseHdr(z);
        }
        break;
    case Z_PktGetData:
        if (b == ZDLE)
        {
            z->Quoted = true;
            break;
        }
        if (z->Quoted)
        {
            z->Quoted = false;
            switch (b)
            {
            case ZCRCE:
            case ZCRCG:
            case ZCRCQ:
            case ZCRCW:
                z->TERM = b;
                z->PktInCount = z->use32 ? 4 : 2;
                z->ZPktState = Z_PktGetCRC;
                break;
            case ZRUB0:
                b = 0x7F;
                break;
            case ZRUB1:
                b = 0xFF;
                break;
            default:
                b ^= 0x40;
            }
        }
        if (z->use32)
            z->rxcrc32 = zcrc32(b, z->rxcrc32);
        else
            z->rxcrc = zcrc16(b, z->rxcrc);
        if (z->ZPktState == Z_PktGetData)
        {
            if (z->PktInPtr < ZMAXDATA)
                z->PktIn[z->PktInPtr++] = b;
            else
            {
                // longer than any subpacket: we have lost the frame end, ask again now rather than time out
                z->ZPktState = Z_PktGetPAD;
                if (z->ZState == Z_RecvData)
                    ZSendRPOS(z);
            }
        }
        break;
    case Z_PktGetCRC:
        if (b == ZDLE)
        {
            z->Quoted = true;
            break;
        }
        if (z->Quoted)
        {
            b = (b == ZRUB0) ? 0x7F : (b == ZRUB1) ? 0xFF : b ^ 0x40;
            z->Quoted = false;
        }
        if (z->use32)
            z->rxcrc32 = zcrc32(b, z->rxcrc32);
        else
            z->rxcrc = zcrc16(b, z->rxcrc);
        if (--z->PktInCount <= 0)
            ZCheckData(z);
        break;
    }
}

// nothing arrived for z->timeout: prompt the sender again, or give up
static void ZTimeOut(zrx_t *z)
{
    switch (z->ZState)
    {
    case Z_RecvInit:
        if (++z->tries > ZT_INIT_TRIES)
            zfail(z, "Sender did not respond");
        else
            ZSendRInit(z);
        break;
    case Z_RecvInit2:
        if (++z->tries > ZT_DATA_TRIES)
            zfail(z, "Sender stopped responding");
        else
            ZSendACK(z);
        break;
    case Z_RecvData:
        if (++z->tries > ZT_DATA_TRIES)
            zfail(z, "Sender stopped responding");
        else
            ZSendRPOS(z);
        break;
    case Z_RecvFIN:
        z->ZState = Z_End; // the "OO" is a courtesy
        break;
    }
    z->ZPktState = Z_PktGetPAD;
}

static void zreceive(zrx_t *z)
{
    z->ZState = Z_RecvInit;
    z->ZPktState = Z_PktGetPAD;
    z->CanCount = 5;
    ZResetPkt(z);
    ZSendRInit(z);
    while (z->ZState != Z_End)
    {
        int c = zgetc(z->timeout);
        if (c < 0)
        {
            ZTimeOut(z);
            continue;
        }
        z->tries = 0;
        ZParse(z, c);
    }
}

// ---- sending ----

// one byte of a binary header or a data subpacket, escaped as lrzsz does (and every control character if the
// receiver asked for that)
static void zputbin(zrx_t *z, uint8_t b, int flush)
{
    bool esc;
    switch (b)
    {
    case ZDLE:
    case 0x10:
    case 0x11:
    case 0x13:
    case 0x90:
    case 0x91:
    case 0x93:
        esc = true;
        break;
    case 0x0D:
    case 0x8D:
        esc = z->escctl || (z->lastsent & 0x7F) == '@'; // "@ CR" means something to a modem or a telnet server
        break;
    default:
        esc = z->escctl && (b & 0x60) == 0;
    }
    if (esc)
    {
        zputc(ZDLE, 0);
        b ^= 0x40;
    }
    z->lastsent = b;
    zputc(b, flush);
}

static uint32_t ztxcrc(zrx_t *z, uint8_t b, uint32_t crc)
{
    return z->txcrc32 ? zcrc32(b, crc) : zcrc16(b, crc);
}

// the check that ends a binary header or a subpacket: CRC-32 low byte first, CRC-16 high byte first
static void zputcrc(zrx_t *z, uint32_t crc)
{
    if (z->txcrc32)
    {
        crc = ~crc;
        for (int i = 0; i < 4; i++, crc >>= 8)
            zputbin(z, crc, i == 3);
    }
    else
    {
        zputbin(z, crc >> 8, 0);
        zputbin(z, crc, 1);
    }
}

// send a binary header (what a sender uses)
static void ZSbHdr(zrx_t *z, uint8_t type)
{
    uint32_t crc = ztxcrc(z, type, z->txcrc32 ? 0xFFFFFFFF : 0);
    zputc(ZPAD, 0);
    zputc(ZDLE, 0);
    zputc(z->txcrc32 ? ZBIN32 : ZBIN, 0);
    zputbin(z, type, 0);
    for (int i = 0; i < 4; i++)
    {
        zputbin(z, z->TxHdr[i], 0);
        crc = ztxcrc(z, z->TxHdr[i], crc);
    }
    zputcrc(z, crc);
}

// send a data subpacket ending in term
static void ZSdata(zrx_t *z, const uint8_t *buf, int n, uint8_t term)
{
    uint32_t crc = z->txcrc32 ? 0xFFFFFFFF : 0;
    for (int i = 0; i < n; i++)
    {
        zputbin(z, buf[i], 0);
        crc = ztxcrc(z, buf[i], crc);
    }
    zputc(ZDLE, 0);
    zputc(term, 0);
    zputcrc(z, ztxcrc(z, term, crc));
}

// wait up to timeout_us for a header from the receiver: its type, or -1 for none (or the session has ended)
static int zgethdr(zrx_t *z, uint32_t timeout_us)
{
    uint64_t end = ztime() + timeout_us;
    z->gothdr = false;
    while (z->ZState == Z_Send)
    {
        uint64_t now = ztime();
        int c = zgetc(now < end ? (uint32_t)(end - now) : 0);
        if (c < 0)
            return -1;
        ZParse(z, c);
        if (z->gothdr)
            return z->RxType;
    }
    return -1;
}

// a header that ends the session from the receiver's side
static bool zrefused(zrx_t *z, int t)
{
    if (t != ZFIN && t != ZABORT && t != ZFERR)
        return false;
    zfail(z, "Receiver cancelled");
    return true;
}

// ZRQINIT until the receiver answers ZRINIT, then take its capabilities
static bool zsendinit(zrx_t *z)
{
    for (z->tries = 0;;)
    {
        ZStoHdr(z, 0);
        ZShHdr(z, ZRQINIT);
        int t = zgethdr(z, ZT_INIT); // the first waits for a person to start the receiver
        if (z->ZState != Z_Send)
            return false;
        if (t == ZRINIT)
            break;
        if (t < 0 && ++z->tries > ZT_INIT_TRIES)
        {
            zfail(z, "Receiver did not respond");
            return false;
        }
    }
    uint32_t rxbuf = z->RxHdr[ZP0] | (z->RxHdr[ZP1] << 8); // 0: it can take any amount
    z->txcrc32 = (z->RxHdr[ZF0] & CANFC32) != 0;
    z->escctl = (z->RxHdr[ZF0] & ESCCTL) != 0;
    z->window = ZTXWINDOW;
    if (!(z->RxHdr[ZF0] & CANFDX)) // it cannot listen while it receives: stop after every subpacket
        z->window = ZMAXDATA;
    else if (rxbuf && rxbuf < z->window)
        z->window = rxbuf;
    return true;
}

// send the open file z->fnbr under the name z->name: false if the session has ended
static bool zsendfile(zrx_t *z)
{
    uint32_t size = (uint32_t)f_size(FileTable[z->fnbr].fptr);
    uint8_t info[FF_MAX_LFN + 32];
    int t;
    // ZFILE: the receiver answers with where to start (ZRPOS), or that it does not want the file (ZSKIP)
    for (z->tries = 0;;)
    {
        ZStoHdr(z, 0);
        z->TxHdr[ZF0] = ZCBIN;
        ZSbHdr(z, ZFILE);
        int n = strlen(z->name) + 1;
        memcpy(info, z->name, n);
        // size, no time (0: the receiver uses its own) and an ordinary file (else lrzsz makes one only its owner reads)
        n += sprintf((char *)info + n, "%lu 0 100644", (unsigned long)size) + 1;
        ZSdata(z, info, n, ZCRCW);
        t = zgethdr(z, ZT_DATA);
        if (z->ZState != Z_Send || zrefused(z, t))
            return false;
        if (t == ZRPOS)
            break;
        if (t == ZSKIP)
            return true;
        if (t < 0 && ++z->tries > ZT_DATA_TRIES)
        {
            zfail(z, "Receiver stopped responding");
            return false;
        }
    }
    uint32_t pos = ZRclHdr(z), start = 0, rpos = ~0u;
    int rposcount = 0;
    bool frame = false; // a ZDATA header has started a frame and data follows it
    for (z->tries = 0;;)
    {
        if (!frame)
        {
            if (pos > size)
                pos = size;
            start = pos;
            MMerrno = 0;
            FSerror = f_lseek(FileTable[z->fnbr].fptr, pos);
            if (FSerror)
            {
                ErrorCheck(z->fnbr);
                z->fileopen = false;
                zfail(z, zfilemsg("File read failed"));
                return false;
            }
            ZStoHdr(z, pos);
            ZSbHdr(z, ZDATA);
            frame = true;
        }
        const uint8_t *data = z->stage;
        unsigned int n = 0;
        if (pos < size)
        {
            n = size - pos < ZMAXDATA ? size - pos : ZMAXDATA;
            n = zfileread(z, z->stage, n);
            if (n == 0)
            {
                zfail(z, zfilemsg("File read failed"));
                return false;
            }
        }
        pos += n;
        uint8_t term = pos >= size ? ZCRCE : pos - start >= z->window ? ZCRCW : ZCRCG;
        ZSdata(z, data, n, term);
        if (term == ZCRCE)
        {
            ZStoHdr(z, pos);
            ZSbHdr(z, ZEOF);
        }
        // while streaming, only look for a header (a receiver that loses data says so at once); after a
        // ZCRCW or the ZEOF, wait for the answer
        t = zgethdr(z, term == ZCRCG ? 0 : ZT_DATA);
        while (1)
        {
            if (z->ZState != Z_Send || zrefused(z, t))
                return false;
            if (t == ZRPOS) // the receiver lost data: go back to where it is
            {
                if (ZRclHdr(z) != rpos)
                    rposcount = 0;
                else if (++rposcount > ZT_DATA_TRIES)
                {
                    zfail(z, "Too many errors");
                    return false;
                }
                rpos = pos = ZRclHdr(z);
                frame = false;
                break;
            }
            if (t == ZSKIP)
                return true;
            if (term == ZCRCG)
                break;
            if (t == ZACK && term == ZCRCW) // carry on from where the receiver is
            {
                pos = ZRclHdr(z);
                frame = false;
                z->tries = 0;
                break;
            }
            if (t == ZRINIT && term == ZCRCE) // the file is complete
            {
                z->files++;
                z->bytes += size;
                return true;
            }
            if (t < 0) // no answer: send again from the start of this frame
            {
                if (++z->tries > ZT_DATA_TRIES)
                {
                    zfail(z, "Receiver stopped responding");
                    return false;
                }
                pos = start;
                frame = false;
                break;
            }
            t = zgethdr(z, ZT_DATA); // anything else: keep waiting
        }
    }
}

// ZFIN, the receiver's ZFIN, then "OO"
static void zsendfin(zrx_t *z)
{
    for (z->tries = 0; z->tries < 3; z->tries++)
    {
        ZStoHdr(z, 0);
        ZShHdr(z, ZFIN);
        if (zgethdr(z, ZT_REPLY) == ZFIN || z->ZState != Z_Send)
            break;
    }
    zputc('O', 0);
    zputc('O', 1);
}

// the name without its drive or directory, which is what a receiver is given
static void zbasename(char *to, const char *from)
{
    const char *p = from;
    for (; *from; from++)
        if (*from == '/' || *from == '\\' || *from == ':')
            p = from + 1;
    strncpy(to, p, FF_MAX_LFN - 1);
}

// the file the current program was loaded from, as MM.INFO$(CURRENT) finds it: the loader puts
// '#name on the program's first line
static char *zcurrentprogram(char *buf)
{
    char *tp = (char *)ProgMemory;
    if (*tp++ == 1 && *tp++ == 39 && *tp == '#' && tp[1])
    {
        strcpy(buf, "A:/");
        strncat(buf, tp + 1, FF_MAX_LFN - 4);
        return buf;
    }
    return NULL;
}

// called by cmd_xmodem() with cmdline at the word after the Z: SEND [file$] | RECEIVE [file$]
void cmd_zmodem(void)
{
    bool send = false;
    char *fname = NULL;
    char progname[FF_MAX_LFN] = {0};
    switch (toupper(*cmdline))
    {
    case 'S':
        send = true;
        break;
    case 'R':
        break;
    case 'C':
        error("Not available: programs are loaded from the SD card");
    default:
        error("Syntax");
    }
    while (IsAlpha(*cmdline))
        cmdline++;
    skipspace(cmdline);
    if (!(OptionConsole & 1))
        error("Serial Console Disabled");
    if (!InitSDCard())
        return;
    if (*cmdline && *cmdline != '\'')
        fname = getFstring(cmdline);
    else
    {
        // no file name: the current program's file, or a batch into the current directory
        if (CurrentLinePtr)
            error("Invalid in a program");
        if (send)
        {
            if (!zcurrentprogram(progname))
                error("No current program");
            fname = progname;
        }
        else
            fname = "*";
    }
    zcrcinit();
    zrx_t *z = GetTempMemory(sizeof(zrx_t));
    z->fnbr = FindFreeFileNbr();
    if (send)
    {
        if (!BasicFileOpen(fname, z->fnbr, FA_READ))
            return;
        z->fileopen = true;
        zbasename(z->name, fname);
        z->stage = GetTempMemory(ZMAXDATA);
    }
    else
    {
        z->target = fname;
        strncpy(z->name, fname, FF_MAX_LFN - 1);
        // "*", or a directory then "/*": a batch, each file under the name its sender gives it
        int n = strlen(z->name);
        if (n && z->name[n - 1] == '*' && (n == 1 || z->name[n - 2] == '/' || z->name[n - 2] == '\\'))
        {
            z->batch = true;
            z->dirlen = n - 1;
        }
        z->stage = GetTempMemory(ZSTAGE);
    }

    char BreakKeySave = BreakKey;
    BreakKey = 0; // the data can hold any byte, Ctrl-C included
    int abortsave = OptionFileErrorAbort;
    OptionFileErrorAbort = 0; // a file error must end the session cleanly, not jump out of it
    if (send)
    {
        z->ZState = Z_Send;
        z->ZPktState = Z_PktGetPAD;
        z->CanCount = 5;
        if (zsendinit(z) && zsendfile(z))
            zsendfin(z);
    }
    else
        zreceive(z);
    if (z->fileopen) // the session ended part way through a file: keep what arrived
    {
        if (!send)
            zflush(z);
        zfileclose(z);
    }
    OptionFileErrorAbort = abortsave;
    // let the other end finish before anything is printed. After a failure the sender may still be sending, and
    // none of that may reach the command prompt as keystrokes: wait for the line to go quiet (10 s at most)
    uint64_t quiet = ztime() + 10000000;
    while (zgetc(*z->err ? 500000 : 50000) >= 0 && ztime() < quiet)
        ;
    BreakKey = BreakKeySave;
    if (*z->err)
    {
        if (z->batch && z->files)
            error("$ (files received: %)", z->err, z->files); // the ones before it are complete
        error("$", z->err);
    }
    if (z->files)
        MMPrintString(send ? "Sent " : "Received ");
    else
        MMPrintString(send ? "Not wanted by the receiver: " : "Nothing received: ");
    if (z->files > 1) // a batch: how many, not the last one's name
    {
        PInt(z->files);
        MMPrintString(" files");
    }
    else
        MMPrintString(z->name);
    if (z->files)
    {
        MMPrintString(", ");
        PInt(z->bytes);
        MMPrintString(" bytes");
    }
    MMPrintString("\r\n");
}
