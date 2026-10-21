/***************************************************************************
CMM2 MMBasic
sprites.h

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
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef SPRITE_HEADER
#define SPRITE_HEADER
struct blitbuffer{
    char * blitbuffptr; //points to the sprite image, set to NULL if not in use
    char * blitstoreptr; //points to the stored background, set to NULL if not in use
    char  collisions[MAXCOLLISIONS+1]; //set to NULL if not in use, otherwise contains current collisions
    int64_t master; //the number of copies of this sprite; -1 for a copy, or a buffer not in use
    uint64_t lastcollisions;
    short x; //set to 1000 if not in use
    short y;
    short next_x; //set to 1000 if not in use
    short next_y;
    short w; 
    short h;
    int bc; //background colour;
    signed char layer; //defaults to 1 if not specified. If zero then scrolls with background
    signed char mymaster; //number of master if this is a copy
    char rotation;
    char active;
    char edges;
    short bgc[8];       // SPRITE(B, #n, side): the last SPRITE(B, #n)'s reach of the background (sprites.c)
};
extern struct blitbuffer blitbuff[MAXBLITBUF];	
extern int layer_in_use[MAXLAYER+1];
extern int sprites_in_use;
extern int LIFOpointer;
extern int zeroLIFOpointer;
extern char *COLLISIONInterrupt;
#define MAXSTOBJECTS 64                                 // the PicoMite's static objects (sprites.c)
struct stobject { short x, y, w, h; char active; };
extern struct stobject stobjects[MAXSTOBJECTS + 1];
extern char *STCollisionInterrupt;
extern int STCollisionFound;
extern void closeallstobjects(void);
extern int CollisionFound;
extern void closeallsprites(void);
extern void loadsprite(char *p);
extern void BlitShowBuff(int bnbr, int x1, int y1, int mode);  // every depth (sprites.c)
// a colour word as the current depth's page holds it: what the drawing commands write (FILL's FillNative), in the 32,
// 16 and 12-bit depths (RGB332 and RGB121 are Rgb121.h's)
#define NATIVE_32(c) (c)
#define NATIVE_16(c) ((BIT_5[((c) & 0xFF0000) >> 19] << 11) | (BIT_6[((c) & 0xFF00) >> 10] << 5) | BIT_5[((c) & 0xFF) >> 3])
#define NATIVE_12(c) ((BIT_4[((c) & 0xFF0000) >> 20] << 8) | (BIT_4[((c) & 0xFF00) >> 12] << 4) | BIT_4[((c) & 0xFF) >> 4] | \
                      (((c) & 0xF000000) >> 12))
extern uint32_t SpriteNative(uint32_t c);                       // in the current depth (any depth: sprites.c)
extern uint32_t SpriteMask(void);                               // the bits of that which are the colour
extern uint32_t SpriteKeyNative(void);                          // the transparent colour, so held and masked
extern void SpritesForget(void);                                // MODE: none showing, the images kept
#endif
#endif
