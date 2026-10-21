# Appendix Q – Astronomy

Four commands, the PicoMite's, give the position in the sky of the Sun, the Moon, the planets and over eighty stars and galaxies, as seen from a place on the Earth at a given time. They are accurate enough to point a telescope or to navigate by:
- STAR uses the time and place from a GPS receiver (OPEN ... AS GPS).
- LOCATION sets a time and place by hand, and ASTRO uses them.
- SLEW works out how to move a polar-aligned German equatorial telescope mount from where it points to a target, including the turn over the top of the mount (a meridian flip) when the target is on the other side of the meridian.

These names are keywords, so STAR, ASTRO, LOCATION and SLEW cannot be the names of variables.

## LOCATION

```
LOCATION date$, latitude, longitude [, sidereal]
```

Sets the time and place that ASTRO uses:
- 'date$' is the date and time in Universal Time (UT, which is GMT), as "dd/mm/yyyy hh:mm:ss". The separators can be '/', '-', ':' or a space; the year can be first ("yyyy-mm-dd hh:mm:ss") or have two digits ("dd/mm/yy").
- 'latitude' is in degrees, negative to the south (-90 to 90).
- 'longitude' is in degrees, negative to the west (-180 to 180).
- 'sidereal', if it is given, is a variable that is set to the local sidereal time there and then, in hours (0 to 24).

```
LOCATION "21/10/2026 20:00:00", 51.4769, -0.0005, lst
```

## STAR and ASTRO

```
STAR body alt, az [, ra, dec]
STAR "name", alt, az [, ra, dec]
STAR name$, alt, az [, ra, dec]
STAR alt, az, ra2000, dec2000 [, pm_ra, pm_dec] [, ra, dec]
```

ASTRO is written in the same ways. STAR uses the time and place of the GPS receiver's last fix, and needs one (GPS(VALID)); ASTRO uses those set by LOCATION.

The body can be named in three ways:
- as a word, followed by a space: STAR MOON alt, az or ASTRO Andromeda Galaxy alt, az;
- as a string in quotes, followed by a comma: STAR "Aldebaran", alt, az;
- as a string expression, followed by a comma (or a space after a variable): STAR target$, alt, az, a variable declared AS STRING, or a function such as MID$().

The names are SUN, MOON, MERCURY, VENUS, MARS, JUPITER, SATURN, URANUS and NEPTUNE, and the stars and galaxies of the table below, in capitals or small letters.

'alt' and 'az' are variables that are set to the body's altitude (degrees above the horizon, corrected for the atmosphere's refraction) and azimuth (degrees from north, 90 east). 'ra' and 'dec', if given, are set to its right ascension (hours) and declination (degrees) at that time.

The last form is for any other object: 'ra2000' and 'dec2000' are its right ascension (hours) and declination (degrees) for the epoch J2000.0, 'pm_ra' and 'pm_dec' its proper motion in arcseconds a year (0 if they are not given), and 'ra' and 'dec' variables for its position at that time, after its precession and proper motion.

```
ASTRO Vega alt, az
IF alt > 0 THEN PRINT "Vega is"; alt; " degrees up, at"; az; " degrees"
```

## SLEW

```
SLEW dRA1, dDec1, flipRA, flipDec, dRA2, dDec2, mountRA, mountDec, ra, dec, lst
```

Works out the moves of a polar-aligned German equatorial mount from where it points (right ascension 'mountRA' in hours, declination 'mountDec' in degrees) to a target ('ra' and 'dec', from STAR or ASTRO), at local sidereal time 'lst' (from LOCATION). The first six arguments are variables that are set to the moves:

{widths: 20,80}
| Variable | Move |
|---|---|
| dRA1, dDec1 | The first move, in hours of right ascension (-12 to 12, positive to the east) and degrees of declination (positive to the north). |
| flipRA, flipDec | The flip, if the target is on the other side of the meridian: 12 hours and 180 degrees (from the east side to the west, or -12 and -180 from the west to the east); both 0 if there is none. |
| dRA2, dDec2 | The move after the flip, from the meridian to the target (dDec2 is always 0: the flip keeps the declination). Both 0 if there is no flip. |

With a flip, the first move goes to the meridian and sets the declination; the flip swings the tube over the top of the mount, keeping the counterweight down; the second move finishes in right ascension. The direction of the declination motor is reversed by the flip, which the sign of 'flipDec' allows for. To turn hours into degrees for a motor, multiply by 15. The program keeps track of where the mount points and passes it to each SLEW, and should check that the target is above the horizon first.

## GPS(SIDEREAL) and GPS(JULIAN)

GPS(SIDEREAL) returns the local sidereal time of the GPS receiver's last fix, in hours, and GPS(JULIAN) the Julian centuries since J2000.0 at that time.

## The Calculations

The time is turned into Julian centuries since J2000.0, and the local sidereal time comes from Greenwich mean sidereal time and the longitude. The Moon's position comes from a shortened ELP-2000/82 series and the planets' from simplified VSOP87 elements, each corrected for where the observer stands on the Earth (parallax). A star's catalogue position is moved by its proper motion and the precession of the Earth's axis since J2000.0. Right ascension and declination are turned into altitude and azimuth, and the altitude corrected for the refraction of a standard atmosphere.

## The Catalogue

{widths: 33,33,34}
| | | |
|---|---|---|
| Achernar | Acrux | Alcyone |
| Aldebaran | Algenib | Algieba |
| Algol | Alhajoth | Alhena |
| Almaak | Alnair | Alnilam |
| Alnitak | Alphard | Alpheratz |
| Alpherg | Alrescha | Alsephina |
| Alshain | Altair | Aludra |
| Andromeda Galaxy | Antares | Arcturus |
| Aspidiske | Bellatrix | Betelgeuse |
| Bodes Galaxy | Canopus | Capella |
| Caph | Castor | Cigar Galaxy |
| Deneb | Denebola | Dubhe |
| Elnath | Eltanin | Enif |
| Fomalhaut | Gacrux | Hadar |
| Homam | Kaus Australis | Kochab |
| Kornephoros | Large Magellanic Cloud | Lesath |
| Markab | Menkalinan | Mimosa |
| Mintaka | Mirfak | Nunki |
| Peacock | Polaris | Pollux |
| Procyon | Rasalgethi | Rasalhague |
| Regulus | Rigel | Rigil Kent |
| Ruchbah | Sabik | Sadalmelik |
| Sadalsuud | Sadr | Saiph |
| Scheat | Shaula | Shedir |
| Sirius | Small Magellanic Cloud | Sombrero Galaxy |
| Spica | Suhail | Tarazed |
| Triangulum Galaxy | Vega | Whirlpool Galaxy |
| Zubenelgenubi | Zubeneschamali | |
