/* Astro.h - STAR, ASTRO, LOCATION and SLEW: the PicoMite's astronomy (Astro.c) */
#ifndef ASTRO_H
#define ASTRO_H
void cmd_star(void);                                // STAR (from the GPS) and ASTRO (from LOCATION)
void cmd_slew(void);
void cmd_locate(void);                              // LOCATION
// the local sidereal time (hours) and the Julian centuries since J2000.0 of a UT date and time (GPS.c's GPS(SIDEREAL),
// GPS(JULIAN))
MMFLOAT getSiderealTime(int day, int month, int year, int hour, int minute, int second, MMFLOAT longitude);
MMFLOAT getJulianCenturies(int day, int month, int year, int hour, int minute, int second);
#endif
