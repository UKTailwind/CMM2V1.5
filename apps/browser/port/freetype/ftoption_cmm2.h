/* ftoption_cmm2.h: FreeType's options for browser.app (apps/browser/make.py names it as FT_CONFIG_OPTIONS_H): FreeType's
   own, less what DejaVu and NetSurf do not use. The fonts are TrueType, hinted by their own bytecode (FreeType's
   default interpreter) and drawn anti-aliased.
   - No compressed fonts (gzip, LZW, bzip2, WOFF): the fonts are plain TTF files.
   - No Mac resource forks, no incremental loading, no SVG or colour glyphs, no BDF properties.
   - No glyph names (psnames and the Adobe Glyph List): NetSurf looks glyphs up by character.
   - No variable fonts: DejaVu has none.
   - No FREETYPE_PROPERTIES from the environment: a program here has none.
   MIT License, as Inc/cmm2img.h. */
#ifndef FTOPTION_CMM2_H
#define FTOPTION_CMM2_H

#include <freetype/config/ftoption.h>

#undef FT_CONFIG_OPTION_ENVIRONMENT_PROPERTIES
#undef FT_CONFIG_OPTION_USE_LZW
#undef FT_CONFIG_OPTION_USE_ZLIB
#undef FT_CONFIG_OPTION_POSTSCRIPT_NAMES
#undef FT_CONFIG_OPTION_ADOBE_GLYPH_LIST
#undef FT_CONFIG_OPTION_MAC_FONTS
#undef FT_CONFIG_OPTION_GUESSING_EMBEDDED_RFORK
#undef FT_CONFIG_OPTION_INCREMENTAL
#undef FT_CONFIG_OPTION_SVG
#undef TT_CONFIG_OPTION_COLOR_LAYERS
#undef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
#undef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#undef TT_CONFIG_OPTION_VARC
#undef TT_CONFIG_OPTION_BDF

#endif
