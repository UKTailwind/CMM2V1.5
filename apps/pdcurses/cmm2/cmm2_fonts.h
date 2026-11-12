/* cmm2_fonts.h: the fonts pdcurses.py makes from Spleen (out/cmm2_fonts.c), each a PSF2 font with its Unicode table */
#ifndef CMM2_FONTS_H
#define CMM2_FONTS_H

struct cmm2_font {
    int w, h;                                   /* a character's size in pixels */
    const char *name;                           /* "8x16", as PDC_FONT and font= give it */
    const unsigned char *psf;
    unsigned size;
};

extern const struct cmm2_font cmm2_fonts[];     /* smallest first */
extern const int cmm2_n_fonts;

#endif
