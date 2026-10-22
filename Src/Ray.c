/***********************************************************************************************************************
PicoMite MMBasic

Ray.c

<COPYRIGHT HOLDERS>  Geoff Graham, Peter Mather
Copyright (c) 2021, <COPYRIGHT HOLDERS> All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
3.	The name MMBasic be used when referring to the interpreter in any documentation and promotional material and the original copyright message be displayed
    on the console at startup (additional copyright messages may be added).
4.	All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed
    by the <copyright holder>.
5.	Neither the name of the <copyright holder> nor the names of its contributors may be used to endorse or promote products derived from this software
    without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDERS> AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDERS> BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

************************************************************************************************************************/
/***********************************************************************************************************************
RAY and RAY(: the PicoMite's raycaster (its graphics/Raycaster.c at bb833ed) on the CMM2, in every colour depth and in
full colour (docs/Ray_Plan.md, 21 Oct 2026). A Wolfenstein-style first-person view of a grid of wall types, with
patterned floor and ceiling, sliding doors, billboard sprites, collision and a minimap; the commands, functions,
defaults, limits and arithmetic are the PicoMite's, and its function names are kept for merging.

  RAY MAP w, h, map%() | map$()          RAY DEFINE type, fg, bg, pattern [, door]      RAY CAMERA x, y, angle [, fov]
  RAY COLOUR floor, ceiling [, floor_bg, ceiling_bg, floor_pat, ceiling_pat]            RAY MOVE speed [, strafe]
  RAY TURN degrees    RAY CELL x, y, value    RAY CAST angle    RAY SPRITE id, n, x, y | REMOVE id | CLEAR
  RAY DOOR x, y, offset | CLOSE x, y | CLEAR    RAY MINIMAP x, y, size    RAY RENDER    RAY CLOSE
  RAY(MAPW | MAPH | CAMX | CAMY | CAMA | DIST col | WALL col | CELL x, y | CASTDIST | CASTWALL | CASTSIDE | CASTX |
      CASTY | DOOR x, y | DEFINE type, property | SPRITES | SPRITEX id | SPRITEY id)

The CMM2's:
- It draws on the write page, whatever selected it (D1). In the 4-bit depth that is one of the PicoMite's framebuffers
  (Rgb121.c), where the PicoMite's own loops write its bytes. In the 8, 12, 16 and 32-bit depths the same pixels are
  written in the page's format; where the page doubles its rows, each row is drawn once and copied to the next (D7).
- A colour is 1 to 15 for the PicoMite's of that index, opaque; anything else is a colour word as a BOX draws it, so 0 is
  BLACK, transparent in the 12-bit depth (D2). Each is converted once a frame (D3). A wall seen from the Y side is
  dimmed: an index the PicoMite's way (its green a level down), a colour word to 3/4 (D4).
- Sprites are the CMM2's sprite buffers (colour words), a pixel left out where SPRITE SHOW would leave it out (D6).
- RAY( is folded into SPRITE( by the tokeniser (MMBasic.c), and fun_sprite hands it here (D13).
- Changes from the PicoMite's: an integer map's cells clamped to 31, as its string form's are (D17, PicoMite issue #97);
  the row above the horizon drawn (the PicoMite's kept the frame before's); its messages in MMBasic's format (% and @).
- The state is in the heap: RAY CLOSE frees it, and a heap wipe (RUN, NEW, CHAIN) forgets it (RayForget). The code is
  in flash bank 2 (the linker script's .bank2).
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Rgb121.h"
#include "Ray.h"
#include <math.h>

#define F_PI 3.14159265358979323846f

#define RAY_MAX_MAP_W 256                                           // the PicoMite's limits (its Raycaster.h)
#define RAY_MAX_MAP_H 256
#define RAY_MAX_SPRITES 32
#define RAY_MAX_DOORS 8
#define RAY_MAX_WALLDEFS 32                                         // 0 empty, 1-31 the wall types
#define RAY_COLLISION_RADIUS 0.25f
#define RAY_MAXCOLOUR 0xFFFFFFF                                     // a colour word's largest (the drawing commands' check)

extern const int colours[16];                                       // the PicoMite's 16 colours (sprites.c)
extern const uint8_t fill_patterns[][8];                            // Turtle's patterns, the PicoMite's (turtle.c)

typedef struct
{
    float x, y;     /* world position */
    int spritenum;  /* sprite buffer number (1-64) */
    uint8_t active; /* 0 = unused, 1 = active */
} RaySprite;

typedef struct
{
    int map_x, map_y; /* cell position */
    float offset;     /* 0.0 = closed, 1.0 = fully open */
    uint8_t active;   /* 0 = unused, 1 = active */
} RayDoor;

typedef struct
{
    uint32_t fg;     /* foreground colour: 1-15 the PicoMite's, else a colour word (D2) */
    uint32_t bg;     /* background colour, the same */
    uint8_t pattern; /* fill pattern index, 0-31 (Turtle patterns) */
    uint8_t is_door; /* 0 = solid wall, 1 = sliding door type */
} WallDef;

typedef struct
{
    /* Map */
    int map_w, map_h;
    uint8_t *map; /* map_w * map_h bytes, each is wall type 0-31 */

    /* Camera */
    float cam_x, cam_y; /* position in map space */
    float cam_angle;    /* heading in degrees */
    float cam_fov;      /* field of view in degrees */

    /* Colours (as WallDef's) and floor/ceiling textures */
    uint32_t floor_fg, floor_bg, ceil_fg, ceil_bg;
    uint8_t floor_pat; /* floor fill pattern index 0-31 (0=Solid) */
    uint8_t ceil_pat;  /* ceiling fill pattern index 0-31 (0=Solid) */

    /* Per-column results (the write page's width) */
    float *col_dist;   /* perpendicular distance per column */
    uint8_t *col_wall; /* wall type hit per column */
    int num_cols;

    /* RAY CAST result (last single-ray cast) */
    float cast_dist; /* perpendicular distance to wall */
    int cast_wall;   /* wall type hit (0 = none) */
    int cast_side;   /* 0 = X-side, 1 = Y-side */
    int cast_mapx;   /* map X cell of the hit */
    int cast_mapy;   /* map Y cell of the hit */

    RaySprite sprites[RAY_MAX_SPRITES];
    RayDoor doors[RAY_MAX_DOORS];
    WallDef walldefs[RAY_MAX_WALLDEFS];
} RayState;

static RayState *rstate = NULL;

/***********************************************************************************************************************
The write page and its pixels (D1, D7)
***********************************************************************************************************************/
typedef struct {
    uint8_t *base;                                                  // the write page
    int w, h;                                                       // its size
    int nb;                                                         // bytes a pixel: 0 in the 4-bit depth (two a byte)
    int rowbytes;                                                   // a row's bytes
    int stride;                                                     // from a row drawn to the next: two rows where the
    int expand;                                                     // page doubles them (expand), each copied at the end
} RayPage;

static void RayPageGet(RayPage *pg) {
    if(VideoColour == 1) error("Invalid for this screen mode");    // (a bit a pixel: the PicoMite's writes 4-bit rows over it)
    pg->base = PageTable[WritePage].address;
    pg->w = PageTable[WritePage].xmax;
    pg->h = PageTable[WritePage].ymax;
    pg->nb = VideoColour == 4 ? 0 : VideoColour == 8 ? 1 : VideoColour == 32 ? 4 : 2;
    pg->rowbytes = pg->nb ? pg->w * pg->nb : (pg->w + 1) >> 1;
    pg->expand = pg->nb && PageTable[WritePage].expand;
    pg->stride = pg->expand ? 2 * pg->rowbytes : pg->rowbytes;
    if(pg->base == NULL) error("Invalid page");
}

// pixel x of a row, c as the page holds it (nb a constant where this is inlined: the loops are made for each)
static inline __attribute__((always_inline)) void RayPut(const int nb, uint8_t *row, int x, uint32_t c) {
    if(nb == 0) {                                                   // the PicoMite's: the left pixel in the low nibble
        uint8_t *b = row + (x >> 1);
        if(x & 1) *b = (*b & 0x0F) | (c << 4);
        else *b = (*b & 0xF0) | (c & 0x0F);
    } else if(nb == 1) row[x] = c;
    else if(nb == 2) ((uint16_t *)row)[x] = c;
    else ((uint32_t *)row)[x] = c;
}

// a pixel by its place (the minimap's), and a row of one colour
static void RayPixel(RayPage *pg, int x, int y, uint32_t c) {
    uint8_t *row = pg->base + y * pg->stride;
    switch(pg->nb) {
        case 0: RayPut(0, row, x, c); break;
        case 1: RayPut(1, row, x, c); break;
        case 2: RayPut(2, row, x, c); break;
        default: RayPut(4, row, x, c); break;
    }
}

static void RayHLine(RayPage *pg, int y, uint32_t c) {
    uint8_t *row = pg->base + y * pg->stride;
    switch(pg->nb) {
        case 0: memset(row, c | (c << 4), pg->rowbytes); break;
        case 1: memset(row, c, pg->rowbytes); break;
        case 2: for(int x = 0; x < pg->w; x++) ((uint16_t *)row)[x] = c; break;
        default: for(int x = 0; x < pg->w; x++) ((uint32_t *)row)[x] = c; break;
    }
}

// rows y1 to y2 drawn: each copied to the row under it, where the page doubles its rows
static void RayDouble(RayPage *pg, int y1, int y2) {
    if(!pg->expand) return;
    for(int y = y1; y <= y2; y++) memcpy(pg->base + y * pg->stride + pg->rowbytes, pg->base + y * pg->stride, pg->rowbytes);
}

// the mouse pointer, where it is drawn on the page about to be written (DrawPixel leaves its pixels alone): hidden, then
// shown again (the 4-bit depth has none)
static int RayHidePointer(void) {
    if(VideoColour < 8 || !cursoron || WritePage != (VideoColour == 12 ? 1 : 0)) return 0;
    hidecursor(0);
    return 1;
}

/***********************************************************************************************************************
Colours (D2, D3, D4)
***********************************************************************************************************************/
/* Auto-dim a 4-bit RGB121 colour by decrementing the green channel.
 * RGB121 bit layout: bit3=R, bit2-1=G(2bits), bit0=B
 * Used for Y-side walls to create a depth cue (darker than X-side).
 */
static inline uint32_t ray_dim_colour(uint32_t c)
{
    uint32_t g = (c >> 1) & 3;
    if (g > 0)
        g--;
    return (c & 0x09) | (g << 1); /* preserve R and B, replace G */
}

// the PicoMite's colour i (0-15) as the page holds it, opaque: in the 4-bit depth the index itself
static uint32_t RayIndex(uint32_t i) {
    return VideoColour == 4 ? i : SpriteNative(0x0F000000 | colours[i]);
}

// a RAY colour as the page holds it: 1 to 15 the PicoMite's, else a colour word as a BOX draws it (0 BLACK); dim, the
// Y side's: an index the PicoMite's way, in the 4-bit depth a colour word too, else a colour word to 3/4
static uint32_t RayNative(uint32_t c, int dim) {
    if(c >= 1 && c <= 15) return RayIndex(dim ? ray_dim_colour(c) : c);
    if(VideoColour == 4) return dim ? ray_dim_colour(RGB121(c)) : RGB121(c);
    if(dim) c = (c & 0xFF000000) | ((((c >> 16) & 0xFF) * 3 >> 2) << 16) | ((((c >> 8) & 0xFF) * 3 >> 2) << 8) |
                ((c & 0xFF) * 3 >> 2);
    return SpriteNative(c);
}

// a colour argument: 0 to RAY_MAXCOLOUR
static uint32_t RayColourArg(char *p) {
    return (uint32_t)getint(p, 0, RAY_MAXCOLOUR);
}

// each wall type's colours, and dimmed, as the page holds them: made at each RENDER
static struct { uint32_t fg, bg, fgd, bgd; } RayWallColour[RAY_MAX_WALLDEFS];

static void RayColours(void) {
    for(int i = 1; i < RAY_MAX_WALLDEFS; i++) {
        WallDef *wd = &rstate->walldefs[i];
        RayWallColour[i].fg = RayNative(wd->fg, 0);
        RayWallColour[i].bg = RayNative(wd->bg, 0);
        RayWallColour[i].fgd = RayNative(wd->fg, 1);
        RayWallColour[i].bgd = RayNative(wd->bg, 1);
    }
}

/***********************************************************************************************************************
The state
***********************************************************************************************************************/
/* Allocate and zero-fill raycaster state */
static void ray_alloc(void)
{
    if (rstate != NULL)
        return;
    rstate = (RayState *)GetMemory(sizeof(RayState));
    memset(rstate, 0, sizeof(RayState));
    rstate->cam_fov = 60.0f;
    rstate->floor_fg = 0; /* BLACK */
    rstate->floor_bg = 0;
    rstate->ceil_fg = 7; /* CYAN */
    rstate->ceil_bg = 7;
    rstate->floor_pat = 0; /* Solid */
    rstate->ceil_pat = 0;

    /* Initialise wall definitions with backwards-compatible defaults:
     * Types 1-15:  green walls  (fg=GREEN, bg=MIDGREEN, pat=type-1)
     * Types 16-31: brown doors  (fg=YELLOW, bg=BROWN, pat=type-1, is_door=1) */
    for (int i = 1; i < RAY_MAX_WALLDEFS; i++)
    {
        rstate->walldefs[i].pattern = (i - 1) % NUM_PATTERNS;
        if (i >= 16)
        {
            rstate->walldefs[i].fg = 14; /* YELLOW */
            rstate->walldefs[i].bg = 12; /* BROWN  */
            rstate->walldefs[i].is_door = 1;
        }
        else
        {
            rstate->walldefs[i].fg = 6; /* GREEN    */
            rstate->walldefs[i].bg = 4; /* MIDGREEN */
            rstate->walldefs[i].is_door = 0;
        }
    }
}

/* Free per-column arrays */
static void ray_free_columns(void)
{
    if (rstate == NULL)
        return;
    FreeMemorySafe((void **)&rstate->col_dist);
    FreeMemorySafe((void **)&rstate->col_wall);
    rstate->num_cols = 0;
}

/* Ensure per-column arrays match the page's width */
static void ray_ensure_columns(int ncols)
{
    if (rstate->num_cols == ncols)
        return;
    ray_free_columns();
    rstate->col_dist = (float *)GetMemory(ncols * sizeof(float));
    rstate->col_wall = (uint8_t *)GetMemory(ncols);
    rstate->num_cols = ncols;
}

void ray_close(void)
{
    if (rstate == NULL)
        return;
    ray_free_columns();
    FreeMemorySafe((void **)&rstate->map);
    FreeMemorySafe((void **)&rstate);
}

// the heap was wiped (HeapWipe, Memory.c): the state went with it
void RayForget(void)
{
    rstate = NULL;
}

/***********************************************************************************************************************
Doors and collision (the PicoMite's)
***********************************************************************************************************************/
/* Get the door offset for a map cell, or 0.0 if not an active door slot */
static float ray_get_door_offset(int mx, int my)
{
    if (rstate == NULL)
        return 0.0f;
    for (int i = 0; i < RAY_MAX_DOORS; i++)
    {
        if (rstate->doors[i].active &&
            rstate->doors[i].map_x == mx &&
            rstate->doors[i].map_y == my)
            return rstate->doors[i].offset;
    }
    return 0.0f;
}

/* Check if a ray passes through the open portion of a sliding door.
 * Returns 1 if the ray passes through, 0 if it hits the door.
 * Must only be called for cells whose WallDef has is_door=1. */
static int ray_door_check_pass(int mx, int my, int side,
                               float pos_x, float pos_y,
                               float ray_dx, float ray_dy,
                               float side_x, float side_y,
                               float delta_x, float delta_y)
{
    float door_off = ray_get_door_offset(mx, my);
    if (door_off <= 0.0f)
        return 0; /* fully closed */
    if (door_off >= 1.0f)
        return 1; /* fully open */
    float perp = (side == 0) ? (side_x - delta_x) : (side_y - delta_y);
    if (perp < 0.001f)
        perp = 0.001f;
    float frac = (side == 0) ? (pos_y + perp * ray_dy) : (pos_x + perp * ray_dx);
    frac -= floorf(frac);
    return (frac < door_off) ? 1 : 0;
}

/* Check if a map cell blocks movement (collision detection).
 * Returns 1 if blocked, 0 if passable (empty or fully-open door). */
static inline int ray_cell_blocks(uint8_t *m, int mw, int x, int y)
{
    uint8_t wt = m[y * mw + x];
    if (wt == 0)
        return 0;
    if (rstate->walldefs[wt].is_door && ray_get_door_offset(x, y) >= 1.0f)
        return 0;
    return 1;
}

/***********************************************************************************************************************
RAY RENDER
***********************************************************************************************************************/
/* The floor and ceiling (the PicoMite's): for each row below the horizon, the world-space floor coordinates from the ray
 * directions at the leftmost and rightmost columns, only additions in the inner loop; the pattern bit picks the fg or
 * bg colour; the ceiling is mirrored above the horizon. nb is a constant where this is inlined. */
static inline __attribute__((always_inline)) void RayFloor(const int nb, RayPage *pg, float pos_x, float pos_y,
                                                           float ray_x0, float ray_y0, float ray_x1, float ray_y1,
                                                           uint32_t f_fg, uint32_t f_bg, uint32_t c_fg, uint32_t c_bg,
                                                           int f_pat, int c_pat)
{
    int hres = pg->w, vres = pg->h, half_vres = vres >> 1;
    float inv_hres = 1.0f / (float)hres;

    for (int y = half_vres + 1; y < vres; y++)
    {
        /* Row distance: camera height (0.5) * screen height / row offset */
        float row_dist = (float)half_vres / (float)(y - half_vres);

        /* Floor position at leftmost column */
        float floor_x = pos_x + row_dist * ray_x0;
        float floor_y = pos_y + row_dist * ray_y0;

        /* Step per column */
        float step_x = row_dist * (ray_x1 - ray_x0) * inv_hres;
        float step_y = row_dist * (ray_y1 - ray_y0) * inv_hres;

        uint8_t *floor_row = pg->base + y * pg->stride;
        /* Ceiling row (mirrored): row (vres - 1 - y) */
        uint8_t *ceil_row = pg->base + (vres - 1 - y) * pg->stride;

        if (nb == 0)
        {
            for (int x = 0; x < hres; x += 2) /* the PicoMite's: a pair of pixels a byte */
            {
                /* Even pixel (low nibble) */
                int tx = ((int)(floor_x * 16.0f)) & 7;
                int ty = ((int)(floor_y * 16.0f)) & 7;
                uint32_t f_col = (fill_patterns[f_pat][ty] & (0x80 >> tx)) ? f_fg : f_bg;
                uint32_t c_col = (fill_patterns[c_pat][ty] & (0x80 >> tx)) ? c_fg : c_bg;

                floor_x += step_x;
                floor_y += step_y;

                /* Odd pixel (high nibble) */
                int tx1 = ((int)(floor_x * 16.0f)) & 7;
                int ty1 = ((int)(floor_y * 16.0f)) & 7;
                uint32_t f_col1 = (fill_patterns[f_pat][ty1] & (0x80 >> tx1)) ? f_fg : f_bg;
                uint32_t c_col1 = (fill_patterns[c_pat][ty1] & (0x80 >> tx1)) ? c_fg : c_bg;

                floor_x += step_x;
                floor_y += step_y;

                /* Write pixel pair */
                int bx = x >> 1;
                floor_row[bx] = (f_col1 << 4) | f_col;
                ceil_row[bx] = (c_col1 << 4) | c_col;
            }
        }
        else
        {
            for (int x = 0; x < hres; x++)
            {
                int tx = ((int)(floor_x * 16.0f)) & 7;
                int ty = ((int)(floor_y * 16.0f)) & 7;
                RayPut(nb, floor_row, x, (fill_patterns[f_pat][ty] & (0x80 >> tx)) ? f_fg : f_bg);
                RayPut(nb, ceil_row, x, (fill_patterns[c_pat][ty] & (0x80 >> tx)) ? c_fg : c_bg);
                floor_x += step_x;
                floor_y += step_y;
            }
        }
        if (!(y & 15))
            routinechecks(0); /* (the keyboard: a big page takes a while) */
    }
}

/* Textured vertical line draw: the pattern bit selects fg or bg, the pattern tiled twice up the wall (the PicoMite's
 * ray_vline_textured, its colours already chosen for the side). nb is a constant where this is inlined. */
static inline __attribute__((always_inline)) void RayVline(const int nb, RayPage *pg, int x, int y_start, int y_end,
                                                           const uint8_t *pattern, int tex_x, uint32_t fg_col,
                                                           uint32_t bg_col)
{
    /* Map screen Y range to texture Y (0-7, tiled 2x per wall height) */
    int wall_screen_h = y_end - y_start + 1;
    uint8_t *row = pg->base + y_start * pg->stride;

    for (int y = y_start; y <= y_end; y++, row += pg->stride)
    {
        /* Texture Y coordinate — tile pattern 2x vertically */
        int tex_y = (((y - y_start) * 16) / wall_screen_h) & 7;
        RayPut(nb, row, x, (pattern[tex_y] & (0x80 >> tex_x)) ? fg_col : bg_col);
    }
}

static void ray_vline_textured(RayPage *pg, int x, int y_start, int y_end, int wall_type, int tex_x, int side)
{
    if (y_start > y_end)
        return;
    int pat_idx = rstate->walldefs[wall_type].pattern;
    if (pat_idx >= NUM_PATTERNS)
        pat_idx = NUM_PATTERNS - 1;
    const uint8_t *pattern = fill_patterns[pat_idx];
    /* Select colour pair: X-side uses definition colours, Y-side auto-dims */
    uint32_t fg = side == 0 ? RayWallColour[wall_type].fg : RayWallColour[wall_type].fgd;
    uint32_t bg = side == 0 ? RayWallColour[wall_type].bg : RayWallColour[wall_type].bgd;
    switch (pg->nb)
    {
    case 0: RayVline(0, pg, x, y_start, y_end, pattern, tex_x, fg, bg); break;
    case 1: RayVline(1, pg, x, y_start, y_end, pattern, tex_x, fg, bg); break;
    case 2: RayVline(2, pg, x, y_start, y_end, pattern, tex_x, fg, bg); break;
    default: RayVline(4, pg, x, y_start, y_end, pattern, tex_x, fg, bg); break;
    }
}

/* A billboard's columns (the PicoMite's), each where no nearer wall is: its image's colour words sampled, a pixel that
 * SPRITE SHOW would leave out (SpriteKeyNative) left out. depth is a constant where this is inlined. */
static inline __attribute__((always_inline)) void RaySpriteColumns(const int depth, RayPage *pg, const uint32_t *img,
                                                                   int spr_img_w, int spr_img_h, float ty, int x0,
                                                                   int y0, int spr_scr_w, int spr_scr_h, int cx0,
                                                                   int cx1, int cy0, int cy1, uint32_t mask,
                                                                   uint32_t key)
{
    const int nb = depth == 4 ? 0 : depth == 8 ? 1 : depth == 32 ? 4 : 2;
    for (int x = cx0; x <= cx1; x++)
    {
        /* Z-buffer: skip column if wall is closer */
        if (ty >= rstate->col_dist[x])
            continue;

        int tex_x = ((x - x0) * spr_img_w) / spr_scr_w;
        if (tex_x >= spr_img_w)
            tex_x = spr_img_w - 1;
        uint8_t *row = pg->base + cy0 * pg->stride;

        for (int y = cy0; y <= cy1; y++, row += pg->stride)
        {
            int tex_y = ((y - y0) * spr_img_h) / spr_scr_h;
            if (tex_y >= spr_img_h)
                tex_y = spr_img_h - 1;
            uint32_t a = img[tex_y * spr_img_w + tex_x];
            uint32_t c = depth == 32 ? NATIVE_32(a) : depth == 16 ? NATIVE_16(a) : depth == 12 ? NATIVE_12(a) :
                         depth == 8 ? RGB332(a) : RGB121(a);
            /* Skip transparent pixels */
            if ((c & mask) == key)
                continue;
            RayPut(nb, row, x, c);
        }
    }
}

/* Sprite rendering — depth-sorted billboards drawn after walls */
static void ray_render_sprites(RayPage *pg)
{
    if (rstate == NULL || rstate->col_dist == NULL)
        return;

    /* Collect active sprites and compute squared distances */
    int n = 0;
    int order[RAY_MAX_SPRITES];
    float sdist[RAY_MAX_SPRITES];
    float px = rstate->cam_x, py = rstate->cam_y;

    for (int i = 0; i < RAY_MAX_SPRITES; i++)
    {
        if (!rstate->sprites[i].active)
            continue;
        float dx = rstate->sprites[i].x - px;
        float dy = rstate->sprites[i].y - py;
        sdist[n] = dx * dx + dy * dy;
        order[n] = i;
        n++;
    }
    if (n == 0)
        return;

    /* Sort furthest-first (insertion sort, max 32 elements) */
    for (int i = 1; i < n; i++)
    {
        float kd = sdist[i];
        int ki = order[i];
        int j = i - 1;
        while (j >= 0 && sdist[j] < kd)
        {
            sdist[j + 1] = sdist[j];
            order[j + 1] = order[j];
            j--;
        }
        sdist[j + 1] = kd;
        order[j + 1] = ki;
    }

    /* Camera vectors */
    float angle_rad = rstate->cam_angle * (F_PI / 180.0f);
    float dir_x = cosf(angle_rad);
    float dir_y = sinf(angle_rad);
    float plane_len = tanf(rstate->cam_fov * 0.5f * (F_PI / 180.0f));
    float plane_x = -dir_y * plane_len;
    float plane_y = dir_x * plane_len;

    /* Inverse determinant of the 2x2 camera matrix */
    float inv_det = 1.0f / (plane_x * dir_y - dir_x * plane_y);
    int hres = pg->w, vres = pg->h;
    int half_hres = hres >> 1;
    int half_vres = vres >> 1;
    uint32_t mask = SpriteMask(), key = SpriteKeyNative();

    for (int s = 0; s < n; s++)
    {
        RaySprite *sp = &rstate->sprites[order[s]];
        int snum = sp->spritenum;
        if (snum < 1 || snum >= MAXBLITBUF || blitbuff[snum].blitbuffptr == NULL)
            continue;

        int spr_img_w = blitbuff[snum].w;
        int spr_img_h = blitbuff[snum].h;
        const uint32_t *img = (const uint32_t *)blitbuff[snum].blitbuffptr;

        float sx = sp->x - px;
        float sy = sp->y - py;

        /* Transform sprite position to camera space */
        float tx = inv_det * (dir_y * sx - dir_x * sy);
        float ty = inv_det * (-plane_y * sx + plane_x * sy);

        if (ty <= 0.1f)
            continue; /* behind camera */

        int scr_x = (int)(half_hres * (1.0f + tx / ty));
        /* Screen height based on sprite image aspect ratio */
        int spr_scr_h = (int)(vres / ty);
        if (spr_scr_h < 1)
            continue;
        if (spr_scr_h > vres * 4)
            spr_scr_h = vres * 4;
        int spr_scr_w = (spr_scr_h * spr_img_w) / spr_img_h;
        if (spr_scr_w < 1)
            spr_scr_w = 1;

        int y0 = half_vres - spr_scr_h / 2;
        int x0 = scr_x - spr_scr_w / 2;
        int x1 = x0 + spr_scr_w - 1;
        int y1 = y0 + spr_scr_h - 1;

        /* Clamp to screen bounds */
        int cx0 = (x0 < 0) ? 0 : x0;
        int cx1 = (x1 >= hres) ? hres - 1 : x1;
        int cy0 = (y0 < 0) ? 0 : y0;
        int cy1 = (y1 >= vres) ? vres - 1 : y1;

        switch (VideoColour)
        {
        case 4: RaySpriteColumns(4, pg, img, spr_img_w, spr_img_h, ty, x0, y0, spr_scr_w, spr_scr_h, cx0, cx1, cy0, cy1, mask, key); break;
        case 8: RaySpriteColumns(8, pg, img, spr_img_w, spr_img_h, ty, x0, y0, spr_scr_w, spr_scr_h, cx0, cx1, cy0, cy1, mask, key); break;
        case 12: RaySpriteColumns(12, pg, img, spr_img_w, spr_img_h, ty, x0, y0, spr_scr_w, spr_scr_h, cx0, cx1, cy0, cy1, mask, key); break;
        case 16: RaySpriteColumns(16, pg, img, spr_img_w, spr_img_h, ty, x0, y0, spr_scr_w, spr_scr_h, cx0, cx1, cy0, cy1, mask, key); break;
        default: RaySpriteColumns(32, pg, img, spr_img_w, spr_img_h, ty, x0, y0, spr_scr_w, spr_scr_h, cx0, cx1, cy0, cy1, mask, key); break;
        }
    }
}

/* Core DDA raycasting engine */
static void ray_render_to_buffer(RayPage *pg)
{
    if (rstate == NULL)
        error("Raycaster not initialised");
    if (rstate->map == NULL)
        error("No map defined");

    int hres = pg->w, vres = pg->h;
    ray_ensure_columns(hres);
    RayColours();

    int half_vres = vres >> 1;

    /* Camera vectors */
    float angle_rad = rstate->cam_angle * (F_PI / 180.0f);
    float dir_x = cosf(angle_rad);
    float dir_y = sinf(angle_rad);

    /* Camera plane (perpendicular to direction, scaled by FOV) */
    float plane_len = tanf(rstate->cam_fov * 0.5f * (F_PI / 180.0f));
    float plane_x = -dir_y * plane_len;
    float plane_y = dir_x * plane_len;

    float pos_x = rstate->cam_x;
    float pos_y = rstate->cam_y;
    int map_w = rstate->map_w;
    int map_h = rstate->map_h;
    uint8_t *map = rstate->map;

    /* ---- Textured floor and ceiling ---- */
    {
        /* Ray directions at leftmost and rightmost screen edges */
        float ray_x0 = dir_x - plane_x;
        float ray_y0 = dir_y - plane_y;
        float ray_x1 = dir_x + plane_x;
        float ray_y1 = dir_y + plane_y;

        uint32_t f_fg = RayNative(rstate->floor_fg, 0);
        uint32_t f_bg = RayNative(rstate->floor_bg, 0);
        uint32_t c_fg = RayNative(rstate->ceil_fg, 0);
        uint32_t c_bg = RayNative(rstate->ceil_bg, 0);
        int f_pat = rstate->floor_pat;
        int c_pat = rstate->ceil_pat;
        if (f_pat >= NUM_PATTERNS)
            f_pat = 0;
        if (c_pat >= NUM_PATTERNS)
            c_pat = 0;

        switch (pg->nb)
        {
        case 0: RayFloor(0, pg, pos_x, pos_y, ray_x0, ray_y0, ray_x1, ray_y1, f_fg, f_bg, c_fg, c_bg, f_pat, c_pat); break;
        case 1: RayFloor(1, pg, pos_x, pos_y, ray_x0, ray_y0, ray_x1, ray_y1, f_fg, f_bg, c_fg, c_bg, f_pat, c_pat); break;
        case 2: RayFloor(2, pg, pos_x, pos_y, ray_x0, ray_y0, ray_x1, ray_y1, f_fg, f_bg, c_fg, c_bg, f_pat, c_pat); break;
        default: RayFloor(4, pg, pos_x, pos_y, ray_x0, ray_y0, ray_x1, ray_y1, f_fg, f_bg, c_fg, c_bg, f_pat, c_pat); break;
        }
        /* Fill the horizon line itself with ceiling colour, and the row above it, which the mirror misses (the
           PicoMite's kept the frame before's there) */
        RayHLine(pg, half_vres, c_fg);
        if (half_vres > 0)
            RayHLine(pg, half_vres - 1, c_fg);
    }

    /* Cast one ray per screen column */
    for (int col = 0; col < hres; col++)
    {
        /* Camera-space X coordinate: -1.0 (left) to +1.0 (right) */
        float camera_x = 2.0f * col / (float)hres - 1.0f;

        /* Ray direction */
        float ray_dx = dir_x + plane_x * camera_x;
        float ray_dy = dir_y + plane_y * camera_x;

        /* Current map cell */
        int map_x = (int)pos_x;
        int map_y = (int)pos_y;

        /* Delta distances: |1/ray_component| */
        float delta_dist_x = (ray_dx == 0.0f) ? 1e30f : fabsf(1.0f / ray_dx);
        float delta_dist_y = (ray_dy == 0.0f) ? 1e30f : fabsf(1.0f / ray_dy);

        /* Step direction and initial side distances */
        int step_x, step_y;
        float side_dist_x, side_dist_y;

        if (ray_dx < 0)
        {
            step_x = -1;
            side_dist_x = (pos_x - map_x) * delta_dist_x;
        }
        else
        {
            step_x = 1;
            side_dist_x = (map_x + 1.0f - pos_x) * delta_dist_x;
        }
        if (ray_dy < 0)
        {
            step_y = -1;
            side_dist_y = (pos_y - map_y) * delta_dist_y;
        }
        else
        {
            step_y = 1;
            side_dist_y = (map_y + 1.0f - pos_y) * delta_dist_y;
        }

        /* DDA loop */
        int hit = 0;
        int side = 0; /* 0 = X-side, 1 = Y-side */
        int wall_type = 0;

        while (!hit)
        {
            if (side_dist_x < side_dist_y)
            {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = 0;
            }
            else
            {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = 1;
            }
            /* Bounds check */
            if (map_x < 0 || map_x >= map_w || map_y < 0 || map_y >= map_h)
            {
                hit = 1;
                wall_type = 1; /* boundary wall */
            }
            else
            {
                wall_type = map[map_y * map_w + map_x];
                if (wall_type > 0)
                {
                    if (rstate->walldefs[wall_type].is_door &&
                        ray_door_check_pass(map_x, map_y, side,
                                            pos_x, pos_y, ray_dx, ray_dy,
                                            side_dist_x, side_dist_y,
                                            delta_dist_x, delta_dist_y))
                    {
                        /* Ray passes through open portion of door */
                    }
                    else
                    {
                        hit = 1;
                    }
                }
            }
        }

        /* Perpendicular distance (avoids fisheye) */
        float perp_dist;
        if (side == 0)
            perp_dist = side_dist_x - delta_dist_x;
        else
            perp_dist = side_dist_y - delta_dist_y;

        if (perp_dist < 0.001f)
            perp_dist = 0.001f;

        /* Store for query functions */
        rstate->col_dist[col] = perp_dist;
        rstate->col_wall[col] = (uint8_t)wall_type;

        /* Wall strip height */
        int line_height = (int)(vres / perp_dist);
        int draw_start = half_vres - line_height / 2;
        int draw_end = half_vres + line_height / 2 - 1;
        if (draw_start < 0)
            draw_start = 0;
        if (draw_end >= vres)
            draw_end = vres - 1;

        /* Texture X coordinate (0-7) from wall hit position */
        float wall_x;
        if (side == 0)
            wall_x = pos_y + perp_dist * ray_dy;
        else
            wall_x = pos_x + perp_dist * ray_dx;
        wall_x -= floorf(wall_x);
        int tex_x = (int)(wall_x * 16.0f) & 7;

        /* Draw the wall column */
        if (wall_type > 0 && draw_start <= draw_end)
            ray_vline_textured(pg, col, draw_start, draw_end, wall_type, tex_x, side);
        if (!(col & 15))
            routinechecks(0);
    }

    /* Render billboard sprites (depth-sorted, z-buffered against walls) */
    ray_render_sprites(pg);
}

static void ray_cmd_render(char *p)
{
    RayPage pg;
    if (rstate == NULL)
        error("Raycaster not initialised");
    if (rstate->map == NULL)
        error("No map defined");
    RayPageGet(&pg);
    int hidden = RayHidePointer();
    ray_render_to_buffer(&pg);
    RayDouble(&pg, 0, pg.h - 1);
    if (hidden)
        showcursor(0, xcursor, ycursor);
}

/***********************************************************************************************************************
The commands
***********************************************************************************************************************/
/* Helper: decode a map character to a cell value (0-35).
 * '0'-'9' → 0-9, 'A'-'Z' / 'a'-'z' → 10-35.
 * Returns -1 for invalid characters. */
static int ray_decode_map_char(unsigned char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 10;
    return -1;
}

/* RAY MAP w, h, map%()       — integer array form
 * RAY MAP w, h, map$()       — string array form: each element a row, '0'-'9' → 0-9, 'A'-'Z' / 'a'-'z' → 10-35
 * (above 31 is 31 in both: the PicoMite's integer form took 32, past its last wall type) */
static void ray_cmd_map(char *p)
{
    getargs(&p, 5, ",");
    if (argc < 5)
        error("Expected: RAY MAP w, h, map$() or map$()", "%", "$");   // (error()'s codes: the PicoMite's #111)

    int w = getint(argv[0], 1, RAY_MAX_MAP_W);
    int h = getint(argv[2], 1, RAY_MAX_MAP_H);

    ray_alloc();

    /* (Re)allocate map storage */
    FreeMemorySafe((void **)&rstate->map);
    rstate->map = (uint8_t *)GetMemory(w * h);
    rstate->map_w = w;
    rstate->map_h = h;

    /* Probe the variable type to decide integer vs string array */
    findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
    if (FoundType() & T_STR)
    {
        /* ---- String array form ---- */
        char *a1str = NULL;
        unsigned char str_size = 0;
        int dims[MAXDIM] = {0};
        int card = parsestringarray(argv[4], &a1str, 3, 1, dims, false, &str_size);
        if (card < h)
            error("String array too small: need % rows", h);

        int stride = (int)str_size + 1;

        for (int row = 0; row < h; row++)
        {
            unsigned char *elem = (unsigned char *)a1str + row * stride;
            int slen = elem[0]; /* length-prefix byte */
            if (slen < w)
                error("Row % string too short: need % chars", row, w);

            for (int col = 0; col < w; col++)
            {
                int v = ray_decode_map_char(elem[1 + col]);
                if (v < 0)
                    error("Invalid map character '@' at row %, col %", (int)elem[1 + col], row, col);
                if (v >= RAY_MAX_WALLDEFS)
                    v = RAY_MAX_WALLDEFS - 1;
                rstate->map[row * w + col] = (uint8_t)v;
            }
        }
    }
    else
    {
        /* ---- Integer array form ---- */
        int64_t *arr = NULL;
        int arr_size = parseintegerarray(argv[4], &arr, 3, 1, NULL, false, NULL);
        if (arr_size < w * h)
            error("Map array too small: need % elements", w * h);

        for (int i = 0; i < w * h; i++)
        {
            int64_t v = arr[i];
            if (v < 0)
                v = 0;
            if (v >= RAY_MAX_WALLDEFS)
                v = RAY_MAX_WALLDEFS - 1;
            rstate->map[i] = (uint8_t)v;
        }
    }
}

/* RAY CAMERA x!, y!, angle! [, fov!] */
static void ray_cmd_camera(char *p)
{
    getargs(&p, 7, ",");
    if (argc < 5)
        error("Expected: RAY CAMERA x, y, angle [, fov]");

    ray_alloc();
    rstate->cam_x = (float)getnumber(argv[0]);
    rstate->cam_y = (float)getnumber(argv[2]);
    rstate->cam_angle = (float)getnumber(argv[4]);
    if (argc >= 7)
        rstate->cam_fov = (float)getnumber(argv[6]);
    if (rstate->cam_fov < 10.0f)
        rstate->cam_fov = 10.0f;
    if (rstate->cam_fov > 170.0f)
        rstate->cam_fov = 170.0f;
}

/* RAY COLOUR floor_fg, ceil_fg [, floor_bg, ceil_bg, floor_pat, ceil_pat]
 * 2 args: solid floor and ceiling; 6 args: textured floor and ceiling */
static void ray_cmd_colour(char *p)
{
    getargs(&p, 11, ",");
    if (argc < 3)
        error("Expected: RAY COLOUR floor, ceiling [, floor_bg, ceiling_bg, floor_pat, ceiling_pat]");

    ray_alloc();

    if (argc >= 11)
    {
        /* 6 arguments: full textured floor/ceiling */
        rstate->floor_fg = RayColourArg(argv[0]);
        rstate->ceil_fg = RayColourArg(argv[2]);
        rstate->floor_bg = RayColourArg(argv[4]);
        rstate->ceil_bg = RayColourArg(argv[6]);
        rstate->floor_pat = (uint8_t)(getint(argv[8], 0, NUM_PATTERNS - 1));
        rstate->ceil_pat = (uint8_t)(getint(argv[10], 0, NUM_PATTERNS - 1));
    }
    else
    {
        /* 2 arguments: solid floor and ceiling */
        rstate->floor_fg = RayColourArg(argv[0]);
        rstate->floor_bg = rstate->floor_fg;
        rstate->ceil_fg = RayColourArg(argv[2]);
        rstate->ceil_bg = rstate->ceil_fg;
        rstate->floor_pat = 0; /* Solid */
        rstate->ceil_pat = 0;
    }
}

/* Helper: evaluate an argument as a wall type: an integer, or a single-character string ('0'-'9' → 0-9, 'A'-'Z' /
 * 'a'-'z' → 10-35) */
static int ray_get_walltype(char *arg, int lo, int hi)
{
    MMFLOAT f;
    long long int i64;
    char *s = NULL;
    int t = T_NOTYPE;

    evaluate(arg, &f, &i64, &s, &t, false);

    if (t & T_STR)
    {
        /* String: must be exactly 1 character */
        int slen = *(unsigned char *)s;
        if (slen != 1)
            error("Expected single character for wall type");
        int v = ray_decode_map_char(s[1]);
        if (v < 0)
            error("Invalid wall type character");
        if (v < lo || v > hi)
            error("% is invalid (valid is % to %)", v, lo, hi);
        return v;
    }
    else
    {
        /* Numeric: convert float to int if needed */
        int v = (t & T_INT) ? (int)i64 : (int)f;
        if (v < lo || v > hi)
            error("% is invalid (valid is % to %)", v, lo, hi);
        return v;
    }
}

/* RAY DEFINE type, fg, bg, pattern [, door]: a wall type's (1-31) colours (D2), pattern (0-31) and door flag */
static void ray_cmd_define(char *p)
{
    getargs(&p, 9, ",");
    if (argc < 7)
        error("Expected: RAY DEFINE type, fg, bg, pattern [, door]");

    ray_alloc();

    int wtype = ray_get_walltype(argv[0], 1, RAY_MAX_WALLDEFS - 1);
    uint32_t fg = RayColourArg(argv[2]);
    uint32_t bg = RayColourArg(argv[4]);
    int pat = getint(argv[6], 0, NUM_PATTERNS - 1);
    int door = 0;
    if (argc >= 9)
        door = getint(argv[8], 0, 1);

    rstate->walldefs[wtype].fg = fg;
    rstate->walldefs[wtype].bg = bg;
    rstate->walldefs[wtype].pattern = (uint8_t)pat;
    rstate->walldefs[wtype].is_door = (uint8_t)door;
}

/* RAY CELL x, y, value: value 0-31, or a single character '0'-'9','A'-'Z' */
static void ray_cmd_cell(char *p)
{
    getargs(&p, 5, ",");
    if (argc < 5)
        error("Expected: RAY CELL x, y, value");

    if (rstate == NULL || rstate->map == NULL)
        error("No map defined");

    int x = getint(argv[0], 0, rstate->map_w - 1);
    int y = getint(argv[2], 0, rstate->map_h - 1);
    int v = ray_get_walltype(argv[4], 0, RAY_MAX_WALLDEFS - 1);

    rstate->map[y * rstate->map_w + x] = (uint8_t)v;
}

/* Internal: cast a single ray from camera at a given absolute angle (degrees); the result in rstate->cast_* */
static void ray_do_cast(float angle_deg)
{
    if (rstate == NULL)
        error("Raycaster not initialised");
    if (rstate->map == NULL)
        error("No map defined");

    float angle_rad = angle_deg * (F_PI / 180.0f);
    float ray_dx = cosf(angle_rad);
    float ray_dy = sinf(angle_rad);

    float pos_x = rstate->cam_x;
    float pos_y = rstate->cam_y;
    int map_w = rstate->map_w;
    int map_h = rstate->map_h;
    uint8_t *map = rstate->map;

    int map_x = (int)pos_x;
    int map_y = (int)pos_y;

    float delta_dist_x = (ray_dx == 0.0f) ? 1e30f : fabsf(1.0f / ray_dx);
    float delta_dist_y = (ray_dy == 0.0f) ? 1e30f : fabsf(1.0f / ray_dy);

    int step_x, step_y;
    float side_dist_x, side_dist_y;

    if (ray_dx < 0)
    {
        step_x = -1;
        side_dist_x = (pos_x - map_x) * delta_dist_x;
    }
    else
    {
        step_x = 1;
        side_dist_x = (map_x + 1.0f - pos_x) * delta_dist_x;
    }
    if (ray_dy < 0)
    {
        step_y = -1;
        side_dist_y = (pos_y - map_y) * delta_dist_y;
    }
    else
    {
        step_y = 1;
        side_dist_y = (map_y + 1.0f - pos_y) * delta_dist_y;
    }

    int hit = 0;
    int side = 0;
    int wall_type = 0;

    while (!hit)
    {
        if (side_dist_x < side_dist_y)
        {
            side_dist_x += delta_dist_x;
            map_x += step_x;
            side = 0;
        }
        else
        {
            side_dist_y += delta_dist_y;
            map_y += step_y;
            side = 1;
        }
        if (map_x < 0 || map_x >= map_w || map_y < 0 || map_y >= map_h)
        {
            hit = 1;
            wall_type = 1;
        }
        else
        {
            wall_type = map[map_y * map_w + map_x];
            if (wall_type > 0)
            {
                if (rstate->walldefs[wall_type].is_door &&
                    ray_door_check_pass(map_x, map_y, side,
                                        pos_x, pos_y, ray_dx, ray_dy,
                                        side_dist_x, side_dist_y,
                                        delta_dist_x, delta_dist_y))
                {
                    /* Ray passes through open portion of door */
                }
                else
                {
                    hit = 1;
                }
            }
        }
    }

    float perp_dist;
    if (side == 0)
        perp_dist = side_dist_x - delta_dist_x;
    else
        perp_dist = side_dist_y - delta_dist_y;
    if (perp_dist < 0.001f)
        perp_dist = 0.001f;

    rstate->cast_dist = perp_dist;
    rstate->cast_wall = wall_type;
    rstate->cast_side = side;
    rstate->cast_mapx = map_x;
    rstate->cast_mapy = map_y;
}

/* RAY CAST angle!: results through RAY(CASTDIST), RAY(CASTWALL) and the others */
static void ray_cmd_cast(char *p)
{
    getargs(&p, 1, ",");
    if (argc < 1)
        error("Expected: RAY CAST angle");

    float angle = (float)getnumber(argv[0]);
    ray_do_cast(angle);
}

/* Collision detection helper: 1 if a box of ±radius around (x,y) overlaps a wall cell */
static int ray_is_blocked(float x, float y, float radius)
{
    if (rstate == NULL || rstate->map == NULL)
        return 1;

    int mw = rstate->map_w;
    int mh = rstate->map_h;
    uint8_t *m = rstate->map;

    /* Check all 4 corners of the bounding box */
    int x0 = (int)(x - radius);
    int x1 = (int)(x + radius);
    int y0 = (int)(y - radius);
    int y1 = (int)(y + radius);

    /* Out of bounds = blocked */
    if (x0 < 0 || x1 >= mw || y0 < 0 || y1 >= mh)
        return 1;

    if (m[y0 * mw + x0] > 0 && ray_cell_blocks(m, mw, x0, y0))
        return 1;
    if (m[y0 * mw + x1] > 0 && ray_cell_blocks(m, mw, x1, y0))
        return 1;
    if (m[y1 * mw + x0] > 0 && ray_cell_blocks(m, mw, x0, y1))
        return 1;
    if (m[y1 * mw + x1] > 0 && ray_cell_blocks(m, mw, x1, y1))
        return 1;

    return 0;
}

/* RAY MOVE speed! [, strafe!]: with collision detection and wall sliding (X only, then Y only) */
static void ray_cmd_move(char *p)
{
    getargs(&p, 3, ",");
    if (argc < 1)
        error("Expected: RAY MOVE speed [, strafe]");

    if (rstate == NULL)
        error("Raycaster not initialised");
    if (rstate->map == NULL)
        error("No map defined");

    float speed = (float)getnumber(argv[0]);
    float strafe = 0.0f;
    if (argc >= 3)
        strafe = (float)getnumber(argv[2]);

    float angle_rad = rstate->cam_angle * (F_PI / 180.0f);
    float dx = cosf(angle_rad);
    float dy = sinf(angle_rad);

    /* Forward/backward + strafe (strafe is perpendicular to facing) */
    float move_x = dx * speed - dy * strafe;
    float move_y = dy * speed + dx * strafe;

    float cur_x = rstate->cam_x;
    float cur_y = rstate->cam_y;
    float new_x = cur_x + move_x;
    float new_y = cur_y + move_y;
    float r = RAY_COLLISION_RADIUS;

    /* Try full movement */
    if (!ray_is_blocked(new_x, new_y, r))
    {
        rstate->cam_x = new_x;
        rstate->cam_y = new_y;
        return;
    }

    /* Wall sliding: try X-only */
    if (!ray_is_blocked(new_x, cur_y, r))
    {
        rstate->cam_x = new_x;
        return;
    }

    /* Wall sliding: try Y-only */
    if (!ray_is_blocked(cur_x, new_y, r))
    {
        rstate->cam_y = new_y;
        return;
    }

    /* Completely blocked — don't move */
}

/* RAY TURN degrees!: positive = clockwise/right; the angle kept 0-360 */
static void ray_cmd_turn(char *p)
{
    getargs(&p, 1, ",");
    if (argc < 1)
        error("Expected: RAY TURN degrees");

    if (rstate == NULL)
        error("Raycaster not initialised");

    float degrees = (float)getnumber(argv[0]);
    rstate->cam_angle += degrees;

    /* Normalise to 0-360 */
    while (rstate->cam_angle < 0.0f)
        rstate->cam_angle += 360.0f;
    while (rstate->cam_angle >= 360.0f)
        rstate->cam_angle -= 360.0f;
}

/* RAY SPRITE id, spritenum, x!, y! | RAY SPRITE REMOVE id | RAY SPRITE CLEAR: spritenum a sprite buffer (1-64) */
static void ray_cmd_sprite(char *p)
{
    char *q;
    if ((q = checkstring(p, "CLEAR")))
    {
        if (rstate)
            memset(rstate->sprites, 0, sizeof(rstate->sprites));
        return;
    }
    if ((q = checkstring(p, "REMOVE")))
    {
        getargs(&q, 1, ",");
        if (argc < 1)
            error("Expected: RAY SPRITE REMOVE id");
        int id = getint(argv[0], 0, RAY_MAX_SPRITES - 1);
        if (rstate)
            rstate->sprites[id].active = 0;
        return;
    }

    getargs(&p, 7, ",");
    if (argc < 7)
        error("Expected: RAY SPRITE id, spritenum, x, y");

    ray_alloc();

    int id = getint(argv[0], 0, RAY_MAX_SPRITES - 1);
    int snum = getint(argv[2], 1, MAXBLITBUF - 1);
    if (blitbuff[snum].blitbuffptr == NULL)
        error("Sprite buffer % not loaded", snum);

    RaySprite *sp = &rstate->sprites[id];
    sp->spritenum = snum;
    sp->x = (float)getnumber(argv[4]);
    sp->y = (float)getnumber(argv[6]);
    sp->active = 1;
}

/* RAY MINIMAP x, y, size: the map from above on the write page at (x,y), its longer side 'size' pixels */
static void ray_render_minimap(RayPage *pg, int mx, int my, int size)
{
    if (rstate == NULL || rstate->map == NULL)
        return;

    int hres = pg->w, vres = pg->h;
    int mw = rstate->map_w;
    int mh = rstate->map_h;
    uint8_t *map = rstate->map;
    uint32_t black = RayIndex(0), yellow = RayIndex(14), myrtle = RayIndex(2), sprite = RayIndex(9), white = RayIndex(15);
    RayColours();

    /* Scale so the longest map axis fits 'size' pixels */
    int view_w, view_h;
    if (mw >= mh)
    {
        view_w = size;
        view_h = (size * mh + mw - 1) / mw; /* round up */
    }
    else
    {
        view_h = size;
        view_w = (size * mw + mh - 1) / mh;
    }
    if (view_w < 1)
        view_w = 1;
    if (view_h < 1)
        view_h = 1;

    /* Draw map cells: walls = their fg, empty = BLACK(0) */
    for (int dy = 0; dy < view_h; dy++)
    {
        int cell_y = dy * mh / view_h;
        int py = my + dy;
        if (py < 0 || py >= vres)
            continue;
        for (int dx = 0; dx < view_w; dx++)
        {
            int cell_x = dx * mw / view_w;
            int px = mx + dx;
            if (px < 0 || px >= hres)
                continue;

            uint32_t col;
            if (cell_x < 0 || cell_x >= mw || cell_y < 0 || cell_y >= mh)
                col = black;
            else
            {
                uint8_t wt = map[cell_y * mw + cell_x];
                if (wt == 0)
                    col = black; /* empty = BLACK */
                else if (rstate->walldefs[wt].is_door)
                {
                    float doff = ray_get_door_offset(cell_x, cell_y);
                    if (doff >= 1.0f)
                        col = black; /* fully open door = BLACK */
                    else if (doff > 0.0f)
                        col = yellow; /* partially open = YELLOW */
                    else
                        col = RayWallColour[wt].fg; /* closed door */
                }
                else
                    col = RayWallColour[wt].fg; /* wall = definition fg */
            }

            RayPixel(pg, px, py, col);
        }
    }

    /* Border — MYRTLE(2) */
    for (int i = 0; i < view_w; i++)
    {
        int bx = mx + i;
        if (bx >= 0 && bx < hres)
        {
            if (my >= 0 && my < vres)
                RayPixel(pg, bx, my, myrtle);
            int by = my + view_h - 1;
            if (by >= 0 && by < vres)
                RayPixel(pg, bx, by, myrtle);
        }
    }
    for (int i = 0; i < view_h; i++)
    {
        int by = my + i;
        if (by >= 0 && by < vres)
        {
            if (mx >= 0 && mx < hres)
                RayPixel(pg, mx, by, myrtle);
            int bx = mx + view_w - 1;
            if (bx >= 0 && bx < hres)
                RayPixel(pg, bx, by, myrtle);
        }
    }

    /* Active sprites shown as dots (colour 9) */
    for (int i = 0; i < RAY_MAX_SPRITES; i++)
    {
        if (!rstate->sprites[i].active)
            continue;
        int sx = mx + (int)(rstate->sprites[i].x * view_w / mw);
        int sy = my + (int)(rstate->sprites[i].y * view_h / mh);
        if (sx >= mx && sx < mx + view_w && sy >= my && sy < my + view_h &&
            sx >= 0 && sx < hres && sy >= 0 && sy < vres)
            RayPixel(pg, sx, sy, sprite);
    }

    /* Player dot (WHITE=15) + direction indicator */
    int ppx = mx + (int)(rstate->cam_x * view_w / mw);
    int ppy = my + (int)(rstate->cam_y * view_h / mh);
    if (ppx >= 0 && ppx < hres && ppy >= 0 && ppy < vres)
        RayPixel(pg, ppx, ppy, white);
    float arad = rstate->cam_angle * (F_PI / 180.0f);
    for (int i = 1; i <= 2; i++)
    {
        int fx = ppx + (int)(cosf(arad) * (float)i);
        int fy = ppy + (int)(sinf(arad) * (float)i);
        if (fx >= mx && fx < mx + view_w && fy >= my && fy < my + view_h &&
            fx >= 0 && fx < hres && fy >= 0 && fy < vres)
            RayPixel(pg, fx, fy, white);
    }

    /* its rows copied where the page doubles them */
    int y1 = my < 0 ? 0 : my, y2 = my + view_h - 1 >= vres ? vres - 1 : my + view_h - 1;
    RayDouble(pg, y1, y2);
}

static void ray_cmd_minimap(char *p)
{
    RayPage pg;
    getargs(&p, 5, ",");
    if (argc < 5)
        error("Expected: RAY MINIMAP x, y, size");

    if (rstate == NULL)
        error("Raycaster not initialised");
    RayPageGet(&pg);

    int mx = getint(argv[0], 0, pg.w - 1);
    int my = getint(argv[2], 0, pg.h - 1);
    int sz = getint(argv[4], 4, 256);

    int hidden = RayHidePointer();
    ray_render_minimap(&pg, mx, my, sz);
    if (hidden)
        showcursor(0, xcursor, ycursor);
}

/* RAY DOOR x, y, offset! | RAY DOOR CLOSE x, y | RAY DOOR CLEAR: a sliding door, 0.0 closed to 1.0 open */
static void ray_cmd_door(char *p)
{
    char *q;
    if ((q = checkstring(p, "CLEAR")))
    {
        if (rstate)
            memset(rstate->doors, 0, sizeof(rstate->doors));
        return;
    }
    if ((q = checkstring(p, "CLOSE")))
    {
        getargs(&q, 3, ",");
        if (argc < 3)
            error("Expected: RAY DOOR CLOSE x, y");
        if (rstate == NULL || rstate->map == NULL)
            error("No map defined");
        int x = getint(argv[0], 0, rstate->map_w - 1);
        int y = getint(argv[2], 0, rstate->map_h - 1);
        for (int i = 0; i < RAY_MAX_DOORS; i++)
        {
            if (rstate->doors[i].active &&
                rstate->doors[i].map_x == x &&
                rstate->doors[i].map_y == y)
            {
                rstate->doors[i].active = 0;
                return;
            }
        }
        return; /* not found, silently ignore */
    }

    getargs(&p, 5, ",");
    if (argc < 5)
        error("Expected: RAY DOOR x, y, offset");
    if (rstate == NULL || rstate->map == NULL)
        error("No map defined");

    int x = getint(argv[0], 0, rstate->map_w - 1);
    int y = getint(argv[2], 0, rstate->map_h - 1);
    float offset = (float)getnumber(argv[4]);
    if (offset < 0.0f)
        offset = 0.0f;
    if (offset > 1.0f)
        offset = 1.0f;

    /* Look for existing slot */
    for (int i = 0; i < RAY_MAX_DOORS; i++)
    {
        if (rstate->doors[i].active &&
            rstate->doors[i].map_x == x &&
            rstate->doors[i].map_y == y)
        {
            rstate->doors[i].offset = offset;
            return;
        }
    }

    /* Allocate new slot */
    for (int i = 0; i < RAY_MAX_DOORS; i++)
    {
        if (!rstate->doors[i].active)
        {
            rstate->doors[i].map_x = x;
            rstate->doors[i].map_y = y;
            rstate->doors[i].offset = offset;
            rstate->doors[i].active = 1;
            return;
        }
    }
    error("Maximum % active doors exceeded", RAY_MAX_DOORS);
}

/* Command dispatcher: RAY <subcommand> ... */
void cmd_ray(void)
{
    char *p;

    if ((p = checkstring(cmdline, "MAP")))
        ray_cmd_map(p);
    else if ((p = checkstring(cmdline, "CAMERA")))
        ray_cmd_camera(p);
    else if ((p = checkstring(cmdline, "COLOUR")))
        ray_cmd_colour(p);
    else if ((p = checkstring(cmdline, "COLOR")))
        ray_cmd_colour(p); /* American spelling alias */
    else if ((p = checkstring(cmdline, "DEFINE")))
        ray_cmd_define(p);
    else if ((p = checkstring(cmdline, "MOVE")))
        ray_cmd_move(p);
    else if ((p = checkstring(cmdline, "TURN")))
        ray_cmd_turn(p);
    else if ((p = checkstring(cmdline, "CELL")))
        ray_cmd_cell(p);
    else if ((p = checkstring(cmdline, "CAST")))
        ray_cmd_cast(p);
    else if ((p = checkstring(cmdline, "DOOR")))
        ray_cmd_door(p);
    else if ((p = checkstring(cmdline, "SPRITE")))
        ray_cmd_sprite(p);
    else if ((p = checkstring(cmdline, "MINIMAP")))
        ray_cmd_minimap(p);
    else if ((p = checkstring(cmdline, "RENDER")))
        ray_cmd_render(p);
    else if ((p = checkstring(cmdline, "CLOSE")))
        ray_close();
    else
        SyntaxError();
}

/* Function dispatcher: RAY( <query> ), its arguments at ep (fun_sprite's: the tokeniser writes RAY( as SPRITE(RAY,) */
void fun_ray(void)
{
    char *p;

    if ((p = checkstring(ep, "MAPW")))
    {
        iret = rstate ? rstate->map_w : 0;
    }
    else if ((p = checkstring(ep, "MAPH")))
    {
        iret = rstate ? rstate->map_h : 0;
    }
    else if ((p = checkstring(ep, "CAMX")))
    {
        fret = rstate ? (MMFLOAT)rstate->cam_x : 0.0;
        targ = T_NBR;
        return;
    }
    else if ((p = checkstring(ep, "CAMY")))
    {
        fret = rstate ? (MMFLOAT)rstate->cam_y : 0.0;
        targ = T_NBR;
        return;
    }
    else if ((p = checkstring(ep, "CAMA")))
    {
        fret = rstate ? (MMFLOAT)rstate->cam_angle : 0.0;
        targ = T_NBR;
        return;
    }
    else if ((p = checkstring(ep, "DIST")))
    {
        getargs(&p, 1, ",");
        int col = getint(argv[0], 0, (rstate && rstate->num_cols > 0) ? rstate->num_cols - 1 : 0);
        fret = (rstate && rstate->col_dist) ? (MMFLOAT)rstate->col_dist[col] : 0.0;
        targ = T_NBR;
        return;
    }
    else if ((p = checkstring(ep, "WALL")))
    {
        getargs(&p, 1, ",");
        int col = getint(argv[0], 0, (rstate && rstate->num_cols > 0) ? rstate->num_cols - 1 : 0);
        iret = (rstate && rstate->col_wall) ? rstate->col_wall[col] : 0;
    }
    else if ((p = checkstring(ep, "CELL")))
    {
        getargs(&p, 3, ",");
        if (argc < 3)
            error("Expected: RAY(CELL x, y)");
        if (rstate == NULL || rstate->map == NULL)
        {
            iret = 0;
        }
        else
        {
            int x = getint(argv[0], 0, rstate->map_w - 1);
            int y = getint(argv[2], 0, rstate->map_h - 1);
            iret = rstate->map[y * rstate->map_w + x];
        }
    }
    else if ((p = checkstring(ep, "CASTDIST")))
    {
        fret = rstate ? (MMFLOAT)rstate->cast_dist : 0.0;
        targ = T_NBR;
        return;
    }
    else if ((p = checkstring(ep, "CASTWALL")))
    {
        iret = rstate ? rstate->cast_wall : 0;
    }
    else if ((p = checkstring(ep, "CASTSIDE")))
    {
        iret = rstate ? rstate->cast_side : 0;
    }
    else if ((p = checkstring(ep, "CASTX")))
    {
        iret = rstate ? rstate->cast_mapx : 0;
    }
    else if ((p = checkstring(ep, "CASTY")))
    {
        iret = rstate ? rstate->cast_mapy : 0;
    }
    else if ((p = checkstring(ep, "DOOR")))
    {
        getargs(&p, 3, ",");
        if (argc < 3)
            error("Expected: RAY(DOOR x, y)");
        if (rstate == NULL || rstate->map == NULL)
        {
            fret = -1.0;
        }
        else
        {
            int x = getint(argv[0], 0, rstate->map_w - 1);
            int y = getint(argv[2], 0, rstate->map_h - 1);
            int found = 0;
            for (int i = 0; i < RAY_MAX_DOORS; i++)
            {
                if (rstate->doors[i].active &&
                    rstate->doors[i].map_x == x &&
                    rstate->doors[i].map_y == y)
                {
                    fret = (MMFLOAT)rstate->doors[i].offset;
                    found = 1;
                    break;
                }
            }
            if (!found)
                fret = -1.0;
        }
        targ = T_NBR;
        return;
    }
    else if ((p = checkstring(ep, "DEFINE")))
    {
        getargs(&p, 3, ",");
        if (argc < 3)
            error("Expected: RAY(DEFINE type, property)");
        int wtype = ray_get_walltype(argv[0], 1, RAY_MAX_WALLDEFS - 1);
        int prop = getint(argv[2], 0, 3);
        if (rstate == NULL)
        {
            iret = 0;
        }
        else
        {
            switch (prop)
            {
            case 0:
                iret = rstate->walldefs[wtype].fg;
                break;
            case 1:
                iret = rstate->walldefs[wtype].bg;
                break;
            case 2:
                iret = rstate->walldefs[wtype].pattern;
                break;
            case 3:
                iret = rstate->walldefs[wtype].is_door;
                break;
            default:
                iret = 0;
                break;
            }
        }
    }
    else if ((p = checkstring(ep, "SPRITES")))
    {
        int cnt = 0;
        if (rstate)
        {
            for (int i = 0; i < RAY_MAX_SPRITES; i++)
                if (rstate->sprites[i].active)
                    cnt++;
        }
        iret = cnt;
    }
    else if ((p = checkstring(ep, "SPRITEX")))
    {
        getargs(&p, 1, ",");
        int id = getint(argv[0], 0, RAY_MAX_SPRITES - 1);
        fret = (rstate && rstate->sprites[id].active) ? (MMFLOAT)rstate->sprites[id].x : 0.0;
        targ = T_NBR;
        return;
    }
    else if ((p = checkstring(ep, "SPRITEY")))
    {
        getargs(&p, 1, ",");
        int id = getint(argv[0], 0, RAY_MAX_SPRITES - 1);
        fret = (rstate && rstate->sprites[id].active) ? (MMFLOAT)rstate->sprites[id].y : 0.0;
        targ = T_NBR;
        return;
    }
    else
    {
        SyntaxError();
    }
    targ = T_INT;
}
