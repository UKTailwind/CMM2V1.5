/* hello.c: a loadable program that tries the services (tools/sdk). MIT License, as cmm2img.h.

     RUN "hello.app" [, words]      prints where it is, its arguments, the heap and the time; writes, reads back and
                                    removes A:/hello_app.txt; lists the first entries of A:/; times a loop
     RUN "hello.app", leave         also leaves from inside a call, with 42 (the loader prints the code)
     RUN "hello.app", code          ends with 7 from main */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"

static void deep(int n) {
    if(n == 0) {
        printf("leaving with 42 from inside a call\n");
        cmm2->leave(42);
        return;
    }
    deep(n - 1);
    printf("not reached\n");
}

int main(int argc, char **argv) {
    int i, d, n, t[6];
    char line[80];
    FILE *f;
    cmm2_dirent e;
    uint64_t t0, us;
    volatile double x = 0;

    printf("Hello from a loadable program: main at %p, image at 0x%08lX, CPU %lu MHz\n", (void *)main,
           (unsigned long)cmm2_start->base, (unsigned long)(cmm2->cpu_hz() / 1000000));
    for(i = 0; i < argc; i++) printf("argv[%d] = \"%s\"\n", i, argv[i]);
    printf("heap: %lu KB at %p\n", (unsigned long)(cmm2_start->heap_size / 1024), cmm2_start->heap);
    char *m = malloc(4 << 20);
    printf("malloc(4 MB): %p\n", (void *)m);
    if(m) { memset(m, 0x55, 4 << 20); free(m); }
    cmm2->datetime(t);
    printf("MMBasic's clock: %04d-%02d-%02d %02d:%02d:%02d\n", t[0], t[1], t[2], t[3], t[4], t[5]);

    if((f = fopen("A:/hello_app.txt", "w")) == NULL) printf("fopen for writing failed\n");
    else {
        fprintf(f, "written by hello.app, pi is %.6f\n", 3.14159265358979);
        fclose(f);
        f = fopen("A:/hello_app.txt", "r");
        if(f && fgets(line, sizeof(line), f)) printf("read back: %s", line);
        if(f) fclose(f);
        printf("remove: %d\n", remove("A:/hello_app.txt"));
    }
    if((d = cmm2->dir_open("A:/")) < 0) printf("dir_open failed: %d\n", d);
    else {
        printf("A:/ starts:");
        for(n = 0; n < 6 && cmm2->dir_read(d, &e) == 1; n++) printf(" %s%s", e.name, e.is_dir ? "/" : "");
        printf("\n");
        cmm2->dir_close(d);
    }
    t0 = cmm2->micros();
    for(i = 0; i < 1000000; i++) x += i * 0.5;
    us = cmm2->micros() - t0;
    printf("a million double multiply-adds: %lu us (x = %.0f)\n", (unsigned long)us, x);

    if(argc > 1 && strcmp(argv[1], "leave") == 0) deep(10);
    if(argc > 1 && strcmp(argv[1], "code") == 0) return 7;
    return 0;
}
