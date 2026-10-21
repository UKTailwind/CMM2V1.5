/* Ray.h - RAY and RAY(: the PicoMite's raycaster in every colour depth (Ray.c, docs/Ray_Plan.md) */
#ifndef RAY_H
#define RAY_H
void cmd_ray(void);
void fun_ray(void);                                 // RAY(: fun_sprite hands it SPRITE(RAY,'s arguments (the tokeniser's fold)
void ray_close(void);                               // RAY CLOSE: everything freed
void RayForget(void);                               // the heap was wiped (HeapWipe): nothing is left to free
#endif
