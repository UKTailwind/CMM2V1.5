/* The CSUB CallTable in the PicoMite's layout (PicoCFunctions.h): one slot a line, in order, at the offset its
   comment gives.  The offsets are the ABI, so the slots are append-only.  CFunctions.c includes this twice.
   CT(f) is the same function in both tables.  CTF(f) is a slot that takes or returns a float or a double: f in
   CallTableP (hard-float) and its wrapper SoftFP_f in CallTablePS (the PicoMite's soft-float convention). */
    CT(uSecP)                           /* 0x00 void uSec(unsigned long long us) */
    CT(putConsole)                      /* 0x04 */
    CT(getConsole)                      /* 0x08 */
    CT(ExtCfg)                          /* 0x0c */
    CT(ExtSetP)                         /* 0x10 void ExtSet(int pin, int val) */
    CT(ExtInpP)                         /* 0x14 int ExtInp(int pin) */
    CT(PinSetBit)                       /* 0x18 */
    CT(PinRead)                         /* 0x1c */
    CT(MMPrintString)                   /* 0x20 */
    CT(IntToStr)                        /* 0x24 */
    CT(CheckAbort)                      /* 0x28 */
    CT(GetMemory)                       /* 0x2c */
    CT(GetTempMemory)                   /* 0x30 */
    CT(FreeMemory)                      /* 0x34 */
    CT(&DrawRectangle)                  /* 0x38 */
    CT(&DrawBitmap)                     /* 0x3c */
    CT(DrawLine)                        /* 0x40 */
    CT(FontTable)                       /* 0x44 */
    CT(&ExtCurrentConfig)               /* 0x48 */
    CT(&HRes)                           /* 0x4c */
    CT(&VRes)                           /* 0x50 */
    CT(SoftReset)                       /* 0x54 */
    CT(error)                           /* 0x58 */
    CT(&ProgMemory)                     /* 0x5c */
    CT(SLOTRECBASE)                     /* 0x60 the record of each variable number (VREC): the array itself */
    CT(&varcnt)                         /* 0x64 */
    CT(&DrawBufferP)                    /* 0x68 void (*)(int x1, int y1, int x2, int y2, char *c) */
    CT(&ReadBuffer)                     /* 0x6c */
    CTF(FloatToStr)                     /* 0x70 */
    CT(CallExecuteProgram)              /* 0x74 */
    CT(&CFuncmSec)                      /* 0x78 */
    CT(CFuncRam)                        /* 0x7c */
    CT(&ScrollLCD)                      /* 0x80 */
    CTF(IntToFloat)                     /* 0x84 */
    CTF(FloatToInt64)                   /* 0x88 */
    CT(&Option)                         /* 0x8c the CMM2's struct option_s, not the PicoMite's */
    CTF(sin)                            /* 0x90 */
    CTF(DrawCircle)                     /* 0x94 */
    CT(DrawTriangle)                    /* 0x98 */
    CT(timer)                           /* 0x9c unsigned long long, microseconds */
    CTF(FMul)                           /* 0xa0 */
    CTF(FAdd)                           /* 0xa4 */
    CTF(FSub)                           /* 0xa8 */
    CTF(FDiv)                           /* 0xac */
    CTF(FCmp)                           /* 0xb0 */
    CTF(LoadFloat)                      /* 0xb4 */
    CT(&CFuncInt1)                      /* 0xb8 */
    CT(&CFuncInt2)                      /* 0xbc */
    CT(&CSubComplete)                   /* 0xc0 */
    CT(&AudioOutputP)                   /* 0xc4 */
    CT(IDiv)                            /* 0xc8 */
    CT(&AudioWrapP)                     /* 0xcc */
    CT(&CFuncInt3)                      /* 0xd0 */
    CT(&CFuncInt4)                      /* 0xd4 */
    CT(PIOExecuteP)                     /* 0xd8 */
    CT(&WritePageAddressExternal)       /* 0xdc the write page (PAGE WRITE) */
    CT(&NoBuffer)                       /* 0xe0 */
    CT(&NoBuffer)                       /* 0xe4 */
    CT(&NoBuffer)                       /* 0xe8 */
    CT(&DrawPixel)                      /* 0xec */
    CT(NoRefresh)                       /* 0xf0 */
    CTF(cos)                            /* 0xf4 */
    CTF(sqrt)                           /* 0xf8 */
    CTF(atan2)                          /* 0xfc */
    CTF(pow)                            /* 0x100 */
    CTF(SAdd)                           /* 0x104 */
    CTF(SSub)                           /* 0x108 */
    CTF(SMul)                           /* 0x10c */
    CTF(SDiv)                           /* 0x110 */
    CTF(SCmp)                           /* 0x114 */
    CTF(sinf)                           /* 0x118 */
    CTF(cosf)                           /* 0x11c */
    CTF(sqrtf)                          /* 0x120 */
    CTF(atan2f)                         /* 0x124 */
    CTF(powf)                           /* 0x128 */
    CTF(DtoS)                           /* 0x12c */
    CTF(StoD)                           /* 0x130 */
    CTF(StoI)                           /* 0x134 */
    CTF(ItoS)                           /* 0x138 */
    CT(IMod)                            /* 0x13c */
    CT(LMul)                            /* 0x140 */
    CT(LDiv)                            /* 0x144 */
    CT(LMod)                            /* 0x148 */
    CT(LShl)                            /* 0x14c */
    CT(LAsr)                            /* 0x150 */
    CT(LLsr)                            /* 0x154 */
    CT(memcpy)                          /* 0x158 */
    CT(memset)                          /* 0x15c */
    CT(memmove)                         /* 0x160 */
    CT(MtoC)                            /* 0x164 */
    CT(CtoM)                            /* 0x168 */
    CT(Mstrcpy)                         /* 0x16c */
    CT(Mstrcat)                         /* 0x170 */
    CT(Mstrcmp)                         /* 0x174 */
    CT(StrLeft)                         /* 0x178 the string cores (Functions.c) */
    CT(StrRight)                        /* 0x17c */
    CT(StrCase)                         /* 0x180 */
    CT(StrMid)                          /* 0x184 */
    CT(StrChar)                         /* 0x188 */
    CT(StrFill)                         /* 0x18c */
    CT(StrInstr)                        /* 0x190 */
    CTF(StrFormat)                      /* 0x194 */
    CTF(TimerVal)                       /* 0x198 */
    CTF(RndVal)                         /* 0x19c */
    CTF(log)                            /* 0x1a0 */
    CTF(tan)                            /* 0x1a4 */
    CT(StrVal)                          /* 0x1a8 */
    CT(TimerSet)                        /* 0x1ac */
    CT(&gui_fcolour)                    /* 0x1b0 */
    CT(&gui_bcolour)                    /* 0x1b4 */
    CTF(PinVal)                         /* 0x1b8 */
    CT(PinPut)                          /* 0x1bc */
