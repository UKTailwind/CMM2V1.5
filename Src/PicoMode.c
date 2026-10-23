/***********************************************************************************************************************
OPTION PICOMITE: the PicoMite's MODE and RESOLUTION on the CMM2 (22 Oct 2026, docs/PicoMite_CMM2_Mode_Map.md)

OPTION PICOMITE VGA | HDMI [, resolution] says which PicoMite a program was written for (its MODE numbers mean other
displays on the two), and at which of its resolutions (OPTION RESOLUTION there, 640x480 by default). OPTION PICOMITE
NONE, the default, leaves MODE the CMM2's.

Under it, MODE n (a number alone) is the PicoMite's mode n: its size, its colours and its framebuffers, as MODE
n,1 (MODE 1), n,4 (2 and 3), n,16 (4) and n,8 (5). Each PicoMite resolution has one CMM2 timing (PicoBase), which all
its modes use, as on the PicoMite, where MODE never changes the monitor's timing: its smaller modes are made from it
as the CMM2 makes mode 7 from mode 8, the pixel clock divided by 2 or 4 and each row shown 2 or 4 times, so a pixel is
2x2 or 4x4 as there (Peter, 23 Oct, on the monitor). The display's pages are the PicoMite's size exactly, so MM.HRES,
MM.VRES, the clipping, the console and a program's PEEKs and POKEs are the PicoMite's. Where the timing is larger
(720x400 and 800x480 on 848x480, 1024x600 on 1024x768) the LTDC shows them as a window at the top left, the rest of the
screen the background (black), where nothing can be drawn (decisions 3 and 4). MODE with a colour depth is still the
CMM2's MODE.

RUN starts a program in MODE 1 (decision 7). When it ends its mode stays at the prompt, its framebuffers closed and its
font, colours and tiles the mode's own, as the PicoMite's end does, until EDIT or FM, which use the CMM2's own display
(decision 8). MODE always clears the screen (decision 9). RESOLUTION (OPTION PICOMITE HDMI) changes the resolution
until the next restart, and sets MODE 1, as the PicoMite HDMI's does.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "PicoMode.h"

extern const int xres[], yres[];
extern void setmode(int mode, int colour, int bc, int force);

int PicoActive;
int PicoModeNow = 1;
int PicoResNow = -1;                                                // (-1: OPTION PICOMITE's)
static int PicoFontNow = 1;

// the PicoMite's resolutions (OPTION RESOLUTION): the VGA builds have the first four
static const struct { short w, h; const char *name, *alias; } PicoRes[] = {
    {640, 480, "640x480", "640"}, {720, 400, "720x400", "720"}, {800, 600, "800x600", "800"}, {848, 480, "848x480", "848"},
    {800, 480, "800x480", NULL}, {1024, 600, "1024x600", NULL}, {1280, 720, "1280x720", "1280"},
    {1024, 768, "1024x768", "1024"}};
#define PICORES (int)(sizeof(PicoRes) / sizeof(PicoRes[0]))
// the CMM2 timing each resolution is shown on (Peter, 23 Oct): its own where the CMM2 has it, 848x480 for 720x400 and
// 800x480, and 1024x768 for 1024x600 (mode 18, the CMM2's 1024x600, is a timing monitors show poorly)
static const unsigned char PicoBase[] = {8, 10, 1, 10, 10, 9, 11, 9};

/* The PicoMite builds OPTION PICOMITE can name: their MM.DEVICE$ (core/MM_Misc.c, fun_device: the RP2350's with its
   " RP2350A" or " RP2350B"), which a program may test to configure itself (Peter, 23 Oct 2026). Option.Picomite is the
   index (0: NONE). The name says only whether MODE is the VGA builds' or the HDMI builds'; nothing else is limited by
   it. OPTION PICOMITE VGA and HDMI are the builds nearest a CMM2: a USB keyboard and the RP2350's modes. */
static const char *const PicoNames[] = {NULL,
    "PicoMiteVGA", "PicoMiteVGA RP2350A", "PicoMiteVGA RP2350B",
    "PicoMiteVGAUSB", "PicoMiteVGAUSB RP2350A", "PicoMiteVGAUSB RP2350B",          // (the last VGA one: PICO_LASTVGA)
    "PicoMiteHDMI RP2350A", "PicoMiteHDMI RP2350B", "PicoMiteHDMIUSB RP2350A", "PicoMiteHDMIUSB RP2350B",
    "PicoMitePC3 RP2350A", "PicoMitePC3 RP2350B", "PicoMiteHDMIBTH RP2350A", "PicoMiteHDMIBTH RP2350B",
    "PicoMiteHDMIWEB RP2350A", "PicoMiteHDMIWEB RP2350B"};
#define PICONAMES (int)(sizeof(PicoNames) / sizeof(PicoNames[0]))
#define PICO_LASTVGA 6
#define PICO_DEFVGA 6                                               // OPTION PICOMITE VGA: PicoMiteVGAUSB RP2350B
#define PICO_DEFHDMI 10                                             // OPTION PICOMITE HDMI: PicoMiteHDMIUSB RP2350B

int PicoFlavour(void) {
    int d = Option.Picomite;                                        // (erased flash: 0xFF, NONE)
    if(d < 1 || d >= PICONAMES) return PICO_NONE;
    return d <= PICO_LASTVGA ? PICO_VGA : PICO_HDMI;
}

// MM.DEVICE$ under OPTION PICOMITE: the PicoMite's name it gives, NULL under NONE
const char *PicoDeviceName(void) {
    return PicoFlavour() == PICO_NONE ? NULL : PicoNames[Option.Picomite];
}

static int PicoResolution(void) {
    if(PicoResNow >= 0) return PicoResNow;
    return Option.PicoRes < PICORES ? Option.PicoRes : 0;
}

// a resolution as the PicoMite's OPTION RESOLUTION and RESOLUTION take it, for this flavour
static int PicoResArg(char *p, int flavour) {
    for(int i = 0; i < (flavour == PICO_VGA ? 4 : PICORES); i++)
        if(checkstring(p, (char *)PicoRes[i].name) || (PicoRes[i].alias && checkstring(p, (char *)PicoRes[i].alias))) return i;
    error("Invalid resolution");
    return 0;
}

/* MODE n under OPTION PICOMITE: the PicoMite's mode n at the current resolution (its setmode() and ResetDisplay(),
   graphics/Draw.c), on that resolution's CMM2 timing, its pixels 1x1, 2x2 or 4x4 as there */
void PicoModeSet(int n) {
    int f = PicoFlavour(), res = PicoResolution(), w = PicoRes[res].w, h = PicoRes[res].h, depth, font, scale = 1;
    int quad = res >= 5, mode = PicoBase[res];                      // 1024x600, 1280x720, 1024x768: HDMI's 4x4 modes
    if(f == PICO_NONE) error("Needs OPTION PICOMITE");
    if(n < 1 || n > (f == PICO_VGA ? 3 : 5)) error("% is invalid (valid is 1 to %)", n, f == PICO_VGA ? 3 : 5);
    switch(n) {
        case 1:                                                     // 1 bit and tiles; font 3 at HDMI's 1280x720 and 1024x768
            depth = 1;
            font = (res == 6 || res == 7) ? ((2 << 4) | 1) : 1;
            break;
        case 3:                                                     // 16 colours, the full resolution (half at the 4x4 ones)
            depth = 4;
            font = 1;
            if(quad) scale = 2;
            break;
        case 4:                                                     // RGB555, HDMI's 640x480 and 720x400 only
            if(res > 1) error("Mode not available in this resolution");
            depth = 16;
            font = (6 << 4) | 1;
            scale = 2;
            break;
        default:                                                    // 2: 16 colours, 5: 256; half the resolution (a quarter
            depth = n == 2 ? 4 : 8;                                 // at the 4x4 ones)
            font = (6 << 4) | 1;
            scale = quad ? 4 : 2;
            break;
    }
    w /= scale;
    h /= scale;
    if(depth == 4 && !Rgb121SizeFits(w, h, scale)) error("Mode not available in this resolution");
    FrameInterrupt = NULL;                                          // (as MODE without one)
    ModeReqW = w;                                                   // the display the PicoMite's size, its pixels 'scale'
    ModeReqH = h;                                                   // square: setmode takes these
    ModeReqHs = ModeReqVs = scale;
    ModeReqFont = font;
    setmode(mode, depth, 0, 1);                                     // (forced: the PicoMite's MODE always clears)
    PicoModeNow = n;
    PicoFontNow = font;
}

/* the end of a program (or an error) with a PicoMite mode on the display: the mode stays, as the PicoMite's do_end
   (setmode(mode, false)) leaves it, with its framebuffers closed, the drawing back on N at its full height (POKE DISPLAY
   VRES), the colours white on black, the mode's font and, in MODE 1, the tiles in the default colours */
void PicoModeEnd(void) {
    Rgb121CloseAll();
    WritePage = ReadPage = 0;
    for(int i = 0; i <= LastPage; i++) if(PageTable[i].address) PageTable[i].ymax = ModeYres(VideoMode);
    gui_fcolour = PromptFC = Option.DefaultFC = WHITE;
    gui_bcolour = PromptBC = Option.DefaultBC = BLACK;
    SetFont(PicoFontNow);
    Option.DefaultFont = gui_font;
    if(VideoColour == 1) MonoTilesReset();
}

/* OPTION PICOMITE VGA | HDMI | NONE [, resolution]: saved, at the prompt only */
void PicoOption(char *tp) {
    int d = -1, res = 0;
    char *p;
    if(CurrentLinePtr) error("Invalid in a program");
    getargs(&tp, 3, ",");
    if(argc < 1) error("Syntax");
    if(checkstring(argv[0], "NONE")) d = 0;
    else if(checkstring(argv[0], "VGA")) d = PICO_DEFVGA;
    else if(checkstring(argv[0], "HDMI")) d = PICO_DEFHDMI;
    else if(*argv[0] == '"') {                                      // a name as a string
        char *s = getCstring(argv[0]);
        for(int i = 1; i < PICONAMES; i++) if(strcasecmp(s, PicoNames[i]) == 0) d = i;
    } else {                                                        // or as words: the longest first (PicoMiteVGA RP2350A
        for(int i = PICONAMES - 1; i >= 1 && d < 0; i--)            // before PicoMiteVGA), the whole argument
            if((p = checkstring(argv[0], (char *)PicoNames[i])) != NULL) {
                skipspace(p);
                if(*p == 0) d = i;
            }
    }
    if(d < 0) error("Invalid device");
    if(argc == 3) {
        if(d == 0) error("Syntax");
        res = PicoResArg(argv[2], d <= PICO_LASTVGA ? PICO_VGA : PICO_HDMI);
    }
    Option.Picomite = d;
    Option.PicoRes = res;
    PicoResNow = -1;
    SaveOptions(1);
}

void PicoOptionList(void) {
    int f = PicoFlavour(), res = Option.PicoRes < PICORES ? Option.PicoRes : 0;
    if(f == PICO_NONE) return;
    MMPrintString("OPTION PICOMITE ");
    MMPrintString((char *)PicoNames[Option.Picomite]);
    MMPrintString(", ");
    MMPrintString((char *)PicoRes[res].name);
    MMPrintString("\r\n");
}

/* RESOLUTION res [, speed]: the PicoMite HDMI's (graphics/Draw.c, cmd_resolution), here the resolution OPTION PICOMITE's
   modes map from until the next restart; then MODE 1. Its VGA builds have none. The speed (640x480's 252000, 315000 or
   378000) is checked and has nothing to change here. */
void cmd_resolution(void) {
    int res;
    if(PicoFlavour() != PICO_HDMI) error("Needs OPTION PICOMITE HDMI");
    getargs(&cmdline, 3, ",");
    if(argc != 1 && argc != 3) error("Syntax");
    res = PicoResArg(argv[0], PICO_HDMI);
    if(argc == 3) {
        int s;
        if(res != 0) error("Syntax");                              // (the speed only for 640x480)
        s = getint(argv[2], 252000, 378000);
        if(!(s == 252000 || s == 315000 || s == 378000)) error("Invalid speed");
    }
    PicoResNow = res;
    PicoModeSet(1);
}
