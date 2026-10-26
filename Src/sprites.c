/***************************************************************************

CMM2 MMBasic
sprites.c

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
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Ray.h"
int layer_in_use[MAXLAYER+1];
unsigned char LIFO[MAXBLITBUF];
unsigned char zeroLIFO[MAXBLITBUF];
#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a < b) ? b : a)
int LIFOpointer=0;
int zeroLIFOpointer=0;
int sprites_in_use=0;
char *COLLISIONInterrupt=NULL;
int CollisionFound=false;
int sprite_which_collided = -1;
// the PicoMite's static objects: rectangles of the scenery, not drawn, that every collision check tests the shown
// sprites against (a hit is &H80 + n in the sprite's list, and SPRITE STINTERRUPT's); SCROLL moves them with the
// background, SCROLLR those whose centre is in its rectangle, as the layer-0 sprites
struct stobject stobjects[MAXSTOBJECTS + 1];
char *STCollisionInterrupt = NULL;
int STCollisionFound = false;
int sprite_hit_st = -1;                                             // SPRITE(ST, COLLISION): the sprite of the last hit
int st_which_collided = -1;                                         // SPRITE(ST, OBJECT): its static object
void closeallstobjects(void){
    memset(stobjects, 0, sizeof(stobjects));
    STCollisionInterrupt = NULL;
    STCollisionFound = false;
    sprite_hit_st = -1;
    st_which_collided = -1;
}
// sprite k's overlaps with static objects into its list (while there is room); a hit is reported at every check while
// it lasts (unlike a sprite's, which is reported when it begins), as the PicoMite's
static void CheckSTCollisions(int k, int *n){
    int sx = blitbuff[k].x, sy = blitbuff[k].y, sw = blitbuff[k].w, sh = blitbuff[k].h;
    for(int i = 1; i <= MAXSTOBJECTS; i++){
        if(!stobjects[i].active) continue;
        if(sx + sw <= stobjects[i].x || sx >= stobjects[i].x + stobjects[i].w ||
           sy + sh <= stobjects[i].y || sy >= stobjects[i].y + stobjects[i].h) continue;
        if(*n < MAXCOLLISIONS) blitbuff[k].collisions[(*n)++] = 0x80 | i;
        STCollisionFound = true;
        if(STCollisionInterrupt != NULL) IntSignal();
        sprite_hit_st = k;
        st_which_collided = i;
    }
}
// a static object's centre moved by dx, dy, wrapped within xmin..xmax, ymin..ymax (SCROLL: the screen; SCROLLR: its
// rectangle, for those whose centre is in it)
static void StaticsMove(int dx, int dy, int xmin, int ymin, int xmax, int ymax, int inside){
    for(int i = 1; i <= MAXSTOBJECTS; i++){
        if(!stobjects[i].active) continue;
        int xs = stobjects[i].x + (stobjects[i].w >> 1), ys = stobjects[i].y + (stobjects[i].h >> 1);
        if(inside && !(xs <= xmax && xs >= xmin && ys <= ymax && ys >= ymin)) continue;
        xs += dx;
        if(xs > xmax) xs -= xmax - xmin + 1;
        if(xs < xmin) xs += xmax - xmin + 1;
        ys += dy;
        if(ys > ymax) ys -= ymax - ymin + 1;
        if(ys < ymin) ys += ymax - ymin + 1;
        stobjects[i].x = xs - (stobjects[i].w >> 1);
        stobjects[i].y = ys - (stobjects[i].h >> 1);
    }
}
static int hideall=0;
struct blitbuffer __attribute__((section(".d2free"))) blitbuff[MAXBLITBUF];		//Buffer pointers for the BLIT command (in D2 RAM,
																				// out of DTCM for the stack: main() zeroes it)
const int colours[16]={0x00,0xFF,0x4000,0x40ff,0x8000,0x80ff,0xff00,0xffff,0xff0000,0xff00FF,0xff4000,0xff40ff,0xff8000,0xff80ff,0xffff00,0xffffff};
void LIFOadd(int n){
    int i, j=0;
    for(i=0; i<LIFOpointer; i++){
        if(LIFO[i]!=n){
            LIFO[j]=LIFO[i];
            j++;
        }
    }
    LIFO[j]=n;
    LIFOpointer=j+1;
}
void LIFOremove(int n){
    int i, j=0;
    for(i=0; i<LIFOpointer; i++){
        if(LIFO[i]!=n){
            LIFO[j]=LIFO[i];
            j++;
        }
    }
    LIFOpointer=j;
}
void LIFOswap(int n, int m){
    int i;
    for(i=0; i<LIFOpointer; i++){
        if(LIFO[i]==n)LIFO[i]=m;
    }
}
void zeroLIFOadd(int n){
    int i, j=0;
    for(i=0; i<zeroLIFOpointer; i++){
        if(zeroLIFO[i]!=n){
            zeroLIFO[j]=zeroLIFO[i];
            j++;
        }
    }
    zeroLIFO[j]=n;
    zeroLIFOpointer=j+1;
}
void zeroLIFOremove(int n){
    int i, j=0;
    for(i=0; i<zeroLIFOpointer; i++){
        if(zeroLIFO[i]!=n){
            zeroLIFO[j]=zeroLIFO[i];
            j++;
        }
    }
    zeroLIFOpointer=j;
}
void zeroLIFOswap(int n, int m){
    int i;
    for(i=0; i<zeroLIFOpointer; i++){
        if(zeroLIFO[i]==n)zeroLIFO[i]=m;
    }
}
uint8_t convert_8bit(uint32_t c){                                  // (RGB121 in a 4-bit mode, whose sprites are a byte a pixel)
    return NATIVE8(c);
}
uint16_t convert_12bit(uint32_t c){
	uint32_t red,green,blue;
	red=BIT_4[((c & 0xFF0000)>>20)]<<8;
    green=BIT_4[((c & 0xFF00)>>12)]<<4;
    blue=BIT_4[((c & 0xFF)>>4)];
    return red|green|blue;
}

uint16_t convert_16bit(uint32_t c){
	uint32_t red,green,blue;
    red=BIT_5[((c & 0xFF0000)>>19)]<<11;
    green=BIT_6[((c & 0xFF00)>>10)]<<5;
    blue=BIT_5[((c & 0xFF)>>3)];
    return red|green|blue;
}

/* A sprite's image is a colour word for each pixel, the CMM2's own (what RGB() gives and a BOX draws: red, green and
   blue in bits 0-23, the transparency 0-15 in bits 24-27, 15 opaque), whatever the colour depth, so that a change of
   mode keeps it (Peter, 21 Oct: "This would mean the mode could be changed and the sprites would still be usable").
   A pixel is transparent when its colour, as the depth's page holds it (its transparency left out), is SpriteKey's:
   black unless SET TRANSPARENT has made it one of the PicoMite's 16 colours. That is the PicoMite's test in the 4-bit
   depth (an RGB121 index), and the old engine's native 0 with black; it holds for a sprite READ from the screen too.
   Its saved background is the page's own pixels, read and restored raw, SPRITEBG bytes each (enough for any depth).
   docs/Sprite_Plan.md. */
#define SPRITEBG 4
static uint32_t SpriteKey = 0;
// a colour word as the current depth's page holds it, and the bits of that which are the colour (not the transparency);
// NATIVE_32, _16 and _12 are in sprites.h (RAY's too: Ray.c)
uint32_t SpriteNative(uint32_t c){
    switch(VideoColour){
        case 32: return NATIVE_32(c);
        case 16: return NATIVE_16(c);
        case 12: return NATIVE_12(c);
        case 4: return RGB121(c);
        case 1: return RGB121(c) != 0;                              // (the PicoMite's MODE 1 sprites: a nibble not 0 is set)
        default: return RGB332(c);
    }
}
uint32_t SpriteMask(void){
    return VideoColour == 32 ? 0xFFFFFF : VideoColour == 16 ? 0xFFFF : VideoColour == 12 ? 0x0FFF : 0xFF;
}
// the transparent colour as the current depth's page holds it, its transparency left out: a sprite's pixel of it is not
// drawn (SHOW's test, and RAY's: Ray.c)
uint32_t SpriteKeyNative(void){
    return SpriteNative(SpriteKey) & SpriteMask();
}
// inv[level] for SpriteWord: the first input that a BIT table maps to that level, or above it (the RBIT tables of OPTION
// VGA OUTPUT LOW and MEDIUM are not the inverses of their BIT tables: a sprite read would draw back other pixels)
static void SpriteLevels(uint8_t *inv, const char *bit, int n, int shift){
    int i = 0;
    for(int l = 0; l < n; l++) {
        while(i < n - 1 && (uint8_t)bit[i] < l) i++;
        inv[l] = i << shift;
    }
}
// a pixel n of the current depth's page as a colour word (SPRITE READ): the inverse of NATIVE_*, so a sprite read from
// the screen draws back the same pixels. A black pixel is black, which SpriteKey makes transparent, as it was.
static uint32_t SpriteWord(uint32_t n, const uint8_t *i4, const uint8_t *i5, const uint8_t *i6){
    int r, g, b;
    switch(VideoColour){
        case 32: return n;
        case 16: return 0x0F000000 | i5[(n >> 11) & 31] << 16 | i6[(n >> 5) & 63] << 8 | i5[n & 31];
        case 12: return (n & 0xF000) << 12 | i4[(n >> 8) & 15] << 16 | i4[(n >> 4) & 15] << 8 | i4[n & 15];
        case 4: return 0x0F000000 | colours[n & 15];                // (the PicoMite's 16 colours: RGB121() gives n back)
        case 1: return n ? 0x0FFFFFFF : 0x0F000000;                 // (set: white, as it draws back)
        default:                                                    // RGB332
            r = (n >> 5) & 7; g = (n >> 2) & 7; b = n & 3;
            return 0x0F000000 | (r << 5 | r << 2 | r >> 1) << 16 | (g << 5 | g << 2 | g >> 1) << 8 | b * 0x55;
    }
}
// sprite bnbr made w x h and not shown (its buffers as they are)
static void SpriteFields(int bnbr, int w, int h){
    struct blitbuffer *b = &blitbuff[bnbr];
    b->bc = 0;
    b->w = w;
    b->h = h;
    b->master = 0;
    b->mymaster = -1;
    b->x = b->y = 10000;
    b->layer = -1;
    b->next_x = b->next_y = 10000;
    b->active = false;
    b->lastcollisions = 0;
    b->edges = 0;
}
// a new sprite bnbr of w x h: its image (w * h words, zeroed: transparent) and its background store, not shown
static void SpriteNew(int bnbr, int w, int h){
    blitbuff[bnbr].blitbuffptr = GetMemory(w * h * 4);
    blitbuff[bnbr].blitstoreptr = GetMemory(w * h * SPRITEBG);
    SpriteFields(bnbr, w, h);
}


// MODE: the screen is cleared, so no sprite is showing (their backgrounds are gone), but their images are kept: they
// are colour words, the same in every mode
void SpritesForget(void){
    for(int i = 0; i < MAXBLITBUF; i++){
        if(i <= MAXLAYER) layer_in_use[i] = 0;
        blitbuff[i].active = false;
        blitbuff[i].layer = -1;
        blitbuff[i].x = blitbuff[i].y = 10000;
        blitbuff[i].next_x = blitbuff[i].next_y = 10000;
        blitbuff[i].lastcollisions = 0;
        blitbuff[i].edges = 0;
        memset(blitbuff[i].collisions, 0, sizeof(blitbuff[i].collisions));
    }
    LIFOpointer = 0;
    zeroLIFOpointer = 0;
    sprites_in_use = 0;
    hideall = 0;
}
void closeallsprites(void){
    int i;
    for(i = 0; i < MAXBLITBUF; i++) {
        if(i<=MAXLAYER)layer_in_use[i]=0;
        if(i){
        	if(blitbuff[i].mymaster==-1)FreeMemorySafe((void *)&blitbuff[i].blitbuffptr);
        	FreeMemorySafe((void *)&blitbuff[i].blitstoreptr);
        }
        blitbuff[i].blitbuffptr = NULL;
        blitbuff[i].blitstoreptr = NULL;
        blitbuff[i].master=-1;
        blitbuff[i].mymaster=-1;
        blitbuff[i].x=10000;
        blitbuff[i].y=10000;
        blitbuff[i].w=0;
        blitbuff[i].h=0;
        blitbuff[i].next_x = 10000;
        blitbuff[i].next_y = 10000;
        blitbuff[i].bc=0;
        blitbuff[i].layer=-1;
		blitbuff[i].active=false;
        blitbuff[i].edges=0;
        blitbuff[i].rotation=0;                                     // (and the collision state: it outlived a RUN, so a
        blitbuff[i].lastcollisions=0;                               // program's first SPRITE(T) could show the last one's)
        memset(blitbuff[i].collisions, 0, sizeof(blitbuff[i].collisions));
    }
    LIFOpointer=0;
    zeroLIFOpointer=0;
    sprites_in_use=0;
    hideall=0;
    closeallstobjects();                                            // (and the static objects, as the PicoMite's CLOSE ALL)
    SpriteKey = 0;                                                  // (and black transparent again)
}
// SPRITE(B, #n): how the background under shown sprite n meets it (the PicoMite's, ksinger's idea): its saved store,
// what was there when it was last drawn (sprites under it too), against the row and column spans of its opaque pixels.
// 0: nothing there (every pixel empty: the transparent colour, or a 12-bit layer's transparent pixel); 1: something in
// its rectangle but not within a span; 2: within one (a hole inside a span counts, as the PicoMite's). bgc[] for
// SPRITE(B, #n, side): 0-3 how far the hits reach in from the left, right, top and bottom edges (the furthest hit's
// offset + 1: 0 none, or all the way across), 4-7 the deepest hit from the row spans' left and right ends and the column
// spans' top and bottom (of each pair only the smaller kept, the depth from the nearer end: both 0 if equal). The spans
// are made each time: LOAD, READ and the key change images.
static int SpriteBackground(int bnbr){
    struct blitbuffer *b = &blitbuff[bnbr];
    short *g = b->bgc;
    int w = b->w, h = b->h, x, y, r = 0, rotation = b->rotation;
    int bpp = VideoColour == 32 ? 4 : (VideoColour == 12 || VideoColour == 16) ? 2 : 1;
    uint32_t *img = (uint32_t *)b->blitbuffptr, mask = SpriteMask(), key = SpriteNative(SpriteKey) & mask;
    for(x = 0; x < 8; x++) g[x] = 0;
    if(img == NULL || !b->active) return 0;
    short *left = GetTempMemory((2 * h + 2 * w) * sizeof(short)), *right = left + h, *top = right + h, *bottom = top + w;
    for(x = 0; x < w; x++){
        top[x] = SHRT_MAX;
        bottom[x] = -1;
    }
    for(y = 0; y < h; y++){
        left[y] = w;
        right[y] = -1;
        for(x = 0; x < w; x++){
            if((SpriteNative(img[y * w + x]) & mask) == key) continue;
            if(left[y] == w) left[y] = x;
            right[y] = x;
            bottom[x] = y;
            if(top[x] == SHRT_MAX) top[x] = y;
        }
    }
    int g0 = -1, g1 = SHRT_MAX, g2 = -1, g3 = SHRT_MAX, g4 = -1, g5 = -1, g6 = -1, g7 = -1;
    for(y = 0; y < h; y++){
        for(x = 0; x < w; x++){
            int k = y * w + x;
            uint32_t n = bpp == 4 ? ((uint32_t *)b->blitstoreptr)[k] : bpp == 2 ? ((uint16_t *)b->blitstoreptr)[k] :
                         ((uint8_t *)b->blitstoreptr)[k];
            if(VideoColour == 12 && (n & 0xF000) == 0) continue;    // a layer's transparent pixel
            if((n & mask) == key) continue;
            if(x > g0) g0 = x;                                      // the hits' rectangle
            if(x < g1) g1 = x;
            if(y > g2) g2 = y;
            if(y < g3) g3 = y;
            if(r == 0) r = 1;
            int bx = rotation & 1 ? w - 1 - x : x, by = rotation & 2 ? h - 1 - y : y;   // in the image's own orientation
            if(bx >= left[by] && bx <= right[by]){
                if(bx - left[by] > g4){ g4 = bx - left[by]; r = 2; }
                if(right[by] - bx > g5){ g5 = right[by] - bx; r = 2; }
            }
            if(by >= top[bx] && by <= bottom[bx]){
                if(by - top[bx] > g6){ g6 = by - top[bx]; r = 2; }
                if(bottom[bx] - by > g7){ g7 = bottom[bx] - by; r = 2; }
            }
        }
    }
    g[0] = g0 == -1 ? 0 : g0 + 1;
    g[1] = g1 == SHRT_MAX ? 0 : w - g1;
    g[2] = g2 == -1 ? 0 : g2 + 1;
    g[3] = g3 == SHRT_MAX ? 0 : h - g3;
    g[4] = g4 + 1;                                                  // (-1 + 1: none)
    g[5] = g5 + 1;
    g[6] = g6 + 1;
    g[7] = g7 + 1;
    if(g[0] == w) g[0] = 0;
    if(g[1] == w) g[1] = 0;
    if(g[2] == h) g[2] = 0;
    if(g[3] == h) g[3] = 0;
    if(g[4] != 0 && g[5] != 0){
        if(g[4] < g[5]) g[5] = 0;
        else if(g[4] > g[5]) g[4] = 0;
        else g[4] = g[5] = 0;                                       // (both sides: a hit from top or bottom)
    }
    if(g[6] != 0 && g[7] != 0){
        if(g[6] < g[7]) g[7] = 0;
        else if(g[6] > g[7]) g[6] = 0;
        else g[6] = g[7] = 0;
    }
    return r;
}

void fun_sprite(void){
    int bnbr=0, w=-1, h=-1,t=0, x=10000, y=10000, l=0, n, c=0;
    char *q;
    if((q = checkstring(ep, "RAY")) != NULL && *q == ',') {         // RAY(, which the tokeniser writes SPRITE(RAY, (Ray.c)
        ep = q + 1;
        fun_ray();
        return;
    }
    getargs(&ep, 5,",");
    if(checkstring(argv[0], "B")){                                  // SPRITE(B, #n [, side 0-7])
        if(argc < 3) error("Syntax");
        if(*argv[2] == '#') argv[2]++;
        bnbr = getint(argv[2], 1, MAXBLITBUF - 1);
        targ = T_INT;
        if(argc == 5) iret = blitbuff[bnbr].bgc[getint(argv[4], 0, 7)];
        else iret = SpriteBackground(bnbr);
        return;
    }
    if(checkstring(argv[0], "ST")){                                 // the PicoMite's static objects
        if(argc < 3) error("Syntax");
        targ = T_INT;
        if(checkstring(argv[2], "COLLISION")) { iret = sprite_hit_st; return; }     // the sprite of the last hit
        if(checkstring(argv[2], "OBJECT")) { iret = st_which_collided; return; }    // and its object
        if(argc < 5) error("Syntax");
        if(*argv[2] == '#') argv[2]++;
        struct stobject *s = &stobjects[getint(argv[2], 1, MAXSTOBJECTS)];
        if(checkstring(argv[4], "X")) iret = s->active ? s->x : -1;
        else if(checkstring(argv[4], "Y")) iret = s->active ? s->y : -1;
        else if(checkstring(argv[4], "W")) iret = s->active ? s->w : -1;
        else if(checkstring(argv[4], "H")) iret = s->active ? s->h : -1;
        else if(checkstring(argv[4], "A")) iret = s->active;
        else error("Syntax");
        return;
    }
    if(checkstring(argv[0], "W")) t=1;
    else if(checkstring(argv[0], "H")) t=2;
    else if(checkstring(argv[0], "X")) t=3;
    else if(checkstring(argv[0], "Y")) t=4;
    else if(checkstring(argv[0], "L")) t=5;
    else if(checkstring(argv[0], "C")) t=6;
    else if(checkstring(argv[0], "V")) t=7;
    else if(checkstring(argv[0], "T")) t=8;
    else if(checkstring(argv[0], "E")) t=9;
    else if(checkstring(argv[0], "D")) t=10;
    else if(checkstring(argv[0], "A")) t=11;
    else if(checkstring(argv[0], "N")) t=12;
    else if(checkstring(argv[0], "S")) t=13;
    else error("Syntax");
    if(t<12){
        if(argc<3)error("Syntax");
        if(*argv[2] == '#') argv[2]++;
        bnbr = getint(argv[2],0,MAXBLITBUF-1);
        if(bnbr==0){
            if(argc==5 && !(t==7 || t==10)){
                n=getint(argv[4],1,blitbuff[0].collisions[0]);
                c=blitbuff[0].collisions[n];
            } else c=blitbuff[0].collisions[0];
        }
        if(blitbuff[bnbr].blitbuffptr!=NULL){
            w=blitbuff[bnbr].w;
            h=blitbuff[bnbr].h;
        } 
        if(blitbuff[bnbr].active){
            x=blitbuff[bnbr].x;
            y=blitbuff[bnbr].y;
            l=blitbuff[bnbr].layer;
            if(argc==5 && !(t==7 || t==10)){
                 n=getint(argv[4],1,blitbuff[bnbr].collisions[0]);
                 c=blitbuff[bnbr].collisions[n];
            } else c=blitbuff[bnbr].collisions[0];
        }
    }
    if(t==1)iret=w;
    else if(t==2)iret=h;
    else if(t==3) {if(blitbuff[bnbr].active)iret=x; else iret=10000;}
    else if(t==4) {if(blitbuff[bnbr].active)iret=y; else iret=10000;}
    else if(t==5) {if(blitbuff[bnbr].active)iret=l; else iret=-1;}
    else if(t==8) {if(blitbuff[bnbr].active)iret=blitbuff[bnbr].lastcollisions; else iret=0;}
    else if(t==9) {if(blitbuff[bnbr].active)iret=blitbuff[bnbr].edges; else iret=0;}
    else if(t==6) {if(blitbuff[bnbr].collisions[0])iret=c; else iret=-1;}
	else if(t==11) iret=(int64_t)((uint32_t)blitbuff[bnbr].blitbuffptr);
    else if(t==7){
    	int rbnbr=0;
    	int x1=0,y1=0,h1=0,w1=0;
    	double vector;
    	if(argc<5)error("Syntax");
        if(*argv[4] == '#') argv[4]++;
        rbnbr = getint(argv[4],1,MAXBLITBUF-1);
        if(blitbuff[rbnbr].blitbuffptr!=NULL){
            w1=blitbuff[rbnbr].w;
            h1=blitbuff[rbnbr].h;
        }
        if(blitbuff[rbnbr].active){
            x1=blitbuff[rbnbr].x;
            y1=blitbuff[rbnbr].y;
        }
        if(!(blitbuff[bnbr].active && blitbuff[rbnbr].active))fret=-1.0;
        else {
        	x+=w/2;
        	y+=h/2;
        	x1+=w1/2;
        	y1+=h1/2;
        	y1-=y;
        	x1-=x;
        	vector=atan2(y1,x1);
        	vector+=PI_VALUE/2.0;
        	if(vector<0)vector+=PI_VALUE*2.0;
        	fret=vector;
        }
        targ=T_NBR;
        return;
    } else if(t==10){
        	int rbnbr=0;
        	int x1=0,y1=0,h1=0,w1=0;
        	if(argc<5)error("Syntax");
            if(*argv[4] == '#') argv[4]++;
            rbnbr = getint(argv[4],1,MAXBLITBUF-1);
            if(blitbuff[rbnbr].blitbuffptr!=NULL){
                w1=blitbuff[rbnbr].w;
                h1=blitbuff[rbnbr].h;
            }
            if(blitbuff[rbnbr].active){
                x1=blitbuff[rbnbr].x;
                y1=blitbuff[rbnbr].y;
            }
            if(!(blitbuff[bnbr].active && blitbuff[rbnbr].active))fret=-1.0;
            else {
            	x+=w/2;
            	y+=h/2;
            	x1+=w1/2;
            	y1+=h1/2;
            	fret=sqrt((x1-x)*(x1-x) + (y1-y)*(y1-y));
            }
            targ=T_NBR;
            return;
    } else if(t==12) {
            if(argc==3){
                n=getint(argv[2],0,MAXLAYER);
                iret=layer_in_use[n];
            } else iret=sprites_in_use;
    } else if(t==13) iret = sprite_which_collided;
    else {
    }
    targ = T_INT;   
}
// an edge of the screen as a collision (0xF1 left, 0xF2 top, 0xF4 right, 0xF8 bottom), while there is room in
// collisions[]: an edge left out is reported by a later move, as its bit in edges stays clear
static void edgecollision(int bnbr, int *n, int out, int bit){
	if(!out) blitbuff[bnbr].edges &= ~bit;
	else if(!(blitbuff[bnbr].edges & bit) && *n < MAXCOLLISIONS){
		blitbuff[bnbr].edges |= bit;
		blitbuff[bnbr].collisions[(*n)++] = 0xF0 | bit;
	}
}
void checklimits(int bnbr, int *n){
    int maxW=PageTable[WritePage].xmax;
    int maxH=PageTable[WritePage].ymax;
	if(*n <= MAXCOLLISIONS) blitbuff[bnbr].collisions[*n]=0;
	edgecollision(bnbr, n, blitbuff[bnbr].x < 0, 1);
	edgecollision(bnbr, n, blitbuff[bnbr].y < 0, 2);
	edgecollision(bnbr, n, blitbuff[bnbr].x + blitbuff[bnbr].w > maxW, 4);
	edgecollision(bnbr, n, blitbuff[bnbr].y + blitbuff[bnbr].h > maxH, 8);
}

// may sprites k and kk collide: both showing, and on the same layer or either on layer 0 (which meets every layer)
static inline int SpritesMeet(int k, int kk){
    return kk != k && blitbuff[kk].active &&
           (blitbuff[k].layer == 0 || blitbuff[kk].layer == 0 || blitbuff[k].layer == blitbuff[kk].layer);
}
// do their rectangles overlap (touching counts, as it always has)
static inline int SpritesOverlap(int k, int kk){
    return !(blitbuff[kk].x + blitbuff[kk].w < blitbuff[k].x || blitbuff[kk].x > blitbuff[k].x + blitbuff[k].w ||
             blitbuff[kk].y + blitbuff[kk].h < blitbuff[k].y || blitbuff[kk].y > blitbuff[k].y + blitbuff[k].h);
}
// sprite k's collisions that have begun into its list from collisions[1] (its lastcollisions bit kk-1 kept for each one
// that lasts, so it is reported once), then the screen edges it has reached; the number of entries + 1. 'both' also
// sets k's bit in each other sprite's mask (a sprite that moved alone: its meeting is not news to the other either).
static int SpriteCollisions(int k, int both){
    int kk, n = 1;
    uint64_t mine = (uint64_t)1 << (k - 1);
    mymemset(blitbuff[k].collisions, 0, MAXCOLLISIONS);
    for(kk = 1; kk < MAXBLITBUF; kk++){
        uint64_t bit = (uint64_t)1 << (kk - 1);
        if(kk == k) continue;
        if(SpritesMeet(k, kk) && SpritesOverlap(k, kk)){
            if(n < MAXCOLLISIONS && !(blitbuff[k].lastcollisions & bit)) blitbuff[k].collisions[n++] = kk;
            blitbuff[k].lastcollisions |= bit;
            if(both) blitbuff[kk].lastcollisions |= mine;
        } else {
            blitbuff[k].lastcollisions &= ~bit;
            if(both) blitbuff[kk].lastcollisions &= ~mine;
        }
    }
    checklimits(k, &n);
    CheckSTCollisions(k, &n);
    if(n > 1) blitbuff[k].collisions[0] = n - 1;
    return n;
}
// after a sprite bnbr has been shown, or (bnbr 0) after MOVE, SCROLL, SCROLLR or RESTORE has moved everything: the
// collisions found, SPRITE(C ...) and SPRITE(S) for them, and the collision interrupt. Which collided is in
// blitbuff[0]'s list after a whole-scene check. CollisionFound stays set until the interrupt has run (MM_Misc.c).
void ProcessCollisions(int bnbr){
    int k, bcol = 1;
    if(COLLISIONInterrupt == NULL) CollisionFound = false;
    if(STCollisionInterrupt == NULL) STCollisionFound = false;
    sprite_which_collided = -1;
    mymemset(blitbuff[0].collisions, 0, MAXCOLLISIONS);
    if(bnbr != 0){                                                  // one sprite has moved
        if(SpriteCollisions(bnbr, 1) > 1){
            CollisionFound = true;
            if(COLLISIONInterrupt != NULL) IntSignal();
            sprite_which_collided = bnbr;
        }
    } else {                                                        // everything has moved
        for(k = 1; k < MAXBLITBUF; k++){
            if(!blitbuff[k].active) continue;
            if(SpriteCollisions(k, 0) > 1 && bcol < MAXCOLLISIONS) blitbuff[0].collisions[bcol++] = k;
        }
        if(bcol > 1){
            CollisionFound = true;
            if(COLLISIONInterrupt != NULL) IntSignal();
            sprite_which_collided = 0;
            blitbuff[0].collisions[0] = bcol - 1;
        }
    }
}

void blithide(int bnbr, int free){
    int w, h, x1, y1;
    w=blitbuff[bnbr].w;
    h=blitbuff[bnbr].h;
    x1 = blitbuff[bnbr].x;
    y1 = blitbuff[bnbr].y;
	blitbuff[bnbr].active=0;
    DrawBufferFast(x1,y1,x1+w-1,y1+h-1,blitbuff[bnbr].blitstoreptr);
}

// Sprite bnbr drawn at x1, y1 over the write page: SHOW, SHOW SAFE, SWAP, WRITE and every redraw. Its image is read in
// its orientation (rotation bit 0 mirrors it left to right, bit 1 top to bottom), and each pixel that is not its
// transparent value bc is drawn, every pixel with mode 8. mode 0: a redraw (the old place is not restored), 1: SHOW (it
// is, if the sprite is shown), 2: SWAP (the saved background is kept), 4: WRITE (not restored, not made shown); | 8.
// The image is colour words, converted as they are drawn; the background is the depth's pixels, a byte each in the 8-bit
// and 4-bit depths (the 4-bit one's RGB121 in the low nibble), two in the 12-bit and 16-bit, four in the 32-bit.
#define SHOWLOOP(T, NATIVE) { \
    uint32_t *img = (uint32_t *)b->blitbuffptr, mask = SpriteMask(), key = SpriteNative(SpriteKey) & mask; \
    T *bg = (T *)store, *out = (T *)rr; \
    for(y = 0; y < h; y++) { \
        uint32_t *row = img + (rotation & 2 ? h - 1 - y : y) * w; \
        for(x = 0; x < w; x++) { \
            uint32_t a = rotation & 1 ? row[w - 1 - x] : row[x]; \
            T n = (T)(NATIVE(a)), s = *bg++; \
            *out++ = ((n & mask) != key || (mode & 8)) ? n : s; \
        } \
    } \
}
// BlitShowBuff's output, in on-chip RAM, where writing it and DrawBufferFast's reading it are fast: the 6 KB of .d2free
// past noisetable and blitbuff (a 32 x 32 sprite in any depth, 64 x 64 in the 8-bit ones), for a bigger sprite a block
// of the on-chip heap kept while the heap lasts (HeapGen), and past what that has, the SDRAM heap for the one draw
#define SHOWOUT 4992                                                // (blitbuff's bgc took 1 KB of it)
static char __attribute__((section(".d2free"), aligned(8))) ShowOut[SHOWOUT];
static char *ShowBig = NULL;
static int ShowBigSize = 0;
static uint32_t ShowBigGen = 0;
static char *ShowBuffer(int n){
    if(n <= SHOWOUT) return ShowOut;
    if(ShowBigGen != HeapGen){                                      // (wiped with the heap since)
        ShowBig = NULL;
        ShowBigSize = 0;
        ShowBigGen = HeapGen;
    }
    if(n > ShowBigSize){
        if(ShowBig != NULL) FreeMemory(ShowBig);
        ShowBig = GetInternalMemoryNull(n);
        ShowBigSize = ShowBig != NULL ? n : 0;
    }
    return ShowBig;                                                 // (NULL: the on-chip heap has not that much)
}
void BlitShowBuff(int bnbr, int x1, int y1, int mode){
    struct blitbuffer *b = &blitbuff[bnbr];
    int x, y, w = b->w, h = b->h, rotation = b->rotation & 3;
    int bpp = VideoColour == 32 ? 4 : (VideoColour == 12 || VideoColour == 16) ? 2 : 1;
    char *rr, *store = b->blitstoreptr;
    if(b->rotation & 4) mode |= 8;                                  // orientations 4-7: every pixel (the PicoMite's)
    ReadPage = WritePage;
    if(b->blitbuffptr == NULL) return;
    if(!(mode == 0 || mode & 4) && b->active)
        DrawBufferFast(b->x, b->y, b->x + w - 1, b->y + h - 1, b->blitstoreptr);
    if(mode & 4) store = GetTempMemory(w * h * SPRITEBG);           // WRITE: a sprite showing keeps its place and its
    else {                                                          // background (they were overwritten: the PicoMite's too)
        b->x = x1;
        b->y = y1;
    }
    if(mode != 2) ReadBufferFast(x1, y1, x1 + w - 1, y1 + h - 1, store);
    // the old screen is now stored, with its place
    if((rr = ShowBuffer(w * h * bpp)) == NULL) rr = GetMemory(w * h * bpp);
    switch(VideoColour){
        case 32: SHOWLOOP(uint32_t, NATIVE_32) break;
        case 16: SHOWLOOP(uint16_t, NATIVE_16) break;
        case 12: SHOWLOOP(uint16_t, NATIVE_12) break;
        case 4: SHOWLOOP(uint8_t, RGB121) break;
        case 1: SHOWLOOP(uint8_t, SpriteNative) break;
        default: SHOWLOOP(uint8_t, RGB332) break;
    }
    DrawBufferFast(x1, y1, x1 + w - 1, y1 + h - 1, rr);
    if(!(mode & 4)) b->active = 1;
    if(rr != ShowOut && rr != ShowBig) FreeMemory(rr);
}
#undef SHOWLOOP

int sumlayer(void){
    int i,j=0;
    for(i=0; i<=MAXLAYER ;i++)j+=layer_in_use[i];
    return j;
}
void loadarray(char *p){                                           // SPRITE LOADARRAY #n, w, h, array(): a colour a pixel
    int bnbr, w, h, size, i;
	int maxH = PageTable[WritePage].ymax;
    int maxW = PageTable[WritePage].xmax;
    MMFLOAT *a3float = NULL;
    int64_t *a3int = NULL;
    uint32_t *q;
    getargs(&p, 7, ",");
    if(*argv[0] == '#') argv[0]++;
    bnbr = getint(argv[0], 1, MAXBLITBUF - 1);
    if(blitbuff[bnbr].blitbuffptr == NULL){
    	w = getint(argv[2], 1, maxW);
    	h = getint(argv[4], 1, maxH);
        // the array whole, as the PicoMite's: a!(5) read w*h values from element 5, past the end
        size = parsenumberarray(argv[6], &a3float, &a3int, 4, 1, NULL, true, NULL) - 1;
        if(size < w * h - 1) error("Array Dimensions");
        SpriteNew(bnbr, w, h);
        q = (uint32_t *)blitbuff[bnbr].blitbuffptr;
        for(i = 0; i < w * h; i++) *q++ = a3float ? (uint32_t)(int)a3float[i] : (uint32_t)a3int[i];   // (the colour as given)
    } else error("Buffer already in use");
}
// SPRITE LOADPNG #n, f$ [, t] [, cutoff] [, x] [, y] [, w] [, h], the PicoMite's. A pixel whose alpha is above cutoff
// (1-254, default 30) keeps its colour; the others are colour t of the PicoMite's 16 (0, black: transparent unless SET
// TRANSPARENT says otherwise). A negative t instead makes the opaque pixels that would draw as black (in the depth: the
// transparent colour) colour -t. x, y, w, h: a part of the image, no bigger than the screen; it is decoded a row at a
// time, and is the sprite's only once every row has decoded. (The CMM2's third argument was a cut-off of 1-15.)
typedef struct {
    int xoff, yoff, sw, sh, cutoff, remap;
    uint32_t transparent, mask, black, *q;
} s_spritepng;
static int spritepngrow(void *v, unsigned y, const unsigned char *rgba){
    s_spritepng *c = (s_spritepng *)v;
    if((int)y < c->yoff) return 1;
    if((int)y >= c->yoff + c->sh) return 0;                         // (the rows past the part: not decoded)
    routinechecks(1);                                               // (as LOAD PNG: sound keeps playing)
    const unsigned char *rr = rgba + c->xoff * 4;
    for(int x = 0; x < c->sw; x++, rr += 4){
        uint32_t w;
        if(rr[3] > c->cutoff){
            w = 0x0F000000 | rr[0] << 16 | rr[1] << 8 | rr[2];
            if(c->remap >= 0 && (SpriteNative(w) & c->mask) == c->black) w = 0x0F000000 | colours[c->remap];
        } else w = c->transparent;
        *c->q++ = w;
    }
    return (int)y + 1 < c->yoff + c->sh;                            // (0 after the part's last row: decoding stops)
}
void loadpng(char *p){
	upng_t *upng;
    int maxW = PageTable[WritePage].xmax;
	int maxH = PageTable[WritePage].ymax;
    int bnbr, w, h, xoff = 0, yoff = 0, sw = -1, sh = -1, t = 0, remap = -1, cutoff = 30;
    uint32_t *img;
    getargs(&p, 15, ",");
    if(argc < 3) error("Syntax");
    if(*argv[0] == '#') argv[0]++;
    bnbr = getint(argv[0], 1, MAXBLITBUF - 1);
    if(blitbuff[bnbr].blitbuffptr != NULL) error("Buffer already in use");
    char *f = getCstring(argv[2]);
    if(argc >= 5 && *argv[4]){
        int tt = getint(argv[4], -15, 15);
        if(tt < 0) remap = -tt;
        else t = tt;
    }
    if(argc >= 7 && *argv[6]) cutoff = getint(argv[6], 1, 254);
    if(argc >= 9 && *argv[8]) xoff = getinteger(argv[8]);
    if(argc >= 11 && *argv[10]) yoff = getinteger(argv[10]);
    if(argc >= 13 && *argv[12]) sw = getinteger(argv[12]);
    if(argc == 15) sh = getinteger(argv[14]);
    if(xoff < 0 || yoff < 0) error("Coordinates");
    upng = upng_new_from_file(f);
    if(upng == NULL) return;
    upng_header(upng);
    upng_error_check(upng);                                         // (an unreadable or unsupported file: freed and reported)
    w = upng_get_width(upng);
    h = upng_get_height(upng);
    if(sw == -1) sw = w - xoff;
    if(sh == -1) sh = h - yoff;
    if(sw < 1 || sh < 1 || sw + xoff > w || sh + yoff > h){
        upng_free(upng);
        error("Coordinates");
    }
    if(sw > maxW || sh > maxH){
        upng_free(upng);
        error("Image too large");
    }
    img = GetMemory(sw * sh * 4);
    s_spritepng c = {.xoff = xoff, .yoff = yoff, .sw = sw, .sh = sh, .cutoff = cutoff, .remap = remap,
                     .transparent = 0x0F000000 | colours[t], .mask = SpriteMask(), .black = SpriteNative(0) & SpriteMask(),
                     .q = img};
    upng_decode_rows(upng, spritepngrow, &c);
    if(upng_get_error(upng) != UPNG_EOK) FreeMemory(img);           // (not the sprite's: it stays free)
    upng_error_check(upng);
    upng_free(upng);
    blitbuff[bnbr].blitbuffptr = (char *)img;
    blitbuff[bnbr].blitstoreptr = GetMemory(sw * sh * SPRITEBG);
    SpriteFields(bnbr, sw, sh);
}
// SPRITE LOADBMP #n, f$ [, x, y, w, h] (and BLIT LOAD, its other name), the PicoMite's: a BMP, or the part of it at x, y,
// w by h (no bigger than the screen), every pixel opaque (black: transparent unless SET TRANSPARENT says otherwise);
// decoded a row at a time into temporary memory, as the decoder's errors are raised inside it (cleanupAndError), and
// the sprite's only once every row has decoded
static struct { int xoff, yoff, sw, sh; uint32_t *img; } bmprows;
static bool SpriteBMPRow(int *imagewidth, int *imageheight, uint32_t *linedata, int *linenumber){
    int y = *linenumber - bmprows.yoff;
    if(y < 0 || y >= bmprows.sh) return true;
    uint32_t *d = bmprows.img + y * bmprows.sw, *s = linedata + bmprows.xoff;
    for(int x = 0; x < bmprows.sw; x++) *d++ = 0x0F000000 | (*s++ & 0xFFFFFF);   // (0x00RRGGBB a pixel)
    return true;
}
void loadbmp(char *p){
    int maxW = PageTable[WritePage].xmax;
	int maxH = PageTable[WritePage].ymax;
    int bnbr, w, h, xoff = 0, yoff = 0, sw = -1, sh = -1;
    char fp[STRINGSIZE];
    uint32_t *img;
    getargs(&p, 11, ",");
    if(argc < 3) error("Syntax");
    if(*argv[0] == '#') argv[0]++;
    bnbr = getint(argv[0], 1, MAXBLITBUF - 1);
    if(blitbuff[bnbr].blitbuffptr != NULL) error("Buffer already in use");
    strcpy(fp, getFstring(argv[2]));
    if(strchr(&fp[strlen(fp) > 4 ? strlen(fp) - 4 : 0], '.') == NULL) strcat(fp, ".bmp");
    if(argc >= 5 && *argv[4]) xoff = getinteger(argv[4]);
    if(argc >= 7 && *argv[6]) yoff = getinteger(argv[6]);
    if(argc >= 9 && *argv[8]) sw = getinteger(argv[8]);
    if(argc == 11) sh = getinteger(argv[10]);
    if(xoff < 0 || yoff < 0) error("Coordinates");
    if(!InitSDCard()) return;
    BMPfnbr = FindFreeFileNbr();
    if(!BasicFileOpen(fp, BMPfnbr, FA_READ)) return;
    decodeBMPheader(&w, &h);
    if(w < 1 || h < 1){
        FileClose(BMPfnbr);
        error("Invalid BMP file");
    }
    if(sw == -1) sw = w - xoff;
    if(sh == -1) sh = h - yoff;
    if(sw < 1 || sh < 1 || sw + xoff > w || sh + yoff > h){
        FileClose(BMPfnbr);
        error("Coordinates");
    }
    if(sw > maxW || sh > maxH){
        FileClose(BMPfnbr);
        error("Image too large");
    }
    img = GetTempMemory(sw * sh * 4);                               // (freed after the statement, or by an error)
    bmprows.xoff = xoff;
    bmprows.yoff = yoff;
    bmprows.sw = sw;
    bmprows.sh = sh;
    bmprows.img = img;
    linecallback = SpriteBMPRow;
    BMP_Result r = decodeBMP(false);                                // (the file's own row order: the rows are numbered)
    linecallback = NULL;
    FileClose(BMPfnbr);
    if(!r.success) error(r.errorMsg);                               // (the sprite stays free)
    blitbuff[bnbr].blitbuffptr = GetMemory(sw * sh * 4);
    memcpy(blitbuff[bnbr].blitbuffptr, img, sw * sh * 4);
    blitbuff[bnbr].blitstoreptr = GetMemory(sw * sh * SPRITEBG);
    SpriteFields(bnbr, sw, sh);
}
// written as SPRITE (its own token) and not as BLIT: the PicoMite's are two commands, which differ in WRITE and LOAD
static int SpriteCommand(void){
    static int sprite = -1;
    if(sprite < 0) sprite = GetCommandValue("Sprite");
    return cmdtoken == sprite;
}

// HIDE SAFE: the sprites above bnbr hidden, top first, then bnbr, then those above it drawn again; for a layer-0 sprite
// every layer sprite is above it. -1 is "not found" (index 0, the bottom of a list, was taken for it, as the PicoMite's
// was: the layer-0 sprites were then hidden and left so). The caller has checked bnbr is showing.
void hidesafe(int bnbr){
    int found = -1, zerofound = 0, i;
    for(i = LIFOpointer - 1; i >= 0; i--) {
    	blithide(LIFO[i], 0);
    	if(LIFO[i] == bnbr){
    		found = i;
    		break;
    	}
    }
    if(found < 0){
		for(i = zeroLIFOpointer - 1; i >= 0; i--){
			blithide(zeroLIFO[i], 0);
			if(zeroLIFO[i] == bnbr){
				found = i;
				zerofound = 1;
				break;
			}
		}
    }
    if(found < 0) return;                                           // (not showing: nothing was hidden)
    sprites_in_use--;
    layer_in_use[blitbuff[bnbr].layer]--;
    blitbuff[bnbr].x = 10000;
    blitbuff[bnbr].y = 10000;
    if(zerofound) zeroLIFOremove(bnbr);
    else LIFOremove(bnbr);
    blitbuff[bnbr].layer = -1;
    blitbuff[bnbr].next_x = 10000;
    blitbuff[bnbr].next_y = 10000;
	blitbuff[bnbr].lastcollisions = 0;
    blitbuff[bnbr].edges = 0;
    if(zerofound){                                                  // (the ones above it moved down into its place)
        for(i = found; i < zeroLIFOpointer; i++)
        	BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        for(i = 0; i < LIFOpointer; i++)
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
    } else {
        for(i = found; i < LIFOpointer; i++)
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
    }
}

// SHOW SAFE of a sprite showing: as HIDE SAFE, but bnbr is drawn at x, y in its place among the others
void showsafe(int bnbr, int x, int y){
    int found = -1, zerofound = 0, i;
    for(i = LIFOpointer - 1; i >= 0; i--) {
    	blithide(LIFO[i], 0);
    	if(LIFO[i] == bnbr){
    		found = i;
    		break;
    	}
    }
    if(found < 0){
		for(i = zeroLIFOpointer - 1; i >= 0; i--){
			blithide(zeroLIFO[i], 0);
			if(zeroLIFO[i] == bnbr){
				found = i;
				zerofound = 1;
				break;
			}
		}
    }
	BlitShowBuffer(bnbr, x, y, 1);
    if(zerofound){
        for(i = found + 1; i < zeroLIFOpointer; i++)
        	BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        for(i = 0; i < LIFOpointer; i++)
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
    } else {
        for(i = (found < 0 ? 0 : found + 1); i < LIFOpointer; i++)
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
    }
}

// the colours of a .SPR file's digits '0' to 'F', as colour words (BLACK is transparent, as SpriteKey says); with
// SPRITE LOAD's palette 1, the PicoMite's mode 1: its 16 colours in index order (colours[], '0' black)
static const char digits[] = "0123456789ABCDEF";
static const uint32_t spr_colours[16] = {BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE, LITEGRAY, GRAY, ORANGE,
                                         PINK, GOLD, SALMON, BEIGE, BROWN};
void loadsprite(char *p){                                           // SPRITE LOAD f$ [, start] [, palette 0-1]
    int fnbr, width=0, number=0,height=0, newsprite=1, startsprite=1, bnbr, lc = 0, i, palette = 0;
    char *fname;
	uint32_t *qqq = NULL;
    char buff[256];
    getargs(&p, 5,",");
    fnbr = FindFreeFileNbr();
    fname = getFstring(argv[0]);
    if(argc>=3 && *argv[2])startsprite=getint(argv[2],1,64);
    if(argc==5)palette=getint(argv[4],0,1);
    if(strchr(fname, '.') == NULL) strcat(fname, ".SPR");
    if(!BasicFileOpen(fname, fnbr, FA_READ)) error("File not found");
	MMgetline(fnbr, (char *)buff);							    // get the input line
	while(buff[0]==39)MMgetline(fnbr, (char *)buff);
	sscanf((char *)buff, "%d,%d, %d", &width, &number, &height);
	if(height==0)height=width;
	if(width<1 || width>MAXSTRLEN || height<1 || number<1){		// (a line of the file is at most MAXSTRLEN characters)
		FileClose(fnbr);
		error("Invalid sprite file");
	}
	bnbr=startsprite;
	if(number+startsprite>MAXBLITBUF){
		FileClose(fnbr);
		error("Maximum of 64 sprites");
	}
	while(!FileEOF(fnbr) && bnbr<number+startsprite) {                                     // while waiting for the end of file
		if(newsprite){
			newsprite=0;
			if(blitbuff[bnbr].blitbuffptr!=NULL){				// a sprite there already is loaded into, as SPRITE READ reads into one
				char *why = NULL;
				if(blitbuff[bnbr].mymaster != -1) why = "Sprite % is a copy";
				else if(blitbuff[bnbr].master > 0) why = "Sprite % has copies";
				else if(!(blitbuff[bnbr].w==width && blitbuff[bnbr].h==height)){
					if(blitbuff[bnbr].active) why = "Sprite % is showing, at another size";
					else {
						FreeMemorySafe((void *)&blitbuff[bnbr].blitbuffptr);
						FreeMemorySafe((void *)&blitbuff[bnbr].blitstoreptr);
					}
				}
				if(why){
					FileClose(fnbr);
					error(why, bnbr);
				}
			}
	        if(blitbuff[bnbr].blitbuffptr == NULL) SpriteNew(bnbr, width, height);
	        else if(!blitbuff[bnbr].active) SpriteFields(bnbr, width, height);   // (the same size: its buffers used again; a
	                                                                // sprite showing keeps its place, and gets the new image)
	        qqq = (uint32_t *)blitbuff[bnbr].blitbuffptr;
	        lc = height;
		}
		while(lc--){
			MMgetline(fnbr, (char *)buff);									    // get the input line
			while(buff[0] == 39) MMgetline(fnbr, (char *)buff);
			if(strlen(buff) < width) mymemset(&buff[strlen(buff)], 32, width - strlen(buff));
			for(i = 0; i < width; i++){                                 // ' ' transparent, '0' black, 1-F the colours
				const char *d = strchr(digits, toupper(buff[i]));
				*qqq++ = (buff[i] == ' ' || d == NULL || (palette && d == digits)) ? 0 :
				         palette ? 0x0F000000 | colours[d - digits] : spr_colours[d - digits];
			}
		}
		bnbr++;
		newsprite=1;
	}
	FileClose(fnbr);
}
char getnextuncompressednibble(char **s, int reset){
    static int toggle=0;
    if(reset){
        toggle=reset-1;
        return 0;
    }
    if(!toggle){
        toggle ^=1;
        return **s & 0x0f;
    } else {
        toggle ^=1;
        char r=(**s & 0xf0)>>4;
        (*s)++;
        return r;
    }

}
static inline char getnextnibble(char **fc, int reset){
    static uint8_t available;
    static char out;
    if(reset){
        available=0;
    }
    if(available==0){
        available=**fc & 0xF; //number of identical pixels
        out=(**fc)>>4;
        (*fc)++;
    }
    if(!reset)available--;
    return out;
}
void copyframetoscreen(uint8_t *s,int xstart, int xend, int ystart, int yend, int odd){
    int c, i=(xend-xstart+1)*(yend-ystart+1)*3;
    char *buff=GetTempMemory(i);
    char *p=buff;
	if(odd){
		c=colours[(*s & 0xF0)>>4];
		*p++=c & 0xFF;
		*p++= (c>>8) & 0xff;
		*p++= c>>16;
		s++;
		i-=3;
	}
	while(i>0){
		c=colours[*s & 0xF];
		*p++=c & 0xFF;
		*p++= (c>>8) & 0xff;
		*p++= c>>16;
		if(i>3){
			c=colours[(*s & 0xF0)>>4];
			*p++=c & 0xFF;
			*p++= (c>>8) & 0xff;
			*p++= c>>16;
		}
		s++;
		i-=6;
	}
	DrawBuffer(xstart,ystart,xend,yend,buff,0);
}
void docompressed(char *fc,int x1, int y1, int w, int h, int8_t blank){
    if(blank==-1){
        char tobuff[(w+1)/2], *to;              // (an odd width has a last half byte)
        int ww=w;
        int xx1=x1;
        if(x1<0){
            ww+=x1;
            xx1=0;
        }
        if(x1+w>HRes){
            ww=HRes-x1;
        }
        getnextnibble(&fc,1); //reset the decoder
        for(int y=y1;y<y1+h;y++){
            to=tobuff;
            int otoggle=0;
            for(int x=x1;x<x1+w;x++){
                if(y<0 || y>=VRes){
                    getnextnibble(&fc,0);
                    continue;
                }
                if(x>=0 && x<HRes){
                    if(otoggle==0){
                        *to=getnextnibble(&fc,0);
                        otoggle ^=1;
                    } else {
                        *to|=(getnextnibble(&fc,0)<<4);
                        otoggle^=1;
                        to++;
                    }
                } else getnextnibble(&fc,0);
            }
            if(ww>0 && xx1<HRes)copyframetoscreen((unsigned char *)tobuff,xx1, xx1+ww-1, y, y, 0);
        }
    } else {
        char tobuff[(w+1)/2], *to;              // (an odd width has a last half byte)
        getnextnibble(&fc,1); //reset the decoder
        for(int y=y1;y<y1+h;y++){
            int x=x1;
            while(1){
                to=tobuff;
                int otoggle=0;
                char c;
                int ww=0;
                int xx=-1;
                while((c=getnextnibble(&fc,0))==blank){
                    x++;
                    if(x==x1+w)break;
                }
                if(x==x1+w)break; //nothing found so exit
                *to=c;
                otoggle ^=1;
                xx=x;
                x++;
                ww=1;
                if(xx!=x1+w-1){
                    while((c=getnextnibble(&fc,0))!=blank){
                        x++;
                        ww++;
                        if(otoggle==0){
                            *to=c;
                            otoggle ^=1;
                        } else {
                            *to|=(c<<4);
                            otoggle^=1;
                            to++;
                        }
                        if(x==x1+w)break;
                    }
                }
                x++;
                if(xx+ww>HRes){
                    ww=HRes-xx;
                }
                if(xx>=0 && ww>0 && y>=0 && y<VRes)copyframetoscreen((unsigned char *)tobuff,xx, xx+ww-1, y, y, 0);
                if(xx<0 && xx+ww>=0){
                    char *t=tobuff-(xx/2)-(xx&1);
                    ww+=xx;
                    if(ww>0)copyframetoscreen((unsigned char *)t,0, ww-1, y, y, xx&1);
                }
                if(x>=x1+w)break;
            }
        }
    }
}


int blitother(void){
    int x1, y1, w, h;
    char *p;
    if ((p = checkstring(cmdline, "FLASH"))) {                     // the PicoMite's: from an image slot (ImageSlots.c)
        BlitFlash(p);
        return 1;
    }
    if ((p = checkstring(cmdline, "FRAMEBUFFER"))) {               // the PicoMite's, between its framebuffers (Rgb121.c)
        Rgb121BlitFramebuffer(p);
        return 1;
    }
    if ((p = checkstring(cmdline, "RESIZE"))) {                    // the PicoMite's, scaling between them (Rgb121.c)
        Rgb121BlitResize(p);
        return 1;
    }
    int compressed = 0;                                             // BLIT COMPRESSED: BLIT MEMORY's code with bit 15 taken as
    if ((p = checkstring(cmdline, "COMPRESSED"))) compressed = 1; // set, so the data is always compressed (the PicoMite's,
    else p = checkstring(cmdline, "MEMORY");                       // whose width and height have no flag)
    if (p) {
        int8_t blank=-1;
        getargs(&p, 7, ",");
        if(argc<5)error("Syntax");
        char *from=(char *)GetPeekAddr(argv[0]);
        x1 = (int)getinteger(argv[2]);
        y1 = (int)getinteger(argv[4]);
        uint16_t *size=(uint16_t *)from;
        w=(size[0] & 0x7FFF);
        h=(size[1] & 0x7FFF);
        from+=4;
        if(argc==7)blank=getint(argv[6],-1,15);
        if(compressed || size[0] & 0x8000 || size[1] &  0x8000) {
            docompressed(from, x1, y1, w, h, blank);
        } else {
            if(blank==-1){
                 char *fc=from;
                 char tobuff[(w+1)/2], *to;              // (an odd width has a last half byte)
                 int ww=w;
                 int xx1=x1;
                 if(x1<0){
                     ww+=x1;
                     xx1=0;
                 }
                 if(x1+w>HRes){
                     ww=HRes-x1;
                 }
                 getnextuncompressednibble(&fc,1); //reset the decoder
                 for(int y=y1;y<y1+h;y++){
                     to=tobuff;
                     int otoggle=0;
                     for(int x=x1;x<x1+w;x++){
                         if(y<0 || y>=VRes){
                             getnextuncompressednibble(&fc,0);
                             continue;
                         }
                         if(x>=0 && x<HRes){
                             if(otoggle==0){
                                 *to=getnextuncompressednibble(&fc,0);
                                 otoggle ^=1;
                             } else {
                                 *to|=(getnextuncompressednibble(&fc,0)<<4);
                                 otoggle^=1;
                                 to++;
                             }
                         } else getnextuncompressednibble(&fc,0);
                     }
                     if(ww>0 && xx1<HRes)copyframetoscreen((unsigned char *)tobuff,xx1, xx1+ww-1, y, y, 0);
                 }
             } else {
                 char *fc=from;
                 char tobuff[(w+1)/2], *to;              // (an odd width has a last half byte)
                 getnextuncompressednibble(&fc,1); //reset the decoder
                 for(int y=y1;y<y1+h;y++){
                     int x=x1;
                     while(1){
                         to=tobuff;
                         int otoggle=0;
                         char c;
                         int ww=0;
                         int xx=-1;
                         while((c=getnextuncompressednibble(&fc,0))==blank){
                             x++;
                             if(x==x1+w)break;
                         }
                         if(x==x1+w)break; //nothing found so exit
                         *to=c;
                         otoggle ^=1;
                         xx=x;
                         x++;
                         ww=1;
                         if(xx!=x1+w-1){
                             while((c=getnextuncompressednibble(&fc,0))!=blank){
                                 x++;
                                 ww++;
                                 if(otoggle==0){
                                     *to=c;
                                     otoggle ^=1;
                                 } else {
                                     *to|=(c<<4);
                                     otoggle^=1;
                                     to++;
                                 }
                                 if(x==x1+w)break;
                             }
                         }
                         x++;
                         if(xx+ww>HRes){
                             ww=HRes-xx;
                         }
                         if(xx>=0 && ww>0 && y>=0 && y<VRes)copyframetoscreen((unsigned char *)tobuff,xx, xx+ww-1, y, y, 0);
                         if(xx<0 && xx+ww>=0){
                             char *t=tobuff-(xx/2)-(xx&1);
                             ww+=xx;
                             if(ww>0)copyframetoscreen((unsigned char *)t,0, ww-1, y, y, xx&1);
                         }
                         if(x>=x1+w)break;
                    }
                 }
             }
        }
        return 1;
    }
    return 0;
}

void cmd_blit(void) {
    int x1, y1, x2, y2, w, h, bnbr;
    int new=0;
    int maxW=PageTable[WritePage].xmax;
	int maxH=PageTable[WritePage].ymax;
    char *p, *q;
    if(blitother())return;
    if((p = checkstring(cmdline, "SHOW SAFE"))) {
		int layer;
		getargs(&p, 11,",");
		if(!(argc ==7 || argc==9 || argc==11)) error("Syntax");
		if(hideall)error("Sprites are hidden");
		if(*argv[0] == '#') argv[0]++;
		bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
		if(blitbuff[bnbr].blitbuffptr!=NULL){
			x1 = getint(argv[2],-blitbuff[bnbr].w+1,maxW-1);
			y1 = getint(argv[4],-blitbuff[bnbr].h+1,maxH-1);
			layer=getint(argv[6],0,MAXLAYER);
			if(argc>=9 && *argv[8])blitbuff[bnbr].rotation=getint(argv[8],0,7);   // (4-7 opaque: BlitShowBuff)
			else blitbuff[bnbr].rotation = 0;
			if(argc==11 && *argv[10]){
				new=getint(argv[10],0,1);
			}
			q=blitbuff[bnbr].blitbuffptr;
			w=blitbuff[bnbr].w;
			h=blitbuff[bnbr].h;
		    int cursorhidden=0;
		    if(cursoron){
				hidecursor(0);
				cursorhidden=1;
		    }
			if(blitbuff[bnbr].active){
				int old = blitbuff[bnbr].layer;                     // another layer was ignored (the PicoMite's too): within
				if(!new && layer != old && old != 0 && layer != 0){ // the list of layers 1-10 it keeps its place; to or from
					layer_in_use[old]--;                            // layer 0 (the other list) it goes on top
					layer_in_use[layer]++;
					blitbuff[bnbr].layer = layer;
				}
				if(new || (layer != old && (old == 0 || layer == 0))){
					hidesafe(bnbr);
					blitbuff[bnbr].layer=layer;
					layer_in_use[blitbuff[bnbr].layer]++;
					if(blitbuff[bnbr].layer==0) zeroLIFOadd(bnbr);
					else LIFOadd(bnbr);
					sprites_in_use++;
					BlitShowBuffer(bnbr, x1, y1, 1);
				} else {
					showsafe(bnbr,x1, y1);
				}
			} else {
				blitbuff[bnbr].layer=layer;
				layer_in_use[blitbuff[bnbr].layer]++;
				if(blitbuff[bnbr].layer==0) zeroLIFOadd(bnbr);
				else LIFOadd(bnbr);
				sprites_in_use++;
				BlitShowBuffer(bnbr, x1, y1, 1);
			}
		    if(cursorhidden)showcursor(0, xcursor,ycursor);
			ProcessCollisions(bnbr);
	        if(sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error("sprite internal error");
		} else error("Buffer not in use");
    } else if((p = checkstring(cmdline, "SHOW"))) {
		int layer;
		getargs(&p, 9,",");
		if(!(argc ==7 || argc==9)) error("Syntax");
		if(hideall)error("Sprites are hidden");
		if(*argv[0] == '#') argv[0]++;
		bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
		if(blitbuff[bnbr].blitbuffptr!=NULL){
			x1 = getint(argv[2],-blitbuff[bnbr].w+1,maxW-1);
			y1 = getint(argv[4],-blitbuff[bnbr].h+1,maxH-1);
			layer=getint(argv[6],0,MAXLAYER);
			if(argc==9)blitbuff[bnbr].rotation=getint(argv[8],0,7);
			else blitbuff[bnbr].rotation = 0;
			q=blitbuff[bnbr].blitbuffptr;
			w=blitbuff[bnbr].w;
			h=blitbuff[bnbr].h;
			if(blitbuff[bnbr].active){
				layer_in_use[blitbuff[bnbr].layer]--;
				if(blitbuff[bnbr].layer==0)zeroLIFOremove(bnbr);
				else LIFOremove(bnbr);
				sprites_in_use--;
			}
			blitbuff[bnbr].layer=layer;
			layer_in_use[blitbuff[bnbr].layer]++;
			if(blitbuff[bnbr].layer==0) zeroLIFOadd(bnbr);
			else LIFOadd(bnbr);
			sprites_in_use++;
		    int cursorhidden=0;
		    if(cursoron){
				hidecursor(0);
				cursorhidden=1;
		    }
			BlitShowBuffer(bnbr, x1, y1, 1);
		    if(cursorhidden)showcursor(0, xcursor,ycursor);
			ProcessCollisions(bnbr);
	        if(sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error("sprite internal error");
		} else error("Buffer not in use");
    } else if((p = checkstring(cmdline, "HIDE ALL"))) {
		if(hideall)error("Sprites are hidden");
    	int i;
	    int cursorhidden=0;
	    if(cursoron){
			hidecursor(0);
			cursorhidden=1;
	    }
        for(i=LIFOpointer-1; i>= 0; i--) {
            blithide(LIFO[i],0);
        }
        for(i=zeroLIFOpointer-1; i>= 0; i--){
            blithide(zeroLIFO[i],0);
        }
	    if(cursorhidden)showcursor(0, xcursor,ycursor);
        hideall=1;
    } else if((p = checkstring(cmdline, "RESTORE"))) {
		if(!hideall)error("Sprites are not hidden");
    	int i;
	    int cursorhidden=0;
	    if(cursoron){
			hidecursor(0);
			cursorhidden=1;
	    }
        for(i=0; i< zeroLIFOpointer; i++){
        	BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for(i=0; i< LIFOpointer; i++){
        	    if(blitbuff[LIFO[i]].next_x != 10000){
                blitbuff[LIFO[i]].x=blitbuff[LIFO[i]].next_x;
                blitbuff[LIFO[i]].next_x = 10000;
            }
            if(blitbuff[LIFO[i]].next_y != 10000){
                blitbuff[LIFO[i]].y=blitbuff[LIFO[i]].next_y;
                blitbuff[LIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
	    if(cursorhidden)showcursor(0, xcursor,ycursor);
        hideall=0;
        ProcessCollisions(0);
    } else if((p = checkstring(cmdline, "HIDE SAFE"))) {
        getargs(&p, 1,",");
        if(sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error("sprite internal error");
        if(argc !=1) error("Syntax");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
		if(hideall)error("Sprites are hidden");
        if(blitbuff[bnbr].blitbuffptr!=NULL){
            if(blitbuff[bnbr].active){
				int cursorhidden=0;
				if(cursoron){
					hidecursor(0);
					cursorhidden=1;
				}
				hidesafe(bnbr);
				if(cursorhidden)showcursor(0, xcursor,ycursor);
				if(sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error("sprite internal error");
            } else error("Not Showing");
        } else error("Buffer not in use");
    } else if((p = checkstring(cmdline, "HIDE"))) {
        getargs(&p, 1,",");
        if(argc !=1) error("Syntax");
		if(hideall)error("Sprites are hidden");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
        if(blitbuff[bnbr].blitbuffptr!=NULL){
            if(blitbuff[bnbr].active){
                sprites_in_use--;
        	    int cursorhidden=0;
        	    if(cursoron){
        			hidecursor(0);
        			cursorhidden=1;
        	    }
                blithide(bnbr, 0);
        	    if(cursorhidden)showcursor(0, xcursor,ycursor);
                layer_in_use[blitbuff[bnbr].layer]--;
                blitbuff[bnbr].x=10000;
                blitbuff[bnbr].y=10000;
                if(blitbuff[bnbr].layer==0)zeroLIFOremove(bnbr);
                else LIFOremove(bnbr);
                blitbuff[bnbr].layer=-1;
                blitbuff[bnbr].next_x = 10000;
                blitbuff[bnbr].next_y = 10000;
        		blitbuff[bnbr].lastcollisions=0;
                blitbuff[bnbr].edges=0;
            } else error("Not Showing");
        } else error("Buffer not in use");
        if(sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error("sprite internal error");
//
    } else if((p = checkstring(cmdline, "SWAP"))) {
        int rbnbr;
        getargs(&p, 5,",");
        if(argc < 3) error("Syntax");
		if(hideall)error("Sprites are hidden");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
        if(*argv[2] == '#') argv[2]++;
        rbnbr = getint(argv[2],1,MAXBLITBUF-1);									// get the number
        if(blitbuff[bnbr].blitbuffptr==NULL || blitbuff[bnbr].active==false) error("Original buffer not displayed");
        if(!blitbuff[bnbr].active)error("Original buffer not displayed");
        if(blitbuff[rbnbr].active) error("New buffer already displayed");
        if(!(blitbuff[rbnbr].w ==blitbuff[bnbr].w && blitbuff[rbnbr].h ==blitbuff[bnbr].h)) error("Size mismatch");
// copy the relevant data
        char *store=blitbuff[rbnbr].blitstoreptr;                      // the background stores are exchanged: the new sprite
        blitbuff[rbnbr].blitstoreptr=blitbuff[bnbr].blitstoreptr;    // takes the one under the old (the old kept it too, so
        blitbuff[bnbr].blitstoreptr=store;                           // closing both freed it twice; the new one's leaked)
        blitbuff[rbnbr].x=blitbuff[bnbr].x;
        blitbuff[rbnbr].y=blitbuff[bnbr].y;
        blitbuff[rbnbr].layer=blitbuff[bnbr].layer;
        blitbuff[rbnbr].lastcollisions=blitbuff[bnbr].lastcollisions;
        if(blitbuff[rbnbr].layer==0)zeroLIFOswap(bnbr,rbnbr);
        else LIFOswap(bnbr,rbnbr);
// "Hide" the old sprite
        blitbuff[bnbr].x=10000;
        blitbuff[bnbr].y=10000;
        blitbuff[bnbr].layer=-1;
        blitbuff[bnbr].next_x = 10000;
        blitbuff[bnbr].next_y = 10000;
        blitbuff[bnbr].active=0;
        blitbuff[bnbr].lastcollisions=0;
        if(argc==5)blitbuff[rbnbr].rotation=getint(argv[4],0,7);
        else blitbuff[rbnbr].rotation = 0;
	    int cursorhidden=0;
	    if(cursoron){
			hidecursor(0);
			cursorhidden=1;
	    }
        BlitShowBuffer(rbnbr, blitbuff[rbnbr].x, blitbuff[rbnbr].y, 2);   // (the whole rectangle, over the old one's
                                                                    // background: none of the old image stays)
	    if(cursorhidden)showcursor(0, xcursor,ycursor);
        if(sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error("sprite internal error");

    } else if((p = checkstring(cmdline, "READ"))) {
        getargs(&p, 11,",");
        if(!(argc == 9 || argc == 11)) error("Syntax");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
        x1 = getinteger(argv[2]);
        y1 = getinteger(argv[4]);
        w = getinteger(argv[6]);
        h = getinteger(argv[8]);
        if(w < 1 || h < 1) return;
        if(blitbuff[bnbr].blitbuffptr==NULL) SpriteNew(bnbr, w, h);
        else {
            if(blitbuff[bnbr].mymaster != -1) error("Can't read into a copy", bnbr);
            if(blitbuff[bnbr].master >0) error("Copies exist", bnbr);
            if(!(blitbuff[bnbr].w==w && blitbuff[bnbr].h==h))error("Existing buffer is incorrect size");
        }
        q = GetTempMemory(w * h * 4);                                   // the page's pixels, then as colour words
        if(argc==11){
        	if(checkstring(argv[10], "FRAMEBUFFER")){
        		if(PageTable[WPN].address==NULL)error("Framebuffer not created");
        		ReadPage=WPN;
        	} else {
				ReadPage=getint(argv[10],0,LastPage);
        	}
        }
	    int cursorhidden=0;
	    if(cursoron){
			hidecursor(0);
			cursorhidden=1;
	    }
        ReadBufferFast(x1,y1,x1+w-1,y1+h-1,q);
	    if(cursorhidden)showcursor(0, xcursor,ycursor);
        ReadPage=WritePage;
        {
            uint8_t i4[16], i5[32], i6[64];
            uint32_t *img = (uint32_t *)blitbuff[bnbr].blitbuffptr;
            SpriteLevels(i4, BIT_4, 16, 4);
            SpriteLevels(i5, BIT_5, 32, 3);
            SpriteLevels(i6, BIT_6, 64, 2);
            for(int k = 0; k < w * h; k++)
                img[k] = SpriteWord(VideoColour == 32 ? ((uint32_t *)q)[k] : (VideoColour == 12 || VideoColour == 16) ?
                                    ((uint16_t *)q)[k] : ((uint8_t *)q)[k], i4, i5, i6);
        }
    } else if((p = checkstring(cmdline, "COPY"))) {
        int cpy,nbr,c1,n1;
        getargs(&p, 5,",");
        if(argc !=5) error("Syntax");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
        if(blitbuff[bnbr].blitbuffptr!=NULL){
            if(*argv[2] == '#') argv[2]++;
            c1 = cpy = getint(argv[2], 1, MAXBLITBUF-1);
            n1 = nbr = getint(argv[4], 1, MAXBLITBUF-cpy);

            while(n1) {
                if(blitbuff[c1].blitbuffptr!=NULL)error("Buffer already in use %",c1);
                if(blitbuff[bnbr].master==-1)error("Can't copy a copy");;
                n1--;
                c1++;
            }
            while(nbr) {
                blitbuff[cpy].blitbuffptr=blitbuff[bnbr].blitbuffptr;
                blitbuff[cpy].w=blitbuff[bnbr].w;
                blitbuff[cpy].h=blitbuff[bnbr].h;
                blitbuff[cpy].blitstoreptr = GetMemory(blitbuff[cpy].w * blitbuff[cpy].h * SPRITEBG);
                blitbuff[cpy].bc=blitbuff[bnbr].bc;
                blitbuff[cpy].x=10000;
                blitbuff[cpy].y=10000;
                blitbuff[cpy].next_x = 10000;
                blitbuff[cpy].next_y = 10000;
                blitbuff[cpy].layer=-1;
                blitbuff[cpy].mymaster=bnbr;
                blitbuff[cpy].master=-1;
                blitbuff[cpy].edges=0;
                blitbuff[bnbr].master++;                       // (the number of copies)
        		blitbuff[bnbr].lastcollisions=0;
    			blitbuff[cpy].active=false;
                nbr--;
                cpy++;
            }
        } else error("Buffer not in use");

    } else if((p = checkstring(cmdline, "LOADARRAY"))) {
        loadarray(p);

    } else if((p = checkstring(cmdline, "LOADPNG"))) {
        loadpng(p);

    } else if((p = checkstring(cmdline, "LOADBMP"))) {
        loadbmp(p);

    } else if((p = checkstring(cmdline, "LOAD"))) {
        if(SpriteCommand()) loadsprite(p);                          // SPRITE LOAD: a .SPR file
        else loadbmp(p);                                            // BLIT LOAD: the PicoMite's other name for LOADBMP

    } else if((p = checkstring(cmdline, "SET TRANSPARENT"))) {   // the PicoMite's: one of its 16 colours (0 black)
        SpriteKey = colours[getint(p, 0, 15)];
    } else if((p = checkstring(cmdline, "STATIC CLEAR"))) {
        closeallstobjects();
    } else if((p = checkstring(cmdline, "STATIC"))) {            // SPRITE STATIC #n, x, y, w, h | SPRITE STATIC #n, OFF
        getargs(&p, 9, ",");
        if(*argv[0] == '#') argv[0]++;
        int st = getint(argv[0], 1, MAXSTOBJECTS);
        if(argc == 3 && checkstring(argv[2], "OFF")) memset(&stobjects[st], 0, sizeof(stobjects[st]));
        else if(argc == 9){
            stobjects[st].x = getint(argv[2], -32768, 32767);
            stobjects[st].y = getint(argv[4], -32768, 32767);
            stobjects[st].w = getint(argv[6], 1, 32767);
            stobjects[st].h = getint(argv[8], 1, 32767);
            stobjects[st].active = 1;
        } else error("Syntax");
    } else if((p = checkstring(cmdline, "STINTERRUPT"))) {
        getargs(&p, 1, ",");
        STCollisionInterrupt = GetIntAddress(argv[0]);
        IntSignal();
    } else if((p = checkstring(cmdline, "NOSTINTERRUPT"))) {
        STCollisionInterrupt = NULL;
    } else if((p = checkstring(cmdline, "INTERRUPT"))) {
        getargs(&p, 1,",");
        COLLISIONInterrupt = GetIntAddress(argv[0]);					// get the interrupt location
        IntSignal();
        return;

    } else if((p = checkstring(cmdline, "NOINTERRUPT"))) {
        COLLISIONInterrupt = NULL;					// get the interrupt location
        return;

    } else if((p = checkstring(cmdline, "CLOSE ALL"))) {
        closeallsprites();
        
    } else if((p = checkstring(cmdline, "CLOSE"))) {
        getargs(&p, 1,",");
		if(hideall)error("Sprites are hidden");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);
        if(blitbuff[bnbr].master>0) error("Copies still open");
        if(blitbuff[bnbr].blitbuffptr!=NULL){
     	   if(blitbuff[bnbr].active){
     		   int cursorhidden=0;
     		   if(cursoron){
     			   hidecursor(0);
     			   cursorhidden=1;
     		   }
     		   blithide(bnbr, 1);
     		   if(cursorhidden)showcursor(0, xcursor,ycursor);
     		   if(blitbuff[bnbr].layer==0)zeroLIFOremove(bnbr);
     		   else LIFOremove(bnbr);
     		   layer_in_use[blitbuff[bnbr].layer]--;
     		   sprites_in_use--;
     	   }
           if(blitbuff[bnbr].mymaster==-1)FreeMemorySafe((void *)&blitbuff[bnbr].blitbuffptr);
           else blitbuff[blitbuff[bnbr].mymaster].master--;
   		    FreeMemorySafe((void *)&blitbuff[bnbr].blitstoreptr);
   	        blitbuff[bnbr].blitbuffptr = NULL;
   	        blitbuff[bnbr].blitstoreptr = NULL;
   	        blitbuff[bnbr].master=-1;
   	        blitbuff[bnbr].mymaster=-1;
   	        blitbuff[bnbr].x=10000;
   	        blitbuff[bnbr].y=10000;
   	        blitbuff[bnbr].w=0;
   	        blitbuff[bnbr].h=0;
   	        blitbuff[bnbr].next_x = 10000;
   	        blitbuff[bnbr].next_y = 10000;
   	        blitbuff[bnbr].bc=0;
   	        blitbuff[bnbr].layer=-1;
   			blitbuff[bnbr].active=false;
   	        blitbuff[bnbr].edges=0;
        } else error("Buffer not in use");
        if(sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error("sprite internal error");
    } else if((p = checkstring(cmdline, "TRANSPARENCY"))) {
    	int x,y, trans;
        getargs(&p, 3,",");
        if(!(VideoColour==12))error("Invalid for this display mode");
        if(argc !=3) error("Syntax");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
        trans=getint(argv[2],1,15);
        if(blitbuff[bnbr].blitbuffptr!=NULL){
            w=blitbuff[bnbr].w;
            h=blitbuff[bnbr].h;
            uint32_t *q = (uint32_t *)blitbuff[bnbr].blitbuffptr;   // every pixel not transparent gets it
            for(y = 0; y < h; y++)
            	for(x = 0; x < w; x++, q++)
            		if(*q & 0x0F000000) *q = (*q & 0xF0FFFFFF) | (trans << 24);
        } else error("Buffer not in use");

    } else if((p = checkstring(cmdline, "NEXT"))) {
        getargs(&p, 5,",");
        if(!(argc ==5)) error("Syntax");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
        blitbuff[bnbr].next_x = getint(argv[2],-blitbuff[bnbr].w+1,maxW-1);
        blitbuff[bnbr].next_y = getint(argv[4],-blitbuff[bnbr].h+1,maxH-1);
//
    } else if((p = checkstring(cmdline, "WRITE"))) {
    	int mode=4;
        getargs(&p, 7,",");
        if(!(argc ==5 || argc==7)) error("Syntax");
        if(*argv[0] == '#') argv[0]++;
        bnbr = getint(argv[0],1,MAXBLITBUF-1);									// get the number
        if(blitbuff[bnbr].blitbuffptr!=NULL){
            x1 = getint(argv[2], -blitbuff[bnbr].w+1, maxW);
            y1 = getint(argv[4], -blitbuff[bnbr].h+1, maxH);
            int keep = blitbuff[bnbr].rotation, r = argc == 7 ? getint(argv[6], 0, 7) : SpriteCommand() ? 4 : 0;
            if((r & 4) == 0) mode |= 8;                              // (0-3 every pixel, 4-7 not the transparent ones;
                                                                    // the PicoMite's defaults: BLIT 0, SPRITE 4)
            blitbuff[bnbr].rotation = r & 3;                         // for this draw only: a sprite showing keeps its own
            q=blitbuff[bnbr].blitbuffptr;
            w=blitbuff[bnbr].w;
            h=blitbuff[bnbr].h;
  		    int cursorhidden=0;
  		    if(cursoron){
  			   hidecursor(0);
  			   cursorhidden=1;
  		    }
            BlitShowBuffer(bnbr, x1, y1, mode);
            blitbuff[bnbr].rotation = keep;
  		    if(cursorhidden)showcursor(0, xcursor,ycursor);
        } else error("Buffer not in use");
    } else if((p = checkstring(cmdline, "MOVE"))) {
		if(hideall)error("Sprites are hidden");
        int i;
		int cursorhidden=0;
		if(cursoron){
		   hidecursor(0);
		   cursorhidden=1;
		}
        for(i=LIFOpointer-1; i>= 0; i--) blithide(LIFO[i],0);
        for(i=zeroLIFOpointer-1; i>= 0; i--)blithide(zeroLIFO[i],0);
//
        for(i=0; i< zeroLIFOpointer; i++){
            if(blitbuff[zeroLIFO[i]].next_x != 10000){
                blitbuff[zeroLIFO[i]].x=blitbuff[zeroLIFO[i]].next_x;
                blitbuff[zeroLIFO[i]].next_x = 10000;
            }
            if(blitbuff[zeroLIFO[i]].next_y != 10000){
                blitbuff[zeroLIFO[i]].y=blitbuff[zeroLIFO[i]].next_y;
                blitbuff[zeroLIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for(i=0; i< LIFOpointer; i++){
            if(blitbuff[LIFO[i]].next_x != 10000){
                blitbuff[LIFO[i]].x=blitbuff[LIFO[i]].next_x;
                blitbuff[LIFO[i]].next_x = 10000;
            }
            if(blitbuff[LIFO[i]].next_y != 10000){
                blitbuff[LIFO[i]].y=blitbuff[LIFO[i]].next_y;
                blitbuff[LIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);

        }
		if(cursorhidden)showcursor(0, xcursor,ycursor);
        ProcessCollisions(0);
    } else if((p = checkstring(cmdline, "SCROLLR"))) {
        int i,xmin,xmax,ymin,ymax, blank=-1, sx=0, sy=0, b1, b2, xs=10000, ys=10000;
    	int flip=0;
        char *current3=NULL, *current4=NULL;
        getargs(&p, 13,",");
		if(hideall)error("Sprites are hidden");
        if(!(argc ==11 || (argc==13))) error("Syntax");
        x1 = getint(argv[0],0,maxW-2);
        y1 = getint(argv[2],0,maxH-2);
        w = getint(argv[4],1,maxW-x1);
        h = getint(argv[6],1,maxH-y1);
        sx = getint(argv[8],1-w, w-1);
        sy = getint(argv[10],1-h, h-1);
        if(sx>=0){ //convert coordinates for use with BLIT
            x2=x1+sx;
            w-=sx;
        } else {
            x2=x1;
            x1-=sx; //sx is negative so this subtracts it
            w+=sx;
        }
        if(sy<0){ //convert coordinates for use with BLIT
            y2=y1-sy;
            h+=sy;
        } else {
            y2=y1;
            y1+=sy; 
            h-=sy;
        }
        if(argc==13)blank=getColour(argv[12], 1);
        xmin=min(x1,x2);
        xmax=max(x1,x2)+w-1;
        ymin=min(y1,y2);
        ymax=max(y1,y2)+h-1;
        if(w < 1 || h < 1) return;
        if(x1 < 0) { x2 -= x1; w += x1; x1 = 0; }
        if(x2 < 0) { x1 -= x2; w += x2; x2 = 0; }
        if(y1 < 0) { y2 -= y1; h += y1; y1 = 0; }
        if(y2 < 0) { y1 -= y2; h += y2; y2 = 0; }
        if(x1 + w > maxW) w = maxW - x1;
        if(x2 + w > maxW) w = maxW - x2;
        if(y1 + h > maxH) h = maxH - y1;
        if(y2 + h > maxH) h = maxH - y2;
        if(w < 1 || h < 1 || x1 < 0 || x1 + w > maxW || x2 < 0 || x2 + w > maxW || y1 < 0 || y1 + h > maxH || y2 < 0 || y2 + h > maxH) return;

        b1=RoundUptoInt(abs(sx))*ymax;
        b2=RoundUptoInt(xmax)*abs(sy);
		if(blank<0){
            if(x1!=x2)current3=GetMemory(b1*PageTable[WritePage].nbytes);
            if(y1!=y2)current4=GetMemory(b2*PageTable[WritePage].nbytes);
        }
		int cursorhidden=0;
		if(cursoron){
		   hidecursor(0);
		   cursorhidden=1;
		}

        for(i=LIFOpointer-1; i>= 0; i--) {
            blithide(LIFO[i],0);

        }
            
        for(i=zeroLIFOpointer-1; i>= 0; i--){
            xs=blitbuff[zeroLIFO[i]].x + (blitbuff[zeroLIFO[i]].w >> 1);
            ys=blitbuff[zeroLIFO[i]].y + (blitbuff[zeroLIFO[i]].h >> 1);
            blithide(zeroLIFO[i],0);
            if((xs <= xmax) && (xs  >= xmin) && (ys <= ymax) && (ys >= ymin)){
                xs+=(x2-x1);
                if(xs>=xmax)xs -=  (xmax-xmin+1);
                if(xs < xmin)xs += (xmax-xmin+1);
                blitbuff[zeroLIFO[i]].x=xs-(blitbuff[zeroLIFO[i]].w >> 1);
                ys+=(y2-y1);
                if(ys>=ymax)ys -=  (ymax-ymin+1);
                if(ys < ymin)ys += (ymax-ymin+1);
                blitbuff[zeroLIFO[i]].y=ys-(blitbuff[zeroLIFO[i]].h >> 1);
            }
        }
        StaticsMove(x2 - x1, y2 - y1, xmin, ymin, xmax, ymax, 1);
        if(blank==-1){
            if(x1!=x2){
                if(sx>0){
                    ReadBufferFast(xmax-sx+1, ymin, xmax, ymax, current3) ;
                } else {
                    ReadBufferFast(xmin, ymin, xmin-sx-1, ymax,  current3) ;
                }
            }
            if(y1!=y2){
                if(sy>0){
                    ReadBufferFast(xmin, ymin, xmax, ymin-1+sy, current4);
                } else {
                    ReadBufferFast(xmin, ymax+sy+1, xmax, ymax, current4);
                }
            }
        }
    	int copymode=PageTable[ReadPage].expand | (PageTable[WritePage].expand<<1);
    	switch(copymode){
    	case 0:
    		MoveBufferNormal( x1, y1, x2, y2, w, h, flip);
    		break;
    	case 1:
        	MoveBufferContract( x1, y1, x2, y2, w, h, flip);
    		break;
    	case 2:
        	MoveBufferExpand( x1, y1, x2, y2, w, h, flip);
    		break;
    	case 3:
        	MoveBufferDup( x1, y1, x2, y2, w, h, flip);
    	}
        if(x1!=x2){
            if(blank!=-1){
                if(sx>0){
                    DrawRectangle(xmin, ymin, xmin+sx-1, ymax,  blank) ;
                } else {
                    DrawRectangle(xmax+sx+1, ymin, xmax, ymax, blank) ;
                }
            } else {
                if(sx>0){
                    DrawBufferFast(xmin, ymin, xmin+sx-1, ymax,  current3) ;
                } else {
                    DrawBufferFast(xmax+sx+1, ymin, xmax, ymax, current3) ;
                }
            }
        }
        if(y1!=y2){
            if(blank!=-1){
                if(sy>0){
                    DrawRectangle(xmin, ymax-sy+1, xmax, ymax, blank);
                } else {
                    DrawRectangle(xmin, ymin, xmax, ymin-1-sy, blank);
                }
            } else {
                if(sy>0){
                    DrawBufferFast(xmin, ymax-sy+1, xmax, ymax, current4);
                } else {
                    DrawBufferFast(xmin, ymin, xmax, ymin-1-sy, current4);
                }
            }
        }
		FreeMemorySafe((void*)&current3);
		FreeMemorySafe((void*)&current4);
        for(i=0; i< zeroLIFOpointer; i++){
        	BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for(i=0; i< LIFOpointer; i++){
        	    if(blitbuff[LIFO[i]].next_x != 10000){
                blitbuff[LIFO[i]].x=blitbuff[LIFO[i]].next_x;
                blitbuff[LIFO[i]].next_x = 10000;
            }
            if(blitbuff[LIFO[i]].next_y != 10000){
                blitbuff[LIFO[i]].y=blitbuff[LIFO[i]].next_y;
                blitbuff[LIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
		if(cursorhidden)showcursor(0, xcursor,ycursor);
        ProcessCollisions(0);
    } else if((p = checkstring(cmdline, "SCROLL"))) {
        int i, n, m=0, blank=-2,x,y;
        char *current=NULL;
        getargs(&p, 5,",");
		if(hideall)error("Sprites are hidden");
        x = getint(argv[0],-maxW/2-1, maxW);
        y = getint(argv[2],-maxH/2-1, maxH);
        if(argc==5)blank=getColour(argv[4], 1);
        if(!(x==0 && y==0)){
    		int cursorhidden=0;
    		if(cursoron){
    		   hidecursor(0);
    		   cursorhidden=1;
    		}
        	m=((maxW*(y>0?y:-y))*PageTable[WritePage].nbytes);
        	n=((maxH*(x>0?x:-x))*PageTable[WritePage].nbytes);
        	if(n>m)m=n;
    		if(blank==-2)current=GetMemory(m);
            for(i=LIFOpointer-1; i>= 0; i--) blithide(LIFO[i],0);
            for(i=zeroLIFOpointer-1; i>= 0; i--){
                    int xs=blitbuff[zeroLIFO[i]].x+ (blitbuff[zeroLIFO[i]].w >> 1);
                    int ys=blitbuff[zeroLIFO[i]].y+ (blitbuff[zeroLIFO[i]].h >> 1);
                    blithide(zeroLIFO[i],0);
                    xs+=x;
                    if(xs>=maxW)xs-=maxW;
                    if(xs < 0)xs+=maxW;
                    blitbuff[zeroLIFO[i]].x=xs-(blitbuff[zeroLIFO[i]].w >> 1);
                    ys-=y;
                    if(ys>=maxH)ys-=maxH;
                    if(ys < 0)ys+=maxH;
                    blitbuff[zeroLIFO[i]].y=ys-(blitbuff[zeroLIFO[i]].h >> 1);
            }
            StaticsMove(x, -y, 0, 0, maxW - 1, maxH - 1, 0);
            if(x>0){
                if(blank==-2)ReadBufferFast(maxW-x,0,maxW-1,maxH-1,current);
                ScrollBufferH(x);
                if(blank==-2)DrawBufferFast(0,0,x-1,maxH-1,current);
                else if(blank!=-1)DrawRectangle(0, 0, x-1, maxH-1,  blank) ;
            } else if(x<0){
                x=-x;
                if(blank==-2)ReadBufferFast(0,0,x-1,maxH-1,current);
                ScrollBufferH(-x);
                if(blank==-2)DrawBufferFast(maxW-x,0,maxW-1,maxH-1, current);
                else if(blank!=-1)DrawRectangle(maxW-x,0,maxW-1,maxH-1, blank);
    		}
    		if(y>0){
    	        if(blank==-2)ReadBufferFast(0,0,maxW-1,y-1,current);
    	        ScrollBufferV(y, 0);
    	        if(blank==-2)DrawBufferFast(0,maxH-y,maxW-1,maxH-1,current);
    	        else if(blank!=-1)DrawRectangle(0,maxH-y,maxW-1,maxH-1, blank) ;
    	    } else if(y<0){
    	        y=-y;
    	        if(blank==-2)ReadBufferFast(0,maxH-y,maxW-1,maxH-1,current);
    	        ScrollBufferV(-y, 0 );
    	        if(blank==-2)DrawBufferFast(0,0,maxW-1,y-1,current);
    	        else if(blank!=-1)DrawRectangle(0,0,maxW-1,y-1, blank) ;
    	    }
			for(i=0; i< zeroLIFOpointer; i++){
            	BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
             }
            for(i=0; i< LIFOpointer; i++){
                if(blitbuff[LIFO[i]].next_x != 10000){
                    blitbuff[LIFO[i]].x=blitbuff[LIFO[i]].next_x;
                    blitbuff[LIFO[i]].next_x = 10000;
                }
                if(blitbuff[LIFO[i]].next_y != 10000){
                    blitbuff[LIFO[i]].y=blitbuff[LIFO[i]].next_y;
                    blitbuff[LIFO[i]].next_y = 10000;
                }

                BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
             }
    		if(cursorhidden)showcursor(0, xcursor,ycursor);
            ProcessCollisions(0);
            if(current)FreeMemory(current);
        }
     } else {
    	int flip=0;
        getargs(&cmdline,15,",");
        if(argc < 11) error("Syntax");
        x1 = getinteger(argv[0]);
        y1 = getinteger(argv[2]);
        x2 = getinteger(argv[4]);
        y2 = getinteger(argv[6]);
        w = getinteger(argv[8]);
        h = getinteger(argv[10]);
        if(argc>=13 && *argv[12]){
        	if(checkstring(argv[12], "FRAMEBUFFER")){
        		if(PageTable[WPN].address==NULL)error("Framebuffer not created");
        		ReadPage=WPN;
        	} else {
				ReadPage=getint(argv[12],0,LastPage);
        	}
        }
        int readlimx=PageTable[ReadPage].xmax;
        int readlimy=PageTable[ReadPage].ymax;
        if(argc==15)flip=getint(argv[14],0,7);
        if(w < 1 || h < 1) {
            ReadPage=WritePage;
        	return;
        }
        if(x1 < 0) {
        	x2 -= x1;
        	w += x1;
        	x1 = 0;
        }
        if(x2 < 0) {
        	if(!(flip & 1))x1 -= x2;
        	w += x2;
        	x2 = 0;
        }
        if(y1 < 0) {
        	y2 -= y1;
        	h += y1;
        	y1 = 0;
        }
        if(y2 < 0) {
        	if(!(flip & 2))y1 -= y2;
        	h += y2;
        	y2 = 0;
        }
        if(x1 + w > readlimx) {
        	w = readlimx - x1;
        }
        if(x2 + w > maxW) {
        	if(flip & 1) x1 += x2 + w - maxW;
        	w = maxW - x2;
        }
        if(y1 + h > readlimy) {
        	h = readlimy - y1;
        }
        if(y2 + h > maxH) {
        	if(flip & 2) y1 += y2 + h - maxH;
        	h = maxH - y2;
        }
        if(w < 1 || h < 1 || x1 < 0 || x1 + w > readlimx || x2 < 0 || x2 + w > maxW || y1 < 0 || y1 + h > readlimy || y2 < 0 || y2 + h > maxH) {
            ReadPage=WritePage;
        	return;
        }
        if(VideoColour==4){                                         // 4-bit pages (Rgb121.c)
            Rgb121Blit(x1, y1, x2, y2, w, h, flip);
            ReadPage=WritePage;
            return;
        }
        if(VideoColour==1){                                         // 1-bit pages (Mono.c)
            MonoBlit(x1, y1, x2, y2, w, h, flip);
            ReadPage=WritePage;
            return;
        }
        if(VideoColour==8 && (x1 & 3) == 0 && (x2 & 3) == 0 && (w & 3)==0){
			docopy=zcopy;
        } else if(VideoColour !=8 && (x1 & 1) == 0 && (x2 & 1) == 0 && (w & 1)==0){
			docopy=zcopy;
        } else if(VideoColour==32) docopy=zcopy;
    	int copymode=PageTable[ReadPage].expand | (PageTable[WritePage].expand<<1);
    	switch(copymode){
    	case 0:
//        	MMPrintString("MoveBufferNormal\r\n");
    		MoveBufferNormal( x1, y1, x2, y2, w, h, flip);
    		break;
    	case 1:
//        	MMPrintString("MoveBufferContract\r\n");
        	MoveBufferContract( x1, y1, x2, y2, w, h, flip);
    		break;
    	case 2:
//        	MMPrintString("MoveBufferExpand\r\n");
        	MoveBufferExpand( x1, y1, x2, y2, w, h, flip);

    		break;
    	case 3:
//        	MMPrintString("MoveBufferDup\r\n");
        	MoveBufferDup( x1, y1, x2, y2, w, h, flip);
    	}
        ReadPage=WritePage;
        docopy=mycopy;
    }
}

