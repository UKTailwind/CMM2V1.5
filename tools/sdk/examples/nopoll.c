/* nopoll.c: the USB devices left unserviced (tools/sdk). MIT License, as cmm2img.h.

     RUN "nopoll.app" [, ms]        waits ms (default 3000) without calling poll, so no USB transfer is made, then
                                    polls for 3 seconds: a device that resets itself when it is not polled shows as
                                    disconnected and connected again */
#include <stdio.h>
#include <stdlib.h>
#include "cmm2.h"

int main(int argc, char **argv) {
    uint32_t ms = argc > 1 ? (uint32_t)atoi(argv[1]) : 3000;
    uint64_t t;
    printf("no polling for %lu ms\n", (unsigned long)ms);
    cmm2->delay_us(ms * 1000);
    printf("polling for 3 s\n");
    for(t = cmm2->micros(); cmm2->micros() - t < 3000000;) if(cmm2->poll()) return 3;
    printf("done\n");
    return 0;
}
