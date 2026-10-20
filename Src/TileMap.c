/***********************************************************************************************************************
PicoMite MMBasic

TileMap.c

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
/**
 * @file TileMap.c
 * @author Geoff Graham, Peter Mather
 * @brief Source for the TILEMAP MMBasic command and function (tile map
 *        engine with flash-resident tilesets) and its lightweight
 *        tile-based sprite subsystem. Split out of Draw.c.
 */
/*
 * @cond
 * The following section will be excluded from the documentation.
 */

#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Memory.h"
#include "ImageSlots.h"

/* CMM2 (18 Oct, for Picanoid and Exile): the PicoMite's TILEMAP, its arguments and messages unchanged. A tileset is an
 * image slot (ImageSlots.c: FLASH LOAD IMAGE into slots 4-8) and the tiles are drawn by ImageSlotBlit into a page: the
 * destination letters are the PicoMite's framebuffers (Rgb121Letter): N page 0, L page 1, F page 2, 2 page 3, T page
 * 4, which in a 4-bit mode (MODE n,4: Rgb121.c) are its framebuffers themselves, one not created being N; or a page
 * number. */

// #ifndef PICOMITEMIN
/* ============================================================================
 * TILEMAP subsystem
 *
 * Provides hardware-accelerated tile map rendering using flash-resident
 * tilesets (loaded via FLASH LOAD IMAGE) and compact internal map storage.
 * Map data is read from DATA statements (TILEMAP CREATE) or from a text
 * file (TILEMAP LOAD) into an internal uint16_t array.
 * Tile attributes are stored separately via TILEMAP ATTR.
 * ============================================================================ */

#define MAX_TILEMAPS 4

typedef struct
{
    uint8_t *flash_data; // Pointer to pixel data in flash (past 8-byte header)
    uint16_t *map;       // Heap-allocated map (cols * rows uint16_t values)
    uint16_t *attrs;     // Heap-allocated tile attribute table (indexed by tile number)
    int num_attrs;       // Number of entries in attribute table
    int flash_w;         // Tileset image width in pixels
    int flash_h;         // Tileset image height in pixels
    int tile_w;          // Single tile width in pixels
    int tile_h;          // Single tile height in pixels
    int tiles_per_row;   // Number of tiles across the tileset image
    int map_cols;        // Number of columns in the map
    int map_rows;        // Number of rows in the map
    int view_x;          // Current viewport X offset in world pixels
    int view_y;          // Current viewport Y offset in world pixels
    bool active;         // Whether this slot is in use
} tilemap_t;

static tilemap_t tilemaps[MAX_TILEMAPS] = {0};

/* ============================================================================
 * SPRITE subsystem — lightweight tile-based sprites that reference a tilemap's
 * tileset.  Up to MAX_SPRITES active simultaneously.  Each sprite is positioned
 * at arbitrary pixel coordinates and rendered in one batch via SPRITE DRAW.
 * ============================================================================ */

#define MAX_SPRITES 64

typedef struct
{
    int16_t x;           // Screen X in pixels
    int16_t y;           // Screen Y in pixels
    uint16_t tile;       // Tile index in the parent tileset (1-based, 0 = inactive)
    uint8_t tilemap_ref; // Which tilemap slot (0-based index into tilemaps[])
    bool active;         // Whether this sprite is in use
} sprite_t;

static sprite_t sprites[MAX_SPRITES] = {0};

/* Forward declaration — defined further below, but needed by sprite_cmd_draw */
static int tilemap_get_dest(char *token);

/* ---- SPRITE CREATE id, tilemapRef, tileIndex, x, y ---- */
static void sprite_cmd_create(char *p)
{
    getargs(&p, 9, ",");
    if (argc != 9)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_SPRITES) - 1;
    int tmref = getint(argv[2], 1, MAX_TILEMAPS) - 1;
    int tile = getint(argv[4], 1, 65535);
    int x = getinteger(argv[6]);
    int y = getinteger(argv[8]);

    tilemap_t *tm = &tilemaps[tmref];
    if (!tm->active)
        error("Tilemap % not created", tmref + 1);

    sprite_t *sp = &sprites[id];
    sp->x = (int16_t)x;
    sp->y = (int16_t)y;
    sp->tile = (uint16_t)tile;
    sp->tilemap_ref = (uint8_t)tmref;
    sp->active = true;
}

/* ---- SPRITE MOVE id, x, y ---- */
static void sprite_cmd_move(char *p)
{
    getargs(&p, 5, ",");
    if (argc != 5)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_SPRITES) - 1;
    sprite_t *sp = &sprites[id];
    if (!sp->active)
        error("Sprite % not created", id + 1);

    sp->x = (int16_t)getinteger(argv[2]);
    sp->y = (int16_t)getinteger(argv[4]);
}

/* ---- SPRITE SET id, tileIndex ---- */
static void sprite_cmd_set(char *p)
{
    getargs(&p, 3, ",");
    if (argc != 3)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_SPRITES) - 1;
    sprite_t *sp = &sprites[id];
    if (!sp->active)
        error("Sprite % not created", id + 1);

    sp->tile = (uint16_t)getint(argv[2], 1, 65535);
}

/* ---- SPRITE DRAW dest, transparent ---- */
static void sprite_cmd_draw(char *p)
{
    getargs(&p, 3, ",");
    if (argc != 3)
        error("Syntax");

    int dest = tilemap_get_dest(argv[0]);
    int transparent = getint(argv[2], -1, 15);

    for (int i = 0; i < MAX_SPRITES; i++)
    {
        sprite_t *sp = &sprites[i];
        if (!sp->active || sp->tile == 0)
            continue;

        tilemap_t *tm = &tilemaps[sp->tilemap_ref];
        if (!tm->active)
            continue;

        int tw = tm->tile_w;
        int th = tm->tile_h;

        /* Convert tile index to source rectangle in the tileset */
        int src_col = (sp->tile - 1) % tm->tiles_per_row;
        int src_row = (sp->tile - 1) / tm->tiles_per_row;
        int src_x = src_col * tw;
        int src_y = src_row * th;

        ImageSlotBlit(tm->flash_data, tm->flash_w, tm->flash_h, src_x, src_y, tw, th, dest, sp->x, sp->y, transparent);
    }
}

/* ---- SPRITE DESTROY id ---- */
static void sprite_cmd_destroy(char *p)
{
    getargs(&p, 1, ",");
    if (argc != 1)
        error("Syntax");
    int id = getint(argv[0], 1, MAX_SPRITES) - 1;
    memset(&sprites[id], 0, sizeof(sprite_t));
}

/* ---- SPRITE CLOSE ---- */
static void sprite_closeall(void)
{
    memset(sprites, 0, sizeof(sprites));
}

/* Read 'count' integer values from DATA statements starting at 'label' into
 * a uint16_t array allocated via GetMemory.  Saves and restores the global
 * NextDataLine / NextData state so the user's own READ position is not
 * disturbed. */
static uint16_t *tilemap_read_data(char *label, int count)
{
    /* Save the user's DATA read position */
    char *save_line = NextDataLine;
    int save_data = NextData;

    /* Point to the label */
    NextDataLine = findlabel(label);
    NextData = 0;

    uint16_t *buf = (uint16_t *)GetMemory(count * sizeof(uint16_t));

    int datatoken = GetCommandValue("Data");
    char *p = NextDataLine;
    int idx = 0;

    while (idx < count)
    {
        /* Scan for next DATA statement */
        while (1)
        {
            if (*p == 0)
                p++;
            if (*p == 0)
            {
                FreeMemory((void *)buf);
                NextDataLine = save_line;
                NextData = save_data;
                error("Not enough DATA for tilemap (need %, found %)", count, idx);
            }
            if (*p == T_NEWLINE)
                p += T_NEWLINE_HDR; // skip newline + skip byte
            if (*p == T_LINENBR)
                p += 3;
            skipspace(p);
            if (*p == T_LABEL)
            {
                p += p[1] + 2;
                skipspace(p);
            }
            CommandToken tkn = commandtbl_at(p);
            if (tkn == datatoken)
                break;
            while (*p)
                p++;
        }
        p += sizeof(CommandToken);
        skipspace(p);

        /* Parse comma-separated values on this DATA line */
        {
            getargs(&p, (MAX_ARG_COUNT * 2) - 1, ",");
            int di = 0;
            while (di < argc && idx < count)
            {
                buf[idx++] = (uint16_t)getinteger(argv[di]);
                di += 2; /* skip comma */
            }
        }
    }

    /* Restore the user's DATA read position */
    NextDataLine = save_line;
    NextData = save_data;
    return buf;
}

/* A destination: one of the PicoMite's framebuffer letters (Rgb121Letter) or a page number */
static int tilemap_get_dest(char *token)
{
    int page = Rgb121Letter(token);
    return page >= 0 ? page : getint(token, 0, LastPage);
}

/* The image in 'slot' (loaded by FLASH LOAD IMAGE): its header's address (tilemap_install steps over it), width and
   height; an error if the slot is empty */
static uint8_t *tilemap_flash_image(int slot, int *fw, int *fh)
{
    return ImageSlotPixels(slot, fw, fh, "No image in slot %") - 8;
}

/* Free whatever a tilemap slot holds and mark it inactive */
static void tilemap_release(tilemap_t *tm)
{
    if (tm->map)
        FreeMemory((void *)tm->map);
    if (tm->attrs)
        FreeMemory((void *)tm->attrs);
    memset(tm, 0, sizeof(tilemap_t));
}

/* Point a released tilemap slot at its tileset and map and activate it */
static void tilemap_install(tilemap_t *tm, uint8_t *base, int fw, int fh, int tw, int th, int tpr, int cols, int rows, uint16_t *map)
{
    tm->map = map;
    tm->flash_data = base + 8; /* skip 8-byte header (width + height) */
    tm->flash_w = fw;
    tm->flash_h = fh;
    tm->tile_w = tw;
    tm->tile_h = th;
    tm->tiles_per_row = tpr;
    tm->map_cols = cols;
    tm->map_rows = rows;
    tm->view_x = 0;
    tm->view_y = 0;
    tm->active = true;
}

/* ---- TILEMAP CREATE mapLabel, id, flashSlot, tileW, tileH, tilesPerRow, cols, rows ---- */
static void tilemap_cmd_create(char *p)
{
    /* Parse label name first (before getcsargs tokenises it) */
    skipspace(p);
    if (!isnamestartsym(*p))
        error("Expected label");
    char *label = p;
    if (issymbol(*p))
        p += symbolsize(*p); /* (a label in a saved program may be a symbol: Symbols.h) */
    else
        while (isnamechar(*p))
            p++;
    skipspace(p);
    if (*p != ',')
        error("Syntax");
    p++; /* skip comma */

    getargs(&p, 13, ",");
    if (argc != 13)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
    int slot = getint(argv[2], 1, MAXIMAGESLOTS);
    int tw = getint(argv[4], 1, 256);
    int th = getint(argv[6], 1, 256);
    int tpr = getint(argv[8], 1, 1024);
    int cols = getint(argv[10], 1, 10000);
    int rows = getint(argv[12], 1, 10000);

    int fw, fh;
    uint8_t *base = tilemap_flash_image(slot, &fw, &fh);

    /* Free previous allocation if re-creating, then read the map from DATA */
    tilemap_t *tm = &tilemaps[id];
    tilemap_release(tm);
    uint16_t *map = tilemap_read_data(label, cols * rows);
    tilemap_install(tm, base, fw, fh, tw, th, tpr, cols, rows, map);
}

/* Read the next unsigned decimal value from an open text file.  Values are
 * separated by commas, spaces, tabs or line ends, and anything from a quote
 * or a hash to the end of the line is a comment.  Returns 1 with the value,
 * 0 at the end of the file, -1 on a character that is neither a digit nor a
 * separator, -2 on a value above 65535. */
#define TILEMAP_FILE_SEP(c) ((c) == ' ' || (c) == ',' || (c) == '\t' || (c) == '\r' || (c) == '\n' || (c) == 0 || (c) == 0x1a)
#define TILEMAP_FILE_REM(c) ((c) == '\'' || (c) == '#')
static int tilemap_file_value(int fnbr, int *value)
{
    int c;
    for (;;)
    {
        if (FileEOF(fnbr))
            return 0;
        c = FileGetChar(fnbr) & 0xff;
        if (TILEMAP_FILE_REM(c))
        {
            while (!FileEOF(fnbr) && (FileGetChar(fnbr) & 0xff) != '\n')
                ;
            continue;
        }
        if (TILEMAP_FILE_SEP(c))
            continue;
        if (c >= '0' && c <= '9')
            break;
        return -1;
    }
    int v = 0;
    for (;;)
    {
        v = v * 10 + (c - '0');
        if (v > 65535)
            return -2;
        if (FileEOF(fnbr))
            break;
        c = FileGetChar(fnbr) & 0xff;
        if (c >= '0' && c <= '9')
            continue;
        if (TILEMAP_FILE_REM(c))
        {
            while (!FileEOF(fnbr) && (FileGetChar(fnbr) & 0xff) != '\n')
                ;
            break;
        }
        if (TILEMAP_FILE_SEP(c))
            break;
        return -1;
    }
    *value = v;
    return 1;
}

/* ---- TILEMAP LOAD file$, id, flashSlot, tileW, tileH, tilesPerRow ----
 * As CREATE, but the map comes from a text file: the width and height
 * first, then width * height tile numbers, row by row. */
static void tilemap_cmd_load(char *p)
{
    getargs(&p, 11, ",");
    if (argc != 11)
        error("Syntax");

    char *fname = (char *)getFstring(argv[0]);
    int id = getint(argv[2], 1, MAX_TILEMAPS) - 1;
    int slot = getint(argv[4], 1, MAXIMAGESLOTS);
    int tw = getint(argv[6], 1, 256);
    int th = getint(argv[8], 1, 256);
    int tpr = getint(argv[10], 1, 1024);

    int fw, fh;
    uint8_t *base = tilemap_flash_image(slot, &fw, &fh);

    if (strchr(&fname[strlen(fname) > 4 ? strlen(fname) - 4 : 0], '.') == NULL)
        strcat(fname, ".map"); /* (the CMM2 has no AppendDefaultExtension) */
    if (!InitSDCard())
        return;
    int fnbr = FindFreeFileNbr();
    if (!BasicFileOpen(fname, fnbr, FA_READ))
        return;

    /* The file starts with the map size */
    int cols = 0, rows = 0;
    int r = tilemap_file_value(fnbr, &cols);
    if (r == 1)
        r = tilemap_file_value(fnbr, &rows);
    if (r != 1 || cols < 1 || cols > 10000 || rows < 1 || rows > 10000)
    {
        FileClose(fnbr);
        if (r == -1)
            error("Invalid character in tilemap file");
        error("Tilemap file must start with width and height (1-10000)");
    }

    /* Then cols * rows tile numbers */
    int count = cols * rows;
    uint16_t *map = (uint16_t *)GetMemory(count * sizeof(uint16_t));
    int idx = 0, v = 0;
    while (idx < count)
    {
        r = tilemap_file_value(fnbr, &v);
        if (r != 1)
            break;
        map[idx++] = (uint16_t)v;
        if ((idx & 255) == 0)
            routinechecks(1);
    }
    FileClose(fnbr);
    if (r < 0 || idx < count)
    {
        FreeMemory((void *)map);
        if (r == -1)
            error("Invalid character in tilemap file");
        if (r == -2)
            error("Value out of range in tilemap file");
        error("Not enough data in tilemap file (need %, found %)", count, idx);
    }

    /* Only now, with the whole map read, replace whatever the slot held */
    tilemap_t *tm = &tilemaps[id];
    tilemap_release(tm);
    tilemap_install(tm, base, fw, fh, tw, th, tpr, cols, rows, map);
}

/* ---- TILEMAP ATTR id, numTiles, attrLabel ---- */
static void tilemap_cmd_attr(char *p)
{
    /* Parse label name first (before getcsargs tokenises it) */
    skipspace(p);
    if (!isnamestartsym(*p))
        error("Expected label");
    char *label = p;
    if (issymbol(*p))
        p += symbolsize(*p); /* (a label in a saved program may be a symbol: Symbols.h) */
    else
        while (isnamechar(*p))
            p++;
    skipspace(p);
    if (*p != ',')
        error("Syntax");
    p++; /* skip comma */

    getargs(&p, 3, ",");
    if (argc != 3)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
    tilemap_t *tm = &tilemaps[id];
    if (!tm->active)
        error("Tilemap % not created", id + 1);

    int num = getint(argv[2], 1, 65535);

    /* Free previous attribute table if any */
    if (tm->attrs)
        FreeMemory((void *)tm->attrs);

    tm->attrs = tilemap_read_data(label, num);
    tm->num_attrs = num;
}

/* ---- TILEMAP DESTROY id ---- */
static void tilemap_cmd_destroy(char *p)
{
    getargs(&p, 1, ",");
    if (argc != 1)
        error("Syntax");
    int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
    tilemap_t *tm = &tilemaps[id];
    if (tm->map)
        FreeMemory((void *)tm->map);
    if (tm->attrs)
        FreeMemory((void *)tm->attrs);
    memset(tm, 0, sizeof(tilemap_t));
}

/* Close all tilemaps (called on program end / NEW) */
/* The heap was wiped (HeapWipe: RUN, NEW, a program load): the maps went with it */
/* Does a tilemap draw from the slot image at img (ImageSlots.c, before it frees a slot's block)? */
int TileMapUses(const uint8_t *img)
{
    for (int i = 0; i < MAX_TILEMAPS; i++)
        if (tilemaps[i].active && tilemaps[i].flash_data == img + 8)
            return 1;
    return 0;
}

void TileMapForget(void)
{
    memset(tilemaps, 0, sizeof(tilemaps));
    memset(sprites, 0, sizeof(sprites));
}

void tilemap_closeall(void)
{
    for (int i = 0; i < MAX_TILEMAPS; i++)
    {
        if (tilemaps[i].map)
            FreeMemory((void *)tilemaps[i].map);
        if (tilemaps[i].attrs)
            FreeMemory((void *)tilemaps[i].attrs);
    }
    memset(tilemaps, 0, sizeof(tilemaps));
}

/* ---- TILEMAP SET id, col, row, tileIndex ---- */
static void tilemap_cmd_set(char *p)
{
    getargs(&p, 7, ",");
    if (argc != 7)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
    tilemap_t *tm = &tilemaps[id];
    if (!tm->active)
        error("Tilemap % not created", id + 1);

    int col = getint(argv[2], 0, tm->map_cols - 1);
    int row = getint(argv[4], 0, tm->map_rows - 1);
    int tile = getint(argv[6], 0, 65535);

    tm->map[col + row * tm->map_cols] = (uint16_t)tile;
}

/* ---- TILEMAP DRAW id, dest, viewX, viewY, screenX, screenY, viewW, viewH [, transparent] ---- */
static void tilemap_cmd_draw(char *p)
{
    getargs(&p, 17, ",");
    if (!(argc == 15 || argc == 17))
        error("Syntax");

    int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
    tilemap_t *tm = &tilemaps[id];
    if (!tm->active)
        error("Tilemap % not created", id + 1);

    int dest = tilemap_get_dest(argv[2]);

    int vx = getinteger(argv[4]);
    int vy = getinteger(argv[6]);
    int sx = getinteger(argv[8]);
    int sy = getinteger(argv[10]);
    int vw = getint(argv[12], 1, 3840);
    int vh = getint(argv[14], 1, 2160);
    int transparent = -1;
    if (argc == 17)
        transparent = getint(argv[16], -1, 15);

    /* Store viewport position */
    tm->view_x = vx;
    tm->view_y = vy;

    int tw = tm->tile_w;
    int th = tm->tile_h;

    /* Calculate visible tile range */
    int col_start = vx / tw;
    int row_start = vy / th;
    int col_end = (vx + vw - 1) / tw;
    int row_end = (vy + vh - 1) / th;

    /* Sub-tile pixel offset for smooth scrolling */
    int offset_x = vx % tw;
    int offset_y = vy % th;

    for (int r = row_start; r <= row_end; r++)
    {
        for (int c = col_start; c <= col_end; c++)
        {
            /* Skip out-of-bounds map cells */
            if (c < 0 || c >= tm->map_cols || r < 0 || r >= tm->map_rows)
                continue;

            int tile = (int)tm->map[c + r * tm->map_cols];

            /* Tile 0 = empty, skip */
            if (tile == 0)
                continue;

            /* Convert tile index to source rectangle in the tileset */
            int src_col = (tile - 1) % tm->tiles_per_row;
            int src_row = (tile - 1) / tm->tiles_per_row;
            int src_x = src_col * tw;
            int src_y = src_row * th;

            /* Calculate destination position on screen */
            int dst_x = sx + (c - col_start) * tw - offset_x;
            int dst_y = sy + (r - row_start) * th - offset_y;

            /* ImageSlotBlit clips, so tiles partly off the page are passed as they are */
            ImageSlotBlit(tm->flash_data, tm->flash_w, tm->flash_h, src_x, src_y, tw, th, dest, dst_x, dst_y, transparent);
        }
    }
}

/* ---- TILEMAP SCROLL id, dx, dy ---- */
static void tilemap_cmd_scroll(char *p)
{
    getargs(&p, 5, ",");
    if (argc != 5)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
    tilemap_t *tm = &tilemaps[id];
    if (!tm->active)
        error("Tilemap % not created", id + 1);

    int dx = getinteger(argv[2]);
    int dy = getinteger(argv[4]);

    tm->view_x += dx;
    tm->view_y += dy;

    /* Clamp to world bounds */
    int max_x = tm->map_cols * tm->tile_w - HRes;
    int max_y = tm->map_rows * tm->tile_h - VRes;
    if (tm->view_x < 0)
        tm->view_x = 0;
    if (tm->view_y < 0)
        tm->view_y = 0;
    if (max_x > 0 && tm->view_x > max_x)
        tm->view_x = max_x;
    if (max_y > 0 && tm->view_y > max_y)
        tm->view_y = max_y;
}

/* ---- TILEMAP VIEW id, x, y ---- */
static void tilemap_cmd_view(char *p)
{
    getargs(&p, 5, ",");
    if (argc != 5)
        error("Syntax");

    int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
    tilemap_t *tm = &tilemaps[id];
    if (!tm->active)
        error("Tilemap % not created", id + 1);

    tm->view_x = getinteger(argv[2]);
    tm->view_y = getinteger(argv[4]);
}

/* ==== Command dispatcher: TILEMAP <subcommand> ==== */
void MIPS16 cmd_tilemap(void)
{
    char *p;

    if ((p = checkstring(cmdline, "CREATE")))
    {
        tilemap_cmd_create(p);
    }
    else if ((p = checkstring(cmdline, "LOAD")))
    {
        tilemap_cmd_load(p);
    }
    else if ((p = checkstring(cmdline, "ATTR")))
    {
        tilemap_cmd_attr(p);
    }
    else if ((p = checkstring(cmdline, "DESTROY")))
    {
        tilemap_cmd_destroy(p);
    }
    else if ((p = checkstring(cmdline, "SET")))
    {
        tilemap_cmd_set(p);
    }
    else if ((p = checkstring(cmdline, "DRAW")))
    {
        tilemap_cmd_draw(p);
    }
    else if ((p = checkstring(cmdline, "SCROLL")))
    {
        tilemap_cmd_scroll(p);
    }
    else if ((p = checkstring(cmdline, "VIEW")))
    {
        tilemap_cmd_view(p);
    }
    else if ((p = checkstring(cmdline, "CLOSE")))
    {
        tilemap_closeall();
        sprite_closeall();
    }
    else if ((p = checkstring(cmdline, "SPRITE")))
    {
        char *q;
        if ((q = checkstring(p, "CREATE")))
            sprite_cmd_create(q);
        else if ((q = checkstring(p, "MOVE")))
            sprite_cmd_move(q);
        else if ((q = checkstring(p, "SET")))
            sprite_cmd_set(q);
        else if ((q = checkstring(p, "DRAW")))
            sprite_cmd_draw(q);
        else if ((q = checkstring(p, "DESTROY")))
            sprite_cmd_destroy(q);
        else if (checkstring(p, "CLOSE"))
            sprite_closeall();
        else
            error("Syntax");
    }
    else
    {
        error("Syntax");
    }
}

/* ==== Function dispatcher: TILEMAP( <query> ) ==== */
void MIPS16 fun_tilemap(void)
{
    char *p;

    if ((p = checkstring(ep, "TILE")))
    {
        /* TILEMAP(TILE id, pixelX, pixelY) - returns tile index at world pixel coords */
        getargs(&p, 5, ",");
        if (argc != 5)
            error("Syntax");

        int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
        tilemap_t *tm = &tilemaps[id];
        if (!tm->active)
            error("Tilemap % not created", id + 1);

        int px = getinteger(argv[2]);
        int py = getinteger(argv[4]);
        int col = px / tm->tile_w;
        int row = py / tm->tile_h;

        if (col < 0 || col >= tm->map_cols || row < 0 || row >= tm->map_rows)
        {
            iret = 0;
        }
        else
        {
            iret = tm->map[col + row * tm->map_cols];
        }
        targ = T_INT;
    }
    else if ((p = checkstring(ep, "COLLISION")))
    {
        /* TILEMAP(COLLISION id, x, y, w, h [, mask]) - returns first matching tile in bounding box */
        getargs(&p, 11, ",");
        if (!(argc == 9 || argc == 11))
            error("Syntax");

        int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
        tilemap_t *tm = &tilemaps[id];
        if (!tm->active)
            error("Tilemap % not created", id + 1);

        int bx = getinteger(argv[2]);
        int by = getinteger(argv[4]);
        int bw = getinteger(argv[6]);
        int bh = getinteger(argv[8]);
        int mask = 0; /* 0 = no attribute filtering, match any non-zero tile */
        if (argc == 11)
            mask = getinteger(argv[10]);

        int col_start = bx / tm->tile_w;
        int row_start = by / tm->tile_h;
        int col_end = (bx + bw - 1) / tm->tile_w;
        int row_end = (by + bh - 1) / tm->tile_h;

        iret = 0;
        for (int r = row_start; r <= row_end && iret == 0; r++)
        {
            for (int c = col_start; c <= col_end && iret == 0; c++)
            {
                if (c >= 0 && c < tm->map_cols && r >= 0 && r < tm->map_rows)
                {
                    int tile = (int)tm->map[c + r * tm->map_cols];
                    if (tile != 0)
                    {
                        if (mask == 0)
                        {
                            iret = tile; /* no filtering, any non-zero tile matches */
                        }
                        else if (tm->attrs && tile <= tm->num_attrs)
                        {
                            if (tm->attrs[tile - 1] & mask)
                                iret = tile; /* attribute bits match mask */
                        }
                    }
                }
            }
        }
        targ = T_INT;
    }
    else if ((p = checkstring(ep, "ATTR")))
    {
        /* TILEMAP(ATTR id, tileIndex) - returns attribute flags for a tile type */
        getargs(&p, 3, ",");
        if (argc != 3)
            error("Syntax");

        int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
        tilemap_t *tm = &tilemaps[id];
        if (!tm->active)
            error("Tilemap % not created", id + 1);

        int tile = getint(argv[2], 1, 65535);
        if (!tm->attrs || tile > tm->num_attrs)
            iret = 0;
        else
            iret = tm->attrs[tile - 1]; /* 1-based tile index */
        targ = T_INT;
    }
    else if ((p = checkstring(ep, "VIEWX")))
    {
        /* TILEMAP(VIEWX id) - returns current viewport X */
        getargs(&p, 1, ",");
        if (argc != 1)
            error("Syntax");
        int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
        tilemap_t *tm = &tilemaps[id];
        if (!tm->active)
            error("Tilemap % not created", id + 1);
        iret = tm->view_x;
        targ = T_INT;
    }
    else if ((p = checkstring(ep, "VIEWY")))
    {
        /* TILEMAP(VIEWY id) - returns current viewport Y */
        getargs(&p, 1, ",");
        if (argc != 1)
            error("Syntax");
        int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
        tilemap_t *tm = &tilemaps[id];
        if (!tm->active)
            error("Tilemap % not created", id + 1);
        iret = tm->view_y;
        targ = T_INT;
    }
    else if ((p = checkstring(ep, "COLS")))
    {
        /* TILEMAP(COLS id) - returns map column count */
        getargs(&p, 1, ",");
        if (argc != 1)
            error("Syntax");
        int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
        tilemap_t *tm = &tilemaps[id];
        if (!tm->active)
            error("Tilemap % not created", id + 1);
        iret = tm->map_cols;
        targ = T_INT;
    }
    else if ((p = checkstring(ep, "ROWS")))
    {
        /* TILEMAP(ROWS id) - returns map row count */
        getargs(&p, 1, ",");
        if (argc != 1)
            error("Syntax");
        int id = getint(argv[0], 1, MAX_TILEMAPS) - 1;
        tilemap_t *tm = &tilemaps[id];
        if (!tm->active)
            error("Tilemap % not created", id + 1);
        iret = tm->map_rows;
        targ = T_INT;
    }
    else if ((p = checkstring(ep, "SPRITE")))
    {
        char *q;
        if ((q = checkstring(p, "X")))
        {
            /* TILEMAP(SPRITE X id) */
            getargs(&q, 1, ",");
            if (argc != 1)
                error("Syntax");
            int id = getint(argv[0], 1, MAX_SPRITES) - 1;
            if (!sprites[id].active)
                error("Sprite % not created", id + 1);
            iret = sprites[id].x;
            targ = T_INT;
        }
        else if ((q = checkstring(p, "Y")))
        {
            /* TILEMAP(SPRITE Y id) */
            getargs(&q, 1, ",");
            if (argc != 1)
                error("Syntax");
            int id = getint(argv[0], 1, MAX_SPRITES) - 1;
            if (!sprites[id].active)
                error("Sprite % not created", id + 1);
            iret = sprites[id].y;
            targ = T_INT;
        }
        else if ((q = checkstring(p, "TILE")))
        {
            /* TILEMAP(SPRITE TILE id) */
            getargs(&q, 1, ",");
            if (argc != 1)
                error("Syntax");
            int id = getint(argv[0], 1, MAX_SPRITES) - 1;
            if (!sprites[id].active)
                error("Sprite % not created", id + 1);
            iret = sprites[id].tile;
            targ = T_INT;
        }
        else if ((q = checkstring(p, "HIT")))
        {
            /* TILEMAP(SPRITE HIT id1, id2) - bounding-box overlap test */
            getargs(&q, 3, ",");
            if (argc != 3)
                error("Syntax");
            int id1 = getint(argv[0], 1, MAX_SPRITES) - 1;
            int id2 = getint(argv[2], 1, MAX_SPRITES) - 1;
            if (!sprites[id1].active)
                error("Sprite % not created", id1 + 1);
            if (!sprites[id2].active)
                error("Sprite % not created", id2 + 1);

            tilemap_t *tm1 = &tilemaps[sprites[id1].tilemap_ref];
            tilemap_t *tm2 = &tilemaps[sprites[id2].tilemap_ref];
            int w1 = tm1->tile_w, h1 = tm1->tile_h;
            int w2 = tm2->tile_w, h2 = tm2->tile_h;

            int ax = sprites[id1].x, ay = sprites[id1].y;
            int bx = sprites[id2].x, by = sprites[id2].y;

            iret = (ax < bx + w2 && ax + w1 > bx &&
                    ay < by + h2 && ay + h1 > by)
                       ? 1
                       : 0;
            targ = T_INT;
        }
        else if ((q = checkstring(p, "W")))
        {
            /* TILEMAP(SPRITE W id) - tile width from parent tilemap */
            getargs(&q, 1, ",");
            if (argc != 1)
                error("Syntax");
            int id = getint(argv[0], 1, MAX_SPRITES) - 1;
            if (!sprites[id].active)
                error("Sprite % not created", id + 1);
            iret = tilemaps[sprites[id].tilemap_ref].tile_w;
            targ = T_INT;
        }
        else if ((q = checkstring(p, "H")))
        {
            /* TILEMAP(SPRITE H id) - tile height from parent tilemap */
            getargs(&q, 1, ",");
            if (argc != 1)
                error("Syntax");
            int id = getint(argv[0], 1, MAX_SPRITES) - 1;
            if (!sprites[id].active)
                error("Sprite % not created", id + 1);
            iret = tilemaps[sprites[id].tilemap_ref].tile_h;
            targ = T_INT;
        }
        else
        {
            error("Syntax");
        }
    }
    else
    {
        error("Syntax");
    }
}
// #endif

/*  @endcond */
