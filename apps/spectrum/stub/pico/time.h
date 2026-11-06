#ifndef CMM2_PICO_TIME_H
#define CMM2_PICO_TIME_H
#include "pico.h"
typedef struct repeating_timer repeating_timer_t;
struct repeating_timer { int unused; };
typedef bool (*repeating_timer_callback_t)(repeating_timer_t *rt);
#endif
