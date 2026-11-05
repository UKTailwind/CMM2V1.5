# C Programs

## The language

The language is C89 (ANSI C) with long long, // comments and declarations after statements. A program is one source file, as there is no linker.

These are not supported:

- bit fields;
- declarations inside a for statement, as in for(int i = 0; ...): declare i before it;
- local arrays initialised from a string: "static char s[] = ..." works, and "char s[] = ..." inside a function does not.

Errors are listed with the file's name and the line number (hello.c:12 - missing semicolon), and CC then gives the error "hello.c did not compile".

## The library

- stdio.h: printf, sprintf and fprintf, putchar and puts, and files with fopen, fread, fwrite, fgetc, fgets, fputc, fputs, fseek, ftell, rewind, feof, fflush, fclose, remove and rename. There is no scanf.
- stdlib.h: malloc, calloc, realloc and free, atoi, atol, atof, strtod, strtol and strtoul, abs, labs and llabs, rand and srand, and exit.
- string.h: strlen, strcpy, strncpy, strcat, strcmp, strncmp, strchr, strrchr, memset, memcpy, memmove and memcmp.
- math.h: sin, cos, tan, asin, acos, atan, atan2, sinh, cosh, tanh, sqrt, exp, log, log10, pow, floor, ceil, fabs and fmod, in double precision.
- ctype.h: isdigit, isalpha and the others, toupper and tolower.
- time.h: time, clock (CLOCKS_PER_SEC is 1000000), and time_us and time_us64, the microseconds since the Colour Maximite 2 started.
- cmm2.h: the Colour Maximite 2's own facilities (below).

Files have MMBasic's names: A:/dir/name.txt, or a name relative to the current directory.

## Graphics, sound and the rest

cmm2.h gives a C program the services that the firmware gives a loadable program, by the same names: the video modes and the palette, drawing with MMBasic's own BOX, CIRCLE, TEXT and the rest (gfx_box, gfx_circle, gfx_text...), saving the screen as a PNG, the keyboard, the mouse, sound, and the Colour Maximite 2's pins. The companion manual *Colour Maximite 2 Loadable Programs* describes each service.

```
#include <cmm2.h>

int main(void)
{
    cmm2_video v;

    video_mode(1, 8, &v);
    gfx_cls(BLACK);
    gfx_circle(400, 300, 100, 2, 1.0, GREEN, RED);
    gfx_text(400, 300, "Hello", "CM", 1, 2, WHITE, NOFILL);
    return 0;
}
```

The screen is left as it is when the program ends, and MMBasic's own mode and palette come back when it does. The framebuffer that video_mode gives is an ordinary pointer, so a program can also draw by writing to it.

A:/cc/examples/shapes.c draws with these services, and blink.c flashes an output pin.

## How a program runs

Each function is translated to the processor's own instructions where the compiler can, and the rest to a compact bytecode that the firmware interprets. Floating point uses the processor's double precision hardware. A program takes about 16ms to start.
