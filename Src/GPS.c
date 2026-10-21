/***************************************************************************
CMM2 MMBasic
GPS.c

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */
 
/* TODO:  Include other files here if needed. */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Astro.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */
/** Descriptive Data Item Name

  @Summary
    Brief one-line summary of the data item.
    
  @Description
    Full description, explaining the purpose and usage of data item.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
  @Remarks
    Any additional remarks
 */
//int global_data;
int GPSchannel=0;
volatile char gpsbuf1[128];
volatile char gpsbuf2[128];
volatile char *gpsbuf;
volatile char *gpsready;
volatile char gpscount;
volatile int gpscurrent;
int gpsmonitor;
MMFLOAT GPSlatitude=0;
MMFLOAT GPSlongitude=0;
MMFLOAT GPSspeed=0;
int GPSvalid=0;
// MMBasic strings: byte 0 is the length.  As "000:00:0" and "000-00-200" it was '0' (48), so until an RMC
// sentence with a date set them, GPS(TIME) and GPS(DATE) returned 48 bytes of memory (the PicoMite's 774e640)
char GPStime[9] = {8, '0', '0', ':', '0', '0', ':', '0', '0'};
char GPSdate[11] = {10, '0', '0', '-', '0', '0', '-', '2', '0', '0', '0'};
MMFLOAT GPStrack=0;
int GPSut[6];                                       // the last fix's UT: day, month, year, hour, minute, second (STAR's)
MMFLOAT GPSSidereal = 0.0;                          // its local sidereal time (hours) and Julian centuries since J2000.0
MMFLOAT GPSJulian = 0.0;                            // (GPS(SIDEREAL), GPS(JULIAN): the PicoMite's)
MMFLOAT GPSdop=0;
int GPSsatellites=0;
MMFLOAT GPSaltitude=0;
MMFLOAT GPSgeoid=0;
int GPSfix=0;  
int GPSadjust=0;
void GPS_parse(char *nmea);
#define EPOCH_ADJUSTMENT_DAYS	719468L
/* year to which the adjustment was made */
#define ADJUSTED_EPOCH_YEAR	0
/* 1st March of year 0 is Wednesday */
#define ADJUSTED_EPOCH_WDAY	3
/* there are 97 leap years in 400-year periods. ((400 - 97) * 365 + 97 * 366) */
#define DAYS_PER_ERA		146097L
/* there are 24 leap years in 100-year periods. ((100 - 24) * 365 + 24 * 366) */
#define DAYS_PER_CENTURY	36524L
/* there is one leap year every 4 years */
#define DAYS_PER_4_YEARS	(3 * 365 + 366)
/* number of days in a non-leap year */
#define DAYS_PER_YEAR		365
/* number of days in January */
#define DAYS_IN_JANUARY		31
/* number of days in non-leap February */
#define DAYS_IN_FEBRUARY	28
/* number of years per era */
#define YEARS_PER_ERA		400
#define SECSPERDAY 86400
#define SECSPERHOUR 3600
#define SECSPERMIN 60
#define DAYSPERWEEK 7
#define YEAR_BASE 1900
/* Number of days per month (except for February in leap years). */
static const int monoff[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

static int
is_leap_year(int year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int
leap_days(int y1, int y2)
{
	--y1;
	--y2;
	return (y2/4 - y1/4) - (y2/100 - y1/100) + (y2/400 - y1/400);
}
struct tm *
gmtime_r (const time_t *__restrict tim_p,
	struct tm *__restrict res)
{
  long days, rem;
  const time_t lcltime = *tim_p;
  int era, weekday, year;
  unsigned erayear, yearday, month, day;
  unsigned long eraday;

  days = lcltime / SECSPERDAY + EPOCH_ADJUSTMENT_DAYS;
  rem = lcltime % SECSPERDAY;
  if (rem < 0)
    {
      rem += SECSPERDAY;
      --days;
    }

  /* compute hour, min, and sec */
  res->tm_hour = (int) (rem / SECSPERHOUR);
  rem %= SECSPERHOUR;
  res->tm_min = (int) (rem / SECSPERMIN);
  res->tm_sec = (int) (rem % SECSPERMIN);

  /* compute day of week */
  if ((weekday = ((ADJUSTED_EPOCH_WDAY + days) % DAYSPERWEEK)) < 0)
    weekday += DAYSPERWEEK;
  res->tm_wday = weekday;

  /* compute year, month, day & day of year */
  /* for description of this algorithm see
   * http://howardhinnant.github.io/date_algorithms.html#civil_from_days */
  era = (days >= 0 ? days : days - (DAYS_PER_ERA - 1)) / DAYS_PER_ERA;
  eraday = days - era * DAYS_PER_ERA;	/* [0, 146096] */
  erayear = (eraday - eraday / (DAYS_PER_4_YEARS - 1) + eraday / DAYS_PER_CENTURY -
      eraday / (DAYS_PER_ERA - 1)) / 365;	/* [0, 399] */
  yearday = eraday - (DAYS_PER_YEAR * erayear + erayear / 4 - erayear / 100);	/* [0, 365] */
  month = (5 * yearday + 2) / 153;	/* [0, 11] */
  day = yearday - (153 * month + 2) / 5 + 1;	/* [1, 31] */
  month += month < 10 ? 2 : -10;
  year = ADJUSTED_EPOCH_YEAR + erayear + era * YEARS_PER_ERA + (month <= 1);

  res->tm_yday = yearday >= DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY ?
      yearday - (DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY) :
      yearday + DAYS_IN_JANUARY + DAYS_IN_FEBRUARY + is_leap_year(erayear);
  res->tm_year = year - YEAR_BASE;
  res->tm_mon = month;
  res->tm_mday = day;

  res->tm_isdst = 0;

  return (res);
}
struct tm *
gmtime (const time_t * tim_p)
{
  struct _reent *reent = _REENT;

  _REENT_CHECK_TM(reent);
  return gmtime_r (tim_p, (struct tm *)_REENT_TM(reent));
}

time_t
timegm(const struct tm *tm)
{
	int year;
	time_t days;
	time_t hours;
	time_t minutes;
	time_t seconds;

	year = 1900 + tm->tm_year;
	days = 365 * (year - 1970) + leap_days(1970, year);
	days += monoff[tm->tm_mon];

	if (tm->tm_mon > 1 && is_leap_year(year))
		++days;
	days += tm->tm_mday - 1;

	hours = days * 24 + tm->tm_hour;
	minutes = hours * 60 + tm->tm_min;
	seconds = minutes * 60 + tm->tm_sec;

	return seconds;
}

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */

/** 
  @Function
    int ExampleLocalFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Description
    Full description, explaining the purpose and usage of the function.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.

  @Precondition
    List and describe any required preconditions. If there are no preconditions,
    enter "None."

  @Parameters
    @param param1 Describe the first parameter to the function.
    
    @param param2 Describe the second parameter to the function.

  @Returns
    List (if feasible) and describe the return values of the function.
    <ul>
      <li>1   Indicates an error occurred
      <li>0   Indicates an error did not occur
    </ul>

  @Remarks
    Describe any special behavior not described above.
    <p>
    Any additional remarks.

  @Example
    @code
    if(ExampleFunctionName(1, 2) == 0)
    {
        return 3;
    }
 */
//static int ExampleLocalFunction(int param1, int param2) {
//    return 0;
//}

#define INDENT_SPACES "  "

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

// *****************************************************************************

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
//int ExampleInterfaceFunction(int param1, int param2) {
//    return 0;
//}
void fun_GPS(void){
    sret = GetTempStrMemory();                                    // this will last for the life of the command
    if(!GPSchannel) error("GPS not activated");
    if(checkstring(ep, "LATITUDE") != NULL) {
        fret = GPSlatitude;
        targ = T_NBR;   
    }
    else if(checkstring(ep, "LONGITUDE") != NULL) {
        fret = GPSlongitude;
        targ = T_NBR;   
    }
    else if(checkstring(ep, "SPEED") != NULL) {
        fret = GPSspeed;
        targ = T_NBR;   
    }
    else if(checkstring(ep, "TRACK") != NULL) {
        fret = GPStrack;
        targ = T_NBR;   
    }
    else if(checkstring(ep, "VALID") != NULL) {
        iret = GPSvalid;
        targ = T_INT;   
    }
    else if(checkstring(ep, "TIME") != NULL) {
        sret = GPStime;
        targ = T_STR;   
    } 
    else if(checkstring(ep, "DATE") != NULL) {
        sret = GPSdate;
        targ = T_STR;   
    } 
    else if(checkstring(ep, "SATELLITES") != NULL) {
        iret = GPSsatellites;
        targ = T_INT;   
    } 
    else if(checkstring(ep, "ALTITUDE") != NULL) {
        fret = GPSaltitude;
        targ = T_NBR;   
    }
    else if(checkstring(ep, "DOP") != NULL) {
        fret = GPSdop;
        targ = T_NBR;   
    }
    else if(checkstring(ep, "FIX") != NULL) {
        iret = GPSfix;
        targ = T_INT;   
    } 
    else if(checkstring(ep, "GEOID") != NULL) {
        fret = GPSgeoid;
        targ = T_NBR;   
    }
    else if(checkstring(ep, "SIDEREAL") != NULL) {              // the PicoMite's: the fix's local sidereal time (hours)
        fret = GPSSidereal;
        targ = T_NBR;
    }
    else if(checkstring(ep, "JULIAN") != NULL) {                // and Julian centuries since J2000.0
        fret = GPSJulian;
        targ = T_NBR;
    }
    else error("Invalid command");
}
  
    
// The PicoMite's (d18b053): the monitor print inside GPS_parse can scroll the screen, and scrolling calls
// routinechecks(), which calls processgps() again.  So the line is claimed, and gpsready cleared, before it is
// parsed, and a nested call returns: the old one parsed the same sentence again until the stack overflowed.
int GPSbusy = false;                                                // a line is being parsed (the USB serial feed waits)
void processgps(void){
    if(GPSTimer>2000){
        GPSvalid=0;
    }
    if(GPSbusy) return;
    if(gpsready !=NULL){
        char *line = (char *)gpsready;
        gpsready=NULL;
        GPSbusy = true;
        GPS_parse(line);
        GPSTimer=0;
        GPSbusy = false;
        CdcGpsFeed();                                               // a USB GPS (COM4-COM7): its next queued line
    }
}
uint8_t parseHex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
    // if (c > 'F')
    return 0;
}

// One NMEA sentence: the PicoMite's parser.  Its trailing CR and LF are taken off, and a sentence without its
// checksum, or with a wrong one, is refused (the CMM2's looked for the '*' 4 from the end, which with the CRLF a
// receiver sends is never there, so it parsed every line unchecked: a sentence cut short, at the 128 character
// limit or by noise, then sent strchr(p, ',') + 1 to address 1).  Latitude and longitude are read in double
// precision (the old fixed point and single float lost about a metre), and RMC sets GPS(VALID), and the time and
// date, only from a sentence that has its date.
void GPS_parse(char *nmea) {
    uint8_t hour, minute, seconds, year=0, month=0, day=0;
    uint16_t __attribute__((unused)) milliseconds;
    MMFLOAT latitudeDegrees=0.0, longitudeDegrees=0.0;
    struct tm *tm;
    struct tm tma;
    char degreebuff[10];
    int len;
    tm=&tma;
    len = strlen(nmea);
    while(len > 0 && (nmea[len - 1] == '\r' || nmea[len - 1] == '\n'))
        nmea[--len] = 0;
    if(gpsmonitor){
        MMPrintString(nmea);
        MMPrintString("\r\n");
    }
    // the checksum: the two hex digits after the '*', the exclusive or of everything between the '$' and the '*'
    if(len >= 4 && nmea[len - 3] == '*') {
        uint16_t sum = parseHex(nmea[len - 2]) * 16;
        sum += parseHex(nmea[len - 1]);
        for(int i = 1; i < (len - 3); i++) sum ^= nmea[i];
        if(sum != 0) return;                                        // bad checksum
    } else
        return;                                                     // no checksum

    if(strstr(nmea, "$GPGGA") || strstr(nmea, "$GNGGA")) {
        char *p = nmea;
        // time
        p = strchr(p, ',')+1;
        MMFLOAT timef = atof(p);
        uint32_t time = timef;
        hour = time / 10000;
        minute = (time % 10000) / 100;
        seconds = (time % 100);
        milliseconds = fmod(timef, 1.0) * 1000;
        // latitude
        p = strchr(p, ',')+1;
        if(',' != *p) {
            strncpy(degreebuff, p, 2);
            p += 2;
            degreebuff[2] = '\0';
            latitudeDegrees = atol(degreebuff) + (atof(p) / 60.0);
        }
        p = strchr(p, ',')+1;
        if(',' != *p) {
            if(p[0] == 'S') latitudeDegrees *= -1.0;
        }
        GPSlatitude = latitudeDegrees;
        // longitude
        p = strchr(p, ',')+1;
        if(',' != *p) {
            strncpy(degreebuff, p, 3);
            p += 3;
            degreebuff[3] = '\0';
            longitudeDegrees = atol(degreebuff) + (atof(p) / 60.0);
        }
        p = strchr(p, ',')+1;
        if(',' != *p) {
            if(p[0] == 'W') longitudeDegrees *= -1.0;
        }
        GPSlongitude = longitudeDegrees;
        p = strchr(p, ',')+1;
        if(',' != *p) GPSfix = atoi(p);
        p = strchr(p, ',')+1;
        if(',' != *p) GPSsatellites = atoi(p);
        p = strchr(p, ',')+1;
        if(',' != *p) GPSdop = (MMFLOAT)atof(p);
        p = strchr(p, ',')+1;
        if(',' != *p) GPSaltitude = (MMFLOAT)atof(p);
        p = strchr(p, ',')+1;                                       // the altitude's units (metres)
        p = strchr(p, ',')+1;
        if(',' != *p) GPSgeoid = (MMFLOAT)atof(p);
        return;
    }
    if(strstr(nmea, "$GPRMC") || strstr(nmea, "$GNRMC")) {
        char *p = nmea;
        int i, localGPSvalid = 0;
        // time
        p = strchr(p, ',')+1;
        MMFLOAT timef = atof(p);
        uint32_t time = timef;
        hour = time / 10000;
        minute = (time % 10000) / 100;
        seconds = (time % 100);
        milliseconds = fmod(timef, 1.0) * 1000;
        GPStime[1] = (hour / 10) + 48;
        GPStime[2] = (hour % 10) + 48;
        GPStime[4] = (minute / 10) + 48;
        GPStime[5] = (minute % 10) + 48;
        GPStime[7] = (seconds / 10) + 48;
        GPStime[8] = (seconds % 10) + 48;
        // status
        p = strchr(p, ',')+1;
        if(p[0] == 'A') localGPSvalid = 1;
        else if(p[0] == 'V') localGPSvalid = 0;
        else {
            GPSvalid = 0;
            return;
        }
        // latitude
        p = strchr(p, ',')+1;
        if(',' != *p) {
            strncpy(degreebuff, p, 2);
            p += 2;
            degreebuff[2] = '\0';
            latitudeDegrees = atol(degreebuff) + (atof(p) / 60.0);
        }
        p = strchr(p, ',')+1;
        if(',' != *p) {
            if(p[0] == 'S') latitudeDegrees *= -1.0;
        }
        GPSlatitude = latitudeDegrees;
        // longitude
        p = strchr(p, ',')+1;
        if(',' != *p) {
            strncpy(degreebuff, p, 3);
            p += 3;
            degreebuff[3] = '\0';
            longitudeDegrees = atol(degreebuff) + (atof(p) / 60.0);
        }
        p = strchr(p, ',')+1;
        if(',' != *p) {
            if(p[0] == 'W') longitudeDegrees *= -1.0;
        }
        GPSlongitude = longitudeDegrees;
        // speed, track
        p = strchr(p, ',')+1;
        if(',' != *p) GPSspeed = (MMFLOAT)atof(p);
        p = strchr(p, ',')+1;
        if(',' != *p) GPStrack = (MMFLOAT)atof(p);
        // the date: then the time and date, moved by the time offset of OPEN ... AS GPS
        p = strchr(p, ',')+1;
        if(',' != *p && p[6] == ',') {
            uint32_t fulldate = atoi(p);
            day = fulldate / 10000;
            month = (fulldate % 10000) / 100;
            year = (fulldate % 100);
            GPSut[0] = day;                         // (UT: GPStime and GPSdate are moved by the offset)
            GPSut[1] = month;
            GPSut[2] = year + 2000;
            GPSut[3] = hour;
            GPSut[4] = minute;
            GPSut[5] = seconds;
            GPStime[0] = 8;
            tm->tm_year = year + 100;
            tm->tm_mon = month - 1;
            tm->tm_mday = day;
            tm->tm_hour = hour;
            tm->tm_min = minute;
            tm->tm_sec = seconds;
            time_t timestamp = timegm(tm);
            timestamp += GPSadjust;
            tm = gmtime(&timestamp);
            i = tm->tm_hour;
            GPStime[1] = (i / 10) + 48;
            GPStime[2] = (i % 10) + 48;
            i = tm->tm_min;
            GPStime[4] = (i / 10) + 48;
            GPStime[5] = (i % 10) + 48;
            i = tm->tm_sec;
            GPStime[7] = (i / 10) + 48;
            GPStime[8] = (i % 10) + 48;
            i = tm->tm_mday;
            GPSdate[0] = 10;
            GPSdate[1] = (i / 10) + 48;
            GPSdate[2] = (i % 10) + 48;
            i = tm->tm_mon + 1;
            GPSdate[4] = (i / 10) + 48;
            GPSdate[5] = (i % 10) + 48;
            i = tm->tm_year % 100;
            GPSdate[9] = (i / 10) + 48;
            GPSdate[10] = (i % 10) + 48;
            GPSSidereal = getSiderealTime(day, month, year + 2000, hour, minute, seconds, GPSlongitude);
            GPSJulian = getJulianCenturies(day, month, year + 2000, hour, minute, seconds);
            GPSvalid = localGPSvalid;
            return;
        }
    }
}
