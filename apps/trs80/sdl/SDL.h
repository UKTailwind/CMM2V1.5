/* SDL.h: the part of SDL 1.2 that sdltrs uses, on the Colour Maximite 2 (sdl_cmm2.c, by the loader's services). The names, the structures' members
   and the key numbers are SDL 1.2's, so that sdltrs's sources are built as they are; there is nothing of SDL's own code here. MIT License, as the
   SDK's files. */
#ifndef SDL_CMM2_H
#define SDL_CMM2_H
#include <stdint.h>
#include <stdio.h>
#include "SDL_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- the start, the time ---- */
#define SDL_INIT_TIMER    0x0001
#define SDL_INIT_AUDIO    0x0010
#define SDL_INIT_VIDEO    0x0020
#define SDL_INIT_JOYSTICK 0x0200
int SDL_Init(Uint32 flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);

/* ---- surfaces ---- */
typedef struct { Sint16 x, y; Uint16 w, h; } SDL_Rect;
typedef struct { Uint8 r, g, b, unused; } SDL_Color;
typedef struct { int ncolors; SDL_Color *colors; } SDL_Palette;
typedef struct {
    SDL_Palette *palette;
    Uint8 BitsPerPixel, BytesPerPixel;
    Uint32 Rmask, Gmask, Bmask, Amask;
} SDL_PixelFormat;
typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    Uint16 pitch;
    void *pixels;
    /* (sdl_cmm2.c's own) */
    int own_pixels;
    Uint8 map[2];                               /* a 1-bit surface's two colours, as the screen's */
} SDL_Surface;

#define SDL_SWSURFACE    0
#define SDL_ANYFORMAT    0x10000000
#define SDL_FULLSCREEN   0x80000000
#define SDL_LOGPAL       1
#define SDL_PHYSPAL      2
#define SDL_ALPHA_OPAQUE 255
#define SDL_MUSTLOCK(s)  0
#define SDL_DISABLE      0
#define SDL_ENABLE       1
typedef enum { SDL_GRAB_OFF = 0, SDL_GRAB_ON = 1 } SDL_GrabMode;

SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags);
SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth, Uint32 r, Uint32 g, Uint32 b, Uint32 a);
SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int w, int h, int depth, int pitch, Uint32 r, Uint32 g, Uint32 b, Uint32 a);
SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags);
void SDL_FreeSurface(SDL_Surface *s);
int SDL_FillRect(SDL_Surface *dst, SDL_Rect *r, Uint32 colour);
int SDL_UpperBlit(SDL_Surface *src, SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr);
#define SDL_BlitSurface SDL_UpperBlit
Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b);
Uint32 SDL_MapRGBA(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colours, int first, int n);
void SDL_UpdateRect(SDL_Surface *s, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
void SDL_UpdateRects(SDL_Surface *s, int n, SDL_Rect *rects);
int SDL_LockSurface(SDL_Surface *s);
void SDL_UnlockSurface(SDL_Surface *s);
int SDL_SaveBMP(SDL_Surface *s, const char *name);
void SDL_WM_SetCaption(const char *title, const char *icon);
SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode);
int SDL_ShowCursor(int toggle);

/* ---- the keys: SDL 1.2's numbers ---- */
typedef int SDLKey;
enum {
    SDLK_UNKNOWN = 0, SDLK_BACKSPACE = 8, SDLK_TAB = 9, SDLK_CLEAR = 12, SDLK_RETURN = 13, SDLK_PAUSE = 19, SDLK_ESCAPE = 27, SDLK_SPACE = 32,
    SDLK_EXCLAIM = 33, SDLK_QUOTEDBL = 34, SDLK_HASH = 35, SDLK_DOLLAR = 36, SDLK_AMPERSAND = 38, SDLK_QUOTE = 39, SDLK_LEFTPAREN = 40,
    SDLK_RIGHTPAREN = 41, SDLK_ASTERISK = 42, SDLK_PLUS = 43, SDLK_COMMA = 44, SDLK_MINUS = 45, SDLK_PERIOD = 46, SDLK_SLASH = 47,
    SDLK_0 = 48, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9,
    SDLK_COLON = 58, SDLK_SEMICOLON = 59, SDLK_LESS = 60, SDLK_EQUALS = 61, SDLK_GREATER = 62, SDLK_QUESTION = 63, SDLK_AT = 64,
    SDLK_LEFTBRACKET = 91, SDLK_BACKSLASH = 92, SDLK_RIGHTBRACKET = 93, SDLK_CARET = 94, SDLK_UNDERSCORE = 95, SDLK_BACKQUOTE = 96,
    SDLK_a = 97, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f, SDLK_g, SDLK_h, SDLK_i, SDLK_j, SDLK_k, SDLK_l, SDLK_m, SDLK_n, SDLK_o, SDLK_p, SDLK_q,
    SDLK_r, SDLK_s, SDLK_t, SDLK_u, SDLK_v, SDLK_w, SDLK_x, SDLK_y, SDLK_z,
    SDLK_DELETE = 127,
    SDLK_KP0 = 256, SDLK_KP1, SDLK_KP2, SDLK_KP3, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7, SDLK_KP8, SDLK_KP9, SDLK_KP_PERIOD, SDLK_KP_DIVIDE,
    SDLK_KP_MULTIPLY, SDLK_KP_MINUS, SDLK_KP_PLUS, SDLK_KP_ENTER, SDLK_KP_EQUALS,
    SDLK_UP = 273, SDLK_DOWN, SDLK_RIGHT, SDLK_LEFT, SDLK_INSERT, SDLK_HOME, SDLK_END, SDLK_PAGEUP, SDLK_PAGEDOWN,
    SDLK_F1 = 282, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14,
    SDLK_F15,
    SDLK_NUMLOCK = 300, SDLK_CAPSLOCK, SDLK_SCROLLOCK, SDLK_RSHIFT, SDLK_LSHIFT, SDLK_RCTRL, SDLK_LCTRL, SDLK_RALT, SDLK_LALT, SDLK_RMETA,
    SDLK_LMETA, SDLK_LSUPER, SDLK_RSUPER, SDLK_MODE, SDLK_COMPOSE, SDLK_HELP, SDLK_PRINT, SDLK_SYSREQ, SDLK_BREAK, SDLK_MENU
};
typedef enum {
    KMOD_NONE = 0, KMOD_LSHIFT = 0x0001, KMOD_RSHIFT = 0x0002, KMOD_LCTRL = 0x0040, KMOD_RCTRL = 0x0080, KMOD_LALT = 0x0100, KMOD_RALT = 0x0200,
    KMOD_LMETA = 0x0400, KMOD_RMETA = 0x0800, KMOD_NUM = 0x1000, KMOD_CAPS = 0x2000, KMOD_MODE = 0x4000
} SDLMod;
#define KMOD_CTRL  (KMOD_LCTRL | KMOD_RCTRL)
#define KMOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define KMOD_ALT   (KMOD_LALT | KMOD_RALT)
#define KMOD_META  (KMOD_LMETA | KMOD_RMETA)
#define SDL_DEFAULT_REPEAT_DELAY    500
#define SDL_DEFAULT_REPEAT_INTERVAL 30

typedef struct SDL_keysym {
    Uint8 scancode;                             /* the key's place: here its HID usage */
    int sym;
    int mod;
    Uint16 unicode;                             /* the character it types (SDL_EnableUNICODE), in the firmware's keyboard layout */
} SDL_keysym;

/* ---- events ---- */
enum {
    SDL_NOEVENT = 0, SDL_ACTIVEEVENT, SDL_KEYDOWN, SDL_KEYUP, SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP, SDL_JOYAXISMOTION,
    SDL_JOYBALLMOTION, SDL_JOYHATMOTION, SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP, SDL_QUIT
};
typedef struct { Uint8 type, which, state; SDL_keysym keysym; } SDL_KeyboardEvent;
typedef struct { Uint8 type, which, state; Uint16 x, y; Sint16 xrel, yrel; } SDL_MouseMotionEvent;
typedef struct { Uint8 type, which, button, state; Uint16 x, y; } SDL_MouseButtonEvent;
typedef struct { Uint8 type, which, axis; Sint16 value; } SDL_JoyAxisEvent;
typedef struct { Uint8 type, which, hat, value; } SDL_JoyHatEvent;
typedef struct { Uint8 type, which, button, state; } SDL_JoyButtonEvent;
typedef union {
    Uint8 type;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_JoyAxisEvent jaxis;
    SDL_JoyHatEvent jhat;
    SDL_JoyButtonEvent jbutton;
} SDL_Event;
int SDL_PollEvent(SDL_Event *e);
int SDL_WaitEvent(SDL_Event *e);
SDLMod SDL_GetModState(void);
int SDL_EnableKeyRepeat(int delay, int interval);
int SDL_EnableUNICODE(int enable);

/* ---- the mouse ---- */
#define SDL_BUTTON(n)         (1 << ((n) - 1))
#define SDL_BUTTON_LEFT       1
#define SDL_BUTTON_MIDDLE     2
#define SDL_BUTTON_RIGHT      3
#define SDL_BUTTON_WHEELUP    4
#define SDL_BUTTON_WHEELDOWN  5
Uint8 SDL_GetMouseState(int *x, int *y);
void SDL_WarpMouse(Uint16 x, Uint16 y);

/* ---- the joysticks: the USB gamepads ---- */
#define SDL_HAT_CENTERED  0x00
#define SDL_HAT_UP        0x01
#define SDL_HAT_RIGHT     0x02
#define SDL_HAT_DOWN      0x04
#define SDL_HAT_LEFT      0x08
#define SDL_HAT_RIGHTUP   (SDL_HAT_RIGHT | SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN (SDL_HAT_RIGHT | SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP    (SDL_HAT_LEFT | SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN  (SDL_HAT_LEFT | SDL_HAT_DOWN)
typedef struct SDL_Joystick SDL_Joystick;
int SDL_NumJoysticks(void);
SDL_Joystick *SDL_JoystickOpen(int index);
void SDL_JoystickClose(SDL_Joystick *j);
const char *SDL_JoystickName(int index);

/* ---- sound ---- */
#define AUDIO_U8     0x0008
#define AUDIO_S16LSB 0x8010
#define AUDIO_S16MSB 0x9010
#define AUDIO_S16    AUDIO_S16LSB
typedef struct SDL_AudioSpec {
    int freq;
    Uint16 format;
    Uint8 channels, silence;
    Uint16 samples, padding;
    Uint32 size;
    void (*callback)(void *userdata, Uint8 *stream, int len);
    void *userdata;
} SDL_AudioSpec;
int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
void SDL_CloseAudio(void);
void SDL_PauseAudio(int on);
void SDL_LockAudio(void);
void SDL_UnlockAudio(void);

#ifdef __cplusplus
}
#endif
#endif
