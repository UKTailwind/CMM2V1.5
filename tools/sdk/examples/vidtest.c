/* vidtest.c: the screen and keyboard services (version 2) tried (tools/sdk). MIT License, as cmm2img.h.

     RUN "vidtest.app" [, mode]     MODE mode,8 (default 3, 320x200): checks that bad modes are refused, draws 256
                                    colour bars, times the frames, turns the palette for 3 seconds, then prints the keys
                                    held as they change until Esc (or Ctrl-C). Built with --api 2. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"

static cmm2_video v;

static void line(int y, const uint8_t *px) {                       // line y, stored twice in a doubled mode
    uint8_t *p = v.fb + y * v.pitch;
    memcpy(p, px, v.width);
    if(v.copies == 2) memcpy(p + v.width, px, v.width);
}

int main(int argc, char **argv) {
    int mode = argc > 1 ? atoi(argv[1]) : 3, i, x, y, r, n, last[7] = {0}, keys[7];
    uint8_t rgb[256 * 3], px[1920];
    uint64_t t0, t1;

    if(cmm2->version < 2) { printf("this firmware's services are version %lu\n", (unsigned long)cmm2->version); return 1; }
    printf("bad modes: mode 0 %d, mode 19 %d, depth 16 %d\n", cmm2->video_mode(0, 8, &v), cmm2->video_mode(19, 8, &v),
           cmm2->video_mode(1, 16, &v));
    r = cmm2->video_mode(mode, 8, &v);
    if(r < 0) { printf("MODE %d,8 refused: %d\n", mode, r); return 2; }

    for(y = 0; y < (int)v.height; y++) {                            // bars of colours 0-254, a frame of 255 round them
        for(x = 0; x < (int)v.width; x++) px[x] = (x * 255 / v.width + y / 8) % 255;
        if(y == 0 || y == (int)v.height - 1) memset(px, 255, v.width);
        px[0] = px[v.width - 1] = 255;
        line(y, px);
    }
    cmm2->video_wait();                                             // the frame time: 60 frames
    t0 = cmm2->micros();
    for(i = 0; i < 60; i++) cmm2->video_wait();
    t1 = cmm2->micros();
    printf("MODE %d,8: %lux%lu, pitch %lu, copies %lu, fb %p; a frame takes %lu us\n", mode, (unsigned long)v.width,
           (unsigned long)v.height, (unsigned long)v.pitch, (unsigned long)v.copies, (void *)v.fb,
           (unsigned long)((t1 - t0) / 60));

    t0 = cmm2->micros();                                            // the palette turned, a step a frame
    for(n = 0; cmm2->micros() - t0 < 3000000 && !cmm2->poll(); n++) {
        for(i = 0; i < 255; i++) {                                  // (255, the frame, stays white)
            int h = (i + n * 2) % 255, s = h % 85 * 3;              // a rainbow: red to green to blue and back
            rgb[i * 3 + 0] = h < 85 ? 255 - s : h < 170 ? 0 : s;
            rgb[i * 3 + 1] = h < 85 ? s : h < 170 ? 255 - s : 0;
            rgb[i * 3 + 2] = h < 85 ? 0 : h < 170 ? s : 255 - s;
        }
        rgb[255 * 3] = rgb[255 * 3 + 1] = rgb[255 * 3 + 2] = 255;
        cmm2->video_palette(0, 256, rgb);
    }
    printf("%d palettes in %lu ms\n", n, (unsigned long)((cmm2->micros() - t0) / 1000));

    printf("press keys (Esc ends)\n");
    while(!cmm2->poll()) {
        n = cmm2->keys_held(keys);
        if(memcmp(keys, last, sizeof(keys))) {
            memcpy(last, keys, sizeof(keys));
            printf("%d held:", n);
            for(i = 0; i < 6; i++) if(keys[i]) printf(" &H%02X", keys[i]);
            printf("  modifiers &H%02X\n", keys[6]);
            for(i = 0; i < 6; i++) if(keys[i] == 0x1B) return 0;
        }
        cmm2->video_wait();
    }
    printf("Ctrl-C\n");
    return 3;
}
