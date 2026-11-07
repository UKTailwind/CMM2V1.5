\ solar_eclipse.fs        local circumstances of solar eclipses, for zeptoforth
\
\ Port of solar_eclipse.bas (Micromite eXtreme version, March 14, 2017).
\ Every float operation is kept in the BASIC's order, so the output can be
\ compared digit for digit with MMBasic.  The word names follow the BASIC subs.
\
\ NEEDS A TRUE 64-BIT DOUBLE-PRECISION BUILD OF ZEPTOFORTH.  Single precision
\ steps by 0.25 day at a julian date near 2451545, and S31.32 overflows on values
\ such as the sun's distance squared.
\
\ To use it:
\   1. Section 1 calls the Colour Maximite 2 port's double-precision words.  For
\      another build, change them to its own.
\   2. Send this file to the board.
\   3. Type  solar-eclipse  for the same prompts as the BASIC program, or
\      reference-run  to run the benchmark inputs without prompts.
\
\ reference-run answers 12,1,2000 / 39,40,36 / -104,57,12 / 1644 / 30.  The
\ reference results, MMBasic's, are in solar_eclipse.out beside this file.  From
\ the line "searching for solar eclipse" to the time line they should match this
\ program's output byte for byte.  They print these lines for it:
\   UTC julian day      2451904.14541560   (begin)
\   UTC julian day      2451904.19690359   (greatest)
\   UTC julian day      2451904.25420764   (end)
\   event duration      2.61100904 hours
\
\ Oddities of the BASIC that are kept on purpose:
\   - pl-c0 has "pl + 3.67446 + Sin(4 * d - f)": '+' where the other lines have
\     '*'.  The term "2 * ju - 5 * l + l" in rm-p0 is also kept as written.
\   - xmod rounds both operands to integers, as MMBasic's Mod does, so the
\     nutation arguments are whole arcseconds before they are scaled.
\   - xasin and xacos use MMBasic's formula, not a library asin or acos.
\ Left out because the BASIC never calls them: vcross, transpose.

begin-module eclipse

\ ---------------------------------------------------------------------------
\ 1. DOUBLE-PRECISION WORDS: the only part to edit for your build
\ ---------------------------------------------------------------------------
\ The program does all its float work through these 17 words.  The names on the
\ right are the words of the Colour Maximite 2 port: replace them with your
\ build's words, or loading stops at the first one.
\
\ A double (x below) is assumed to be two cells on the data stack, so that 2dup,
\ 2swap, 2@, 2!, 2constant, 2value and D: locals all carry it.  Flags are the
\ usual 0 and -1.  Angles are in radians.

\ your-float64-module import

: x+      ( x1 x2 -- x3 )   dv+ ;
: x-      ( x1 x2 -- x3 )   dv- ;
: x*      ( x1 x2 -- x3 )   dv* ;
: x/      ( x1 x2 -- x3 )   dv/ ;
: xnegate ( x1 -- x2 )      dvnegate ;
: xabs    ( x1 -- x2 )      dvabs ;
: xsqrt   ( x1 -- x2 )      dvsqrt ;
: xsin    ( x1 -- x2 )      dvsin ;
: xcos    ( x1 -- x2 )      dvcos ;
: xtan    ( x1 -- x2 )      dvtan ;
: xatan   ( x1 -- x2 )      dvatan ;
: xfloor  ( x1 -- x2 )      dvfloor ;  \ largest whole number <= x1, as a double
: xtrunc  ( x1 -- x2 )      dvnfract ;  \ x1 rounded toward zero, as a double
: x<      ( x1 x2 -- flag ) dv< ;
: x=      ( x1 x2 -- flag ) dv= ;
: n>x     ( n -- x )        n>dv ;     \ signed 32-bit integer to double
: x>n     ( x -- n )        dv>n ;     \ toward zero; only used with |x| < 2^31

\ ---------------------------------------------------------------------------
\ 2. Helpers built from section 1
\ ---------------------------------------------------------------------------

0 n>x 2constant 0x
1 n>x 2constant 1x
2 n>x 2constant 2x
3 n>x 2constant 3x
4 n>x 2constant 4x
5 n>x 2constant 5x
6 n>x 2constant 6x
8 n>x 2constant 8x
10 n>x 2constant 10x
12 n>x 2constant 12x
16 n>x 2constant 16x
18 n>x 2constant 18x
24 n>x 2constant 24x
31 n>x 2constant 31x
60 n>x 2constant 60x
3600 n>x 2constant 3600x
-1 n>x 2constant -1x
1x 2x x/ 2constant halfx

: x>   ( x1 x2 -- flag ) 2swap x< ;
: x<=  ( x1 x2 -- flag ) x> 0= ;
: x>=  ( x1 x2 -- flag ) x< 0= ;
: x<>  ( x1 x2 -- flag ) x= 0= ;
: x0<  ( x -- flag ) 0x x< ;
: x0>  ( x -- flag ) 0x x> ;
: x0=  ( x -- flag ) 0x x= ;
: x0<> ( x -- flag ) 0x x<> ;

\ Sgn()
: xsgn ( x1 -- x2 )
  2dup x0> if 2drop 1x exit then
  x0< if -1x else 0x then
;

\ float to integer conversion as MMBasic does it: half away from zero
: xround ( x1 -- x2 ) 2dup x0< if halfx x- else halfx x+ then xtrunc ;

\ Mod: both operands rounded to integers, result takes the dividend's sign
: xmod { D: a D: b -- D: r }
  a xround to a
  b xround to b
  a b a b x/ xtrunc x* x-
;

\ 10^n as a double; exact for the n used here (n <= 22)
: 10^ ( n -- x ) 1x rot 0 ?do 10x x* loop ;

: digit>x ( c -- x ) [char] 0 - n>x ;

: parse-exp { addr len -- n }
  false 0 { neg? n }
  len 0> if
    addr c@ [char] - = if
      true to neg? 1 +to addr -1 +to len
    else
      addr c@ [char] + = if 1 +to addr -1 +to len then
    then
  then
  begin len 0> while
    n 10 * addr c@ [char] 0 - + to n
    1 +to addr -1 +to len
  repeat
  neg? if n negate else n then
;

\ Text to double, by MMBasic's own method: plain decimals are all their digits
\ as an integer divided by a power of ten; a number with an exponent is its
\ integer and fraction parts added and then scaled.
: str>x { addr len -- x }
  false false false { neg? point? exp? }
  0 0 0 { k e c }
  0x 0x 0x 1x { D: m D: ip D: frac D: fdiv }
  begin len 0> if addr c@ bl = else false then while
    1 +to addr -1 +to len
  repeat
  len 0> if
    addr c@ [char] - = if
      true to neg? 1 +to addr -1 +to len
    else
      addr c@ [char] + = if 1 +to addr -1 +to len then
    then
  then
  begin len 0> while
    addr c@ to c
    c [char] 0 >= c [char] 9 <= and if
      m 10x x* c digit>x x+ to m
      point? if
        1 +to k
        fdiv 10x x* to fdiv
        frac c digit>x fdiv x/ x+ to frac
      else
        ip 10x x* c digit>x x+ to ip
      then
      1 +to addr -1 +to len
    else
      c [char] . = point? 0= and if
        true to point?
        1 +to addr -1 +to len
      else
        c [char] e = c [char] E = or if
          addr 1+ len 1- parse-exp to e
          true to exp?
        then
        0 to len
      then
    then
  repeat
  exp? if
    ip frac x+ e abs 10^ e 0< if x/ else x* then
  else
    m k 10^ x/
  then
  neg? if xnegate then
;

\ a float literal, in or out of a definition
: x# [immediate] ( "number" -- x ) token str>x state @ if swap lit, lit, then ;

\ store a double so that 2@ reads it back
: x, ( x -- ) , , ;

\ store n numbers parsed from the rest of the line
: xrow, ( n "number"... -- ) 0 ?do token str>x x, loop ;

\ 3-element vectors and the 3x3 matrix, indexed from 1 as in the BASIC
: v@ ( vec i -- x ) 1- 8 * + 2@ ;
: v! ( x vec i -- ) 1- 8 * + 2! ;

\ ---------------------------------------------------------------------------
\ 3. Constants and data
\ ---------------------------------------------------------------------------

x# 3.141592653589793 2constant xpi

2x xpi x* 2constant pi2
halfx xpi x* 2constant pidiv2
xpi x# 180.0 x/ 2constant dtr
x# 180.0 xpi x/ 2constant rtd
xpi x# 648000.0 x/ 2constant atr
x# 206264.8062470964 2constant seccon

x# 6378.14 2constant reqm             \ equatorial radius of the earth (km)
1x x# 298.257 x/ 2constant flat       \ earth flattening factor
x# 696000.0 2constant radsun          \ radius of the sun (km)
x# 1738.0 2constant radmoon           \ radius of the moon (km)

\ solar ephemeris data: xsl xsr xsa xsb
create xs-data
4 xrow, 403406 0 4.721964 1.621043
4 xrow, 195207 -97597 5.937458 62830.348067
4 xrow, 119433 -59715 1.115589 62830.821524
4 xrow, 112392 -56188 5.781616 62829.634302
4 xrow, 3891 -1556 5.5474 125660.5691
4 xrow, 2819 -1126 1.5120 125660.9845
4 xrow, 1721 -861 4.1897 62832.4766
4 xrow, 0 941 1.163 0.813
4 xrow, 660 -264 5.415 125659.310
4 xrow, 350 -163 4.315 57533.850
4 xrow, 334 0 4.553 -33.931
4 xrow, 314 309 5.198 777137.715
4 xrow, 268 -158 5.989 78604.191
4 xrow, 242 0 2.911 5.412
4 xrow, 234 -54 1.423 39302.098
4 xrow, 158 0 0.061 -34.861
4 xrow, 132 -93 2.317 115067.698
4 xrow, 129 -20 3.193 15774.337
4 xrow, 114 0 2.828 5296.670
4 xrow, 99 -47 0.52 58849.27
4 xrow, 93 0 4.65 5296.11
4 xrow, 86 0 4.35 -3980.70
4 xrow, 78 -33 2.75 52237.69
4 xrow, 72 -32 4.50 55076.47
4 xrow, 68 0 3.23 261.08
4 xrow, 64 -10 1.22 15773.85
4 xrow, 46 -16 0.14 188491.03
4 xrow, 38 0 3.44 -7756.55
4 xrow, 37 0 4.37 264.89
4 xrow, 32 -24 1.14 117906.27
4 xrow, 29 -13 2.84 55075.75
4 xrow, 28 0 5.96 -7961.39
4 xrow, 27 -9 5.09 188489.81
4 xrow, 27 0 1.72 2132.19
4 xrow, 25 -17 2.56 109771.03
4 xrow, 24 -11 1.92 54868.56
4 xrow, 21 0 0.09 25443.93
4 xrow, 21 31 5.98 -55731.43
4 xrow, 20 -10 4.03 60697.74
4 xrow, 18 0 4.27 2132.79
4 xrow, 17 -12 0.79 109771.63
4 xrow, 14 0 4.24 -7752.82
4 xrow, 13 -5 2.01 188491.91
4 xrow, 13 0 2.65 207.81
4 xrow, 13 0 4.98 29424.63
4 xrow, 12 0 0.93 -7.99
4 xrow, 10 0 2.21 46941.14
4 xrow, 10 0 3.59 -68.29
4 xrow, 10 0 1.50 21463.25
4 xrow, 10 -9 2.55 157208.40

: xs-row ( i -- addr ) 1- 32 * xs-data + ;
: xsl ( i -- x ) xs-row 2@ ;
: xsr ( i -- x ) xs-row 8 + 2@ ;
: xsa ( i -- x ) xs-row 16 + 2@ ;
: xsb ( i -- x ) xs-row 24 + 2@ ;

\ subset of IAU2000 nutation data
create xnut-data
11 xrow, 0 0 0 0 1 -172064161 -174666 92052331 9086 33386 15377
11 xrow, 0 0 2 -2 2 -13170906 -1675 5730336 -3015 -13696 -4587
11 xrow, 0 0 2 0 2 -2276413 -234 978459 -485 2796 1374
11 xrow, 0 0 0 0 2 2074554 207 -897492 470 -698 -291
11 xrow, 0 1 0 0 0 1475877 -3633 73871 -184 11817 -1924
11 xrow, 0 1 2 -2 2 -516821 1226 224386 -677 -524 -174
11 xrow, 1 0 0 0 0 711159 73 -6750 0 -872 358
11 xrow, 0 0 2 0 1 -387298 -367 200728 18 380 318
11 xrow, 1 0 2 0 2 -301461 -36 129025 -63 816 367
11 xrow, 0 -1 2 -2 2 215829 -494 -95929 299 111 132
11 xrow, 0 0 2 -2 1 128227 137 -68982 -9 181 39
11 xrow, -1 0 2 0 2 123457 11 -53311 32 19 -4
11 xrow, -1 0 0 2 0 156994 10 -1235 0 -168 82

: xnut ( i j -- x ) 1- 11 * swap 1- + 8 * xnut-data + 2@ ;

\ leap second data: utc julian date, seconds
create leap-data
2 xrow, 2441317.5 10.0
2 xrow, 2441499.5 11.0
2 xrow, 2441683.5 12.0
2 xrow, 2442048.5 13.0
2 xrow, 2442413.5 14.0
2 xrow, 2442778.5 15.0
2 xrow, 2443144.5 16.0
2 xrow, 2443509.5 17.0
2 xrow, 2443874.5 18.0
2 xrow, 2444239.5 19.0
2 xrow, 2444786.5 20.0
2 xrow, 2445151.5 21.0
2 xrow, 2445516.5 22.0
2 xrow, 2446247.5 23.0
2 xrow, 2447161.5 24.0
2 xrow, 2447892.5 25.0
2 xrow, 2448257.5 26.0
2 xrow, 2448804.5 27.0
2 xrow, 2449169.5 28.0
2 xrow, 2449534.5 29.0
2 xrow, 2450083.5 30.0
2 xrow, 2450630.5 31.0
2 xrow, 2451179.5 32.0
2 xrow, 2453736.5 33.0
2 xrow, 2454832.5 34.0
2 xrow, 2456109.5 35.0
2 xrow, 2457204.5 36.0
2 xrow, 2457754.5 37.0

: jdleap ( i -- x ) 1- 16 * leap-data + 2@ ;
: leapsec ( i -- x ) 1- 16 * 8 + leap-data + 2@ ;

\ global state
0x 2value jdtdbi
0x 2value jdsaved
0x 2value jdprint
0x 2value obslat
0x 2value obslong
0x 2value obsalt
0x 2value trr
0x 2value elev-minima

\ working vectors
24 buffer: rsun
24 buffer: rmoon
24 buffer: rtmoon
24 buffer: rm2s
24 buffer: uaxis
24 buffer: um2o
24 buffer: rsite
24 buffer: rhoijk
24 buffer: rhohatijk
24 buffer: rhohatsez
40 buffer: dvec
72 buffer: tmatrix

: m@ ( i j -- x ) swap 1- 3 * + 1- 8 * tmatrix + 2@ ;
: m! ( x i j -- ) swap 1- 3 * + 1- 8 * tmatrix + 2! ;

\ ---------------------------------------------------------------------------
\ 4. Maths helpers
\ ---------------------------------------------------------------------------

\ modulo 2 pi
: modulo { D: x -- D: a }
  x pi2 x pi2 x/ xtrunc x* x- { D: a }
  a x0< if a pi2 x+ to a then
  a
;

\ four quadrant inverse tangent; a = sine, b = cosine, result 0 to 2 pi
: atan3 { D: a D: b -- D: r }
  a xabs x# 1.0e-10 x< if
    1x b xsgn x- pidiv2 x* exit
  then
  2x a xsgn x- pidiv2 x* { D: c }
  b xabs x# 1.0e-10 x< if c exit then
  c a xsgn b xsgn x* a b x/ xatan xabs pidiv2 x- x* x+
;

\ MMBasic's arcsinus()
: arcsinus { D: x -- D: r }
  2x x 1x 1x x x x* x- xsqrt x+ x/ xatan x*
;

\ ASin()
: xasin { D: f -- D: r }
  f 1x x= if pidiv2 exit then
  f -1x x= if pidiv2 xnegate exit then
  f arcsinus
;

\ ACos()
: xacos { D: f -- D: r }
  f 1x x= if 0x exit then
  f -1x x= if xpi exit then
  pidiv2 f arcsinus x-
;

: vecmag { a -- D: m }
  a 1 v@ 2dup x*
  a 2 v@ 2dup x* x+
  a 3 v@ 2dup x* x+
  xsqrt
;

: uvector { a b -- }
  a vecmag { D: amag }
  4 1 do
    amag x0<> if a i v@ amag x/ else 0x then b i v!
  loop
;

: vdot { a b -- D: c }
  0x
  4 1 do a i v@ b i v@ x* x+ loop
;

\ c = tmatrix * b
: matxvec { b c -- }
  4 1 do
    0x
    4 1 do j i m@ b i v@ x* x+ loop
    c i v!
  loop
;

\ ---------------------------------------------------------------------------
\ 5. Nutation, obliquity and sidereal time
\ ---------------------------------------------------------------------------

\ low precision nutation based on iau 2000a; results are in arcseconds
: nut2000-lp { D: jdate -- D: dpsi D: deps }
  x# 1296000.0 { D: rev }
  jdate x# 2451545.0 x- x# 36525.0 x/ { D: t }

  \ fundamental (delaunay) arguments from simon et al. (1994)

  \ mean anomaly of the moon
  t x# 1717915923.2178 t x# 31.8792 t x# 0.051635 t x# -0.00024470
  x* x+ x* x+ x* x+ x* rev xmod x# 485868.249036 2swap x+ seccon x/ { D: el }

  \ mean anomaly of the sun
  t x# 129596581.0481 t x# -0.5532 t x# 0.000136 t x# -0.00001149
  x* x+ x* x+ x* x+ x* rev xmod x# 1287104.79305 2swap x+ seccon x/ { D: elp }

  \ mean argument of the latitude of the moon
  t x# 1739527262.8478 t x# -12.7512 t x# -0.001037 t x# 0.00000417
  x* x+ x* x+ x* x+ x* rev xmod x# 335779.526232 2swap x+ seccon x/ { D: f }

  \ mean elongation of the moon from the sun
  t x# 1602961601.2090 t x# -6.3706 t x# 0.006593 t x# -0.00003169
  x* x+ x* x+ x* x+ x* rev xmod x# 1072260.70369 2swap x+ seccon x/ { D: d }

  \ mean longitude of the ascending node of the moon
  t x# -6962890.5431 t x# 7.4722 t x# 0.007702 t x# -0.00005939
  x* x+ x* x+ x* x+ x* rev xmod x# 450160.398036 2swap x+ seccon x/ { D: omega }

  0x 0x 0x { D: dpsi D: deps D: arg }

  \ sum nutation series terms
  1 13 do
    1 i xnut el x* 2 i xnut elp x* x+ 3 i xnut f x* x+
    4 i xnut d x* x+ 5 i xnut omega x* x+ to arg

    6 i xnut 7 i xnut t x* x+ arg xsin x*
    10 i xnut arg xcos x* x+ dpsi x+ to dpsi

    8 i xnut 9 i xnut t x* x+ arg xcos x*
    11 i xnut arg xsin x* x+ deps x+ to deps
  -1 +loop

  x# 1.0e-7 dpsi x* to dpsi
  x# 1.0e-7 deps x* to deps

  \ out-of-phase component of the principal (18.6-year) term
  dpsi x# 0.0033 omega xcos x* x+
  deps x# 0.0015 omega xsin x* x+
;

\ nutations and true obliquity, all in radians
: obliq-lp { D: jdate -- D: dpsi D: deps D: obliq }
  jdate xfloor { D: tjdh }
  jdate tjdh x- { D: tjdl }
  tjdh x# 2451545.0 x- x# 36525.0 x/ { D: th }
  tjdl x# 36525.0 x/ { D: tl }
  th tl x+ { D: t }
  t t x* { D: t2 }
  t2 t x* { D: t3 }
  jdate nut2000-lp { D: dpsi D: deps }
  x# 84381.4480 x# 46.8150 t x* x- x# 0.00059 t2 x* x- x# 0.001813 t3 x* x+
  { D: obm }
  obm deps x+ { D: obt }
  atr deps x* to deps
  atr dpsi x* to dpsi
  dpsi deps atr obt x*
;

\ greenwich apparent sidereal time (radians)
: gast2 { D: jdate -- D: gast }
  jdate xfloor { D: tjdh }
  jdate tjdh x- { D: tjdl }
  tjdh x# 2451545.0 x- x# 36525.0 x/ { D: th }
  tjdl x# 36525.0 x/ { D: tl }
  th tl x+ { D: t }
  t t x* { D: t2 }
  t2 t x* { D: t3 }
  jdate nut2000-lp { D: dpsi D: deps }
  x# 84381.4480 x# 46.8150 t x* x- x# 0.00059 t2 x* x- x# 0.001813 t3 x* x+
  { D: obm }
  obm deps x+ { D: obliq }

  \ equation of the equinoxes in seconds of time
  dpsi x# 15.0 x/ obliq seccon x/ xcos x* { D: eqeq }

  eqeq x# 6.2e-6 t3 x* x- x# 0.093104 t2 x* x+ x# 67310.54841 x+
  x# 8640184.812866 tl x* x+ x# 3155760000.0 tl x* x+
  x# 8640184.812866 th x* x+ x# 3155760000.0 th x* x+ { D: st }

  \ modulo 24 hours
  st x# 3600.0 x/ { D: x }
  x 24x x 24x x/ xtrunc x* x- { D: gast }
  gast x0< if gast 24x x+ to gast then

  \ convert to radians
  pi2 gast 24x x/ x*
;

\ ---------------------------------------------------------------------------
\ 6. Geocentric position of the moon
\ ---------------------------------------------------------------------------
\ The series below come line for line from the BASIC, in its order.

0x 2value t1
0x 2value t2
0x 2value t3
0x 2value t4
0x 2value ll
0x 2value d
0x 2value lp
0x 2value l
0x 2value f
0x 2value t
0x 2value ve
0x 2value ma
0x 2value ju

: moon-time ( jdate -- )
  x# 2451545.0 x- x# 36525.0 x/ to t1
  t1 t1 x* to t2
  t1 t1 x* t1 x* to t3
  t1 t1 x* t1 x* t1 x* to t4
;

\ fundamental arguments (radians)
: moon-args ( -- )
  dtr x# 218 18x x# 60 x* x# 59.95571 x+ x# 3600 x/ x+ x* to ll
  ll atr x# 1732564372.83264 t1 x* x# 4.7763 t2 x* x- x# .006681 t3 x* x+ x# 0.00005522 t4 x* x- x* x+ modulo to ll
  dtr x# 297 x# 51 x# 60 x* x# .73512 x+ x# 3600 x/ x+ x* to d
  d atr x# 1602961601.4603 t1 x* x# 5.8681 t2 x* x- x# .006595 t3 x* x+ x# 0.00003184 t4 x* x- x* x+ modulo to d
  dtr x# 357 x# 31 x# 60 x* x# 44.79306 x+ x# 3600 x/ x+ x* to lp
  lp atr x# 129596581.0474 t1 x* x# .5529 t2 x* x- x# 0.000147 t3 x* x+ x* x+ modulo to lp
  dtr x# 134 x# 57 x# 60 x* x# 48.28096 x+ x# 3600 x/ x+ x* to l
  l atr x# 1717915923.4728 t1 x* x# 32.3893 t2 x* x+ x# .051651 t3 x* x+ x# 0.0002447 t4 x* x- x* x+ modulo to l
  dtr x# 93 16x x# 60 x* x# 19.55755 x+ x# 3600 x/ x+ x* to f
  f atr x# 1739527263.0983 t1 x* x# 12.2505 t2 x* x- x# .001021 t3 x* x- x# 0.00000417 t4 x* x+ x* x+ modulo to f
  dtr x# 100 x# 27 x# 60 x* x# 59.22059 x+ x# 3600 x/ x+ x* to t
  t atr x# 129597742.2758 t1 x* x# .0202 t2 x* x- x# .000009 t3 x* x+ x# 0.00000015 t4 x* x+ x* x+ modulo to t
  dtr x# 181 x# 58 x# 60 x* x# 47.28305 x+ x# 3600 x/ x+ x* to ve
  ve atr x# 210664136.43355 x* t1 x* x+ modulo to ve
  dtr x# 355 x# 25 x# 60 x* x# 59.78866 x+ x# 3600 x/ x+ x* to ma
  ma atr x# 68905077.59284 x* t1 x* x+ modulo to ma
  dtr x# 34 x# 21 x# 60 x* x# 5.34212 x+ x# 3600 x/ x+ x* to ju
  ju atr x# 10925660.42861 x* t1 x* x+ modulo to ju
;

\ geocentric distance (km)
\ a(c,0,r) series
: rm-c0 ( -- rm )
  x# 385000.52899
  x# 20905.35504 l pidiv2 x+ xsin x* x-
  x# 3699.11092 2x d x* l x- pidiv2 x+ xsin x* x-
  x# 2955.96756 2x d x* pidiv2 x+ xsin x* x-
  x# 569.92512 2x l x* pidiv2 x+ xsin x* x-
  x# 246.15848 2x d x* 2x l x* x- pidiv2 x+ xsin x* x+
  x# 204.58598 2x d x* lp x- pidiv2 x+ xsin x* x-
  x# 170.73308 2x d x* l x+ pidiv2 x+ xsin x* x-
  x# 152.13771 2x d x* lp x- l x- pidiv2 x+ xsin x* x-
  x# 129.62014 lp l x- pidiv2 x+ xsin x* x-
  x# 108.7427 d pidiv2 x+ xsin x* x+
  x# 104.75523 lp l x+ pidiv2 x+ xsin x* x+
  x# 79.66056 l 2x f x* x- pidiv2 x+ xsin x* x+
  x# 48.8883 lp pidiv2 x+ xsin x* x+
  x# 34.78252 4x d x* l x- pidiv2 x+ xsin x* x-
  x# 30.82384 2x d x* lp x+ pidiv2 x+ xsin x* x+
  x# 24.20848 2x d x* lp x+ l x- pidiv2 x+ xsin x* x+
  x# 23.21043 3x l x* pidiv2 x+ xsin x* x-
  x# 21.63634 4x d x* 2x l x* x- pidiv2 x+ xsin x* x-
  x# 16.67471 d lp x+ pidiv2 x+ xsin x* x-
  x# 14.40269 2x d x* 3x l x* x- pidiv2 x+ xsin x* x+
  x# 12.8314 2x d x* lp x- l x+ pidiv2 x+ xsin x* x-
  x# 11.64995 4x d x* pidiv2 x+ xsin x* x-
  x# 10.44476 2x d x* 2x l x* x+ pidiv2 x+ xsin x* x-
  x# 10.32111 2x d x* 2x f x* x- pidiv2 x+ xsin x* x+
  x# 10.0562 2x d x* lp x- 2x l x* x- pidiv2 x+ xsin x* x+
  x# 9.88445 2x d x* 2x lp x* x- pidiv2 x+ xsin x* x-
  x# 8.75156 2x d x* l x- 2x f x* x- pidiv2 x+ xsin x* x+
  x# 8.37911 d l x- pidiv2 x+ xsin x* x-
  x# 7.00269 lp 2x l x* x- pidiv2 x+ xsin x* x-
  x# 6.322 d l x+ pidiv2 x+ xsin x* x+
  x# 5.75085 lp 2x l x* x+ pidiv2 x+ xsin x* x+
  x# 4.95013 2x d x* 2x lp x* x- l x- pidiv2 x+ xsin x* x-
  x# 4.42118 2x l x* 2x f x* x- pidiv2 x+ xsin x* x-
  x# 4.13111 2x d x* l x+ 2x f x* x- pidiv2 x+ xsin x* x+
  x# 3.95798 4x d x* lp x- l x- pidiv2 x+ xsin x* x-
  x# 3.25824 3x d x* l x- pidiv2 x+ xsin x* x+
  x# 3.1483 2x f x* pidiv2 x+ xsin x* x-
  x# 2.61641 2x d x* lp x+ l x+ pidiv2 x+ xsin x* x+
  x# 2.35363 2x d x* 2x lp x* x+ l x- pidiv2 x+ xsin x* x+
  x# 2.11713 2x lp x* l x- pidiv2 x+ xsin x* x-
  x# 1.89704 4x d x* lp x- 2x l x* x- pidiv2 x+ xsin x* x-
  x# 1.73853 d 2x l x* x- pidiv2 x+ xsin x* x-
  x# 1.57139 4x d x* lp x- pidiv2 x+ xsin x* x-
  x# 1.42255 4x d x* l x+ pidiv2 x+ xsin x* x-
  x# 1.41893 3x d x* pidiv2 x+ xsin x* x-
  x# 1.16553 2x lp x* l x+ pidiv2 x+ xsin x* x+
  x# 1.11694 4x l x* pidiv2 x+ xsin x* x-
  x# 1.06567 2x lp x* pidiv2 x+ xsin x* x+
  x# .93332 d lp x+ l x+ pidiv2 x+ xsin x* x-
  x# .86243 3x d x* 2x l x* x- pidiv2 x+ xsin x* x+
  x# .85124 d lp x+ l x- pidiv2 x+ xsin x* x+
  x# .8488 2x d x* lp x- 2x l x* x+ pidiv2 x+ xsin x* x-
  x# .79563 d 2x f x* x- pidiv2 x+ xsin x* x-
  x# .77854 2x d x* 4x l x* x- pidiv2 x+ xsin x* x+
  x# .77404 2x d x* 2x l x* x- 2x f x* x+ pidiv2 x+ xsin x* x+
  x# .66968 2x d x* 3x l x* x+ pidiv2 x+ xsin x* x-
  x# .65753 2x d x* 2x lp x* x- l x+ pidiv2 x+ xsin x* x-
  x# .65706 2x d x* lp x- 2x f x* x- pidiv2 x+ xsin x* x+
  x# .59632 2x d x* l x- 2x f x* x+ pidiv2 x+ xsin x* x+
  x# .57879 4x d x* lp x+ l x- pidiv2 x+ xsin x* x+
  x# .51423 4x d x* 3x l x* x- pidiv2 x+ xsin x* x-
  x# .50792 4x d x* 2x f x* x- pidiv2 x+ xsin x* x-
  x# .49755 d lp x- pidiv2 x+ xsin x* x+
  x# .49504 2x d x* lp x- 3x l x* x- pidiv2 x+ xsin x* x+
  x# .47262 2x d x* 2x l x* x- 2x f x* x- pidiv2 x+ xsin x* x+
  x# .4225 6x d x* 2x l x* x- pidiv2 x+ xsin x* x-
  x# .42241 lp 3x l x* x- pidiv2 x+ xsin x* x-
  x# .41071 2x d x* 3x lp x* x- pidiv2 x+ xsin x* x-
  x# .37852 d 2x l x* x+ pidiv2 x+ xsin x* x+
  x# .35508 lp 3x l x* x+ pidiv2 x+ xsin x* x+
  x# .34302 2x d x* 2x lp x* x- 2x l x* x- pidiv2 x+ xsin x* x+
  x# .33463 lp l x- 2x f x* x+ pidiv2 x+ xsin x* x+
  x# .33225 d lp x+ 2x l x* x- pidiv2 x+ xsin x* x+
  x# .32334 2x d x* lp x- l x- 2x f x* x- pidiv2 x+ xsin x* x+
  x# .32176 4x d x* l x- 2x f x* x- pidiv2 x+ xsin x* x-
  x# .28663 6x d x* l x- pidiv2 x+ xsin x* x-
  x# .28399 2x d x* 2x l x* x+ 2x f x* x- pidiv2 x+ xsin x* x+
  x# .27904 4x d x* 2x lp x* x- l x- pidiv2 x+ xsin x* x-
  x# .2556 3x d x* lp x- l x- pidiv2 x+ xsin x* x+
  x# .2481 lp l x+ 2x f x* x- pidiv2 x+ xsin x* x-
  x# .24452 4x d x* lp x+ pidiv2 x+ xsin x* x+
  x# .23695 4x d x* lp x+ 2x l x* x- pidiv2 x+ xsin x* x+
  x# .21258 3x d x* lp x+ l x- pidiv2 x+ xsin x* x-
  x# .21251 2x d x* lp x+ 2x l x* x+ pidiv2 x+ xsin x* x+
  x# .20941 2x d x* lp x- l x+ 2x f x* x- pidiv2 x+ xsin x* x+
  x# .20285 4x d x* lp x- l x+ pidiv2 x+ xsin x* x-
  x# .20099 3x d x* 2x f x* x- pidiv2 x+ xsin x* x+
  x# .18567 lp 2x f x* x- pidiv2 x+ xsin x* x-
  x# .18316 6x d x* 3x l x* x- pidiv2 x+ xsin x* x-
  x# .16857 2x d x* lp x+ 3x l x* x- pidiv2 x+ xsin x* x+
  x# .15802 lp 2x f x* x+ pidiv2 x+ xsin x* x-
  x# .15707 3x d x* lp x- pidiv2 x+ xsin x* x-
  x# .14806 2x d x* 3x lp x* x- l x- pidiv2 x+ xsin x* x-
  x# .14763 2x d x* 2x lp x* x+ pidiv2 x+ xsin x* x+
  x# .14368 2x d x* lp x+ 2x l x* x- pidiv2 x+ xsin x* x+
  x# .13922 4x d x* 2x l x* x+ pidiv2 x+ xsin x* x-
  x# .13617 2x lp x* 2x l x* x- pidiv2 x+ xsin x* x-
  x# .13571 2x d x* lp x+ 2x f x* x- pidiv2 x+ xsin x* x-
  x# .12805 4x d x* 2x lp x* x- pidiv2 x+ xsin x* x-
  x# .11411 d lp x- l x- pidiv2 x+ xsin x* x+
  x# .10998 d lp x- l x+ pidiv2 x+ xsin x* x+
  x# .10887 2x d x* 2x lp x* x+ 2x l x* x- pidiv2 x+ xsin x* x-
  x# .10833 4x d x* 2x lp x* x- 2x l x* x- pidiv2 x+ xsin x* x-
  x# .10766 3x d x* lp x+ pidiv2 x+ xsin x* x-
  x# .10326 l 2x f x* x+ pidiv2 x+ xsin x* x-
  x# .09938 d 3x l x* x- pidiv2 x+ xsin x* x-
  x# .08587 6x d x* pidiv2 x+ xsin x* x-
  x# .07982 4x d x* 2x l x* x- 2x f x* x- pidiv2 x+ xsin x* x-
  x# 6.678e-02 6x d x* lp x- 2x l x* x- pidiv2 x+ xsin x* x-
  x# 6.545e-02 3x d x* l x+ pidiv2 x+ xsin x* x-
  x# .06055 d l x+ 2x f x* x- pidiv2 x+ xsin x* x+
  x# .05904 d lp x+ 2x l x* x+ pidiv2 x+ xsin x* x-
  x# .05888 5x l x* pidiv2 x+ xsin x* x-
  x# .0585 2x d x* lp x- 3x l x* x+ pidiv2 x+ xsin x* x-
  x# .05789 4x d x* lp x- 2x f x* x- pidiv2 x+ xsin x* x-
  x# .05527 2x d x* lp x+ l x+ 2x f x* x- pidiv2 x+ xsin x* x-
  x# .05293 3x d x* lp x- 2x l x* x- pidiv2 x+ xsin x* x+
  x# .05191 6x d x* lp x- l x- pidiv2 x+ xsin x* x-
  x# .05072 2x lp x* 2x l x* x+ pidiv2 x+ xsin x* x+
  x# .0502 lp 2x l x* x- 2x f x* x+ pidiv2 x+ xsin x* x-
  x# .04843 3x d x* 3x l x* x- pidiv2 x+ xsin x* x-
  x# .0474 2x d x* 5x l x* x- pidiv2 x+ xsin x* x+
  x# .04736 2x d x* lp x+ l x- 2x f x* x- pidiv2 x+ xsin x* x-
  x# .04608 2x d x* 2x lp x* x- 2x l x* x+ pidiv2 x+ xsin x* x-
  x# .04591 5x d x* 2x l x* x- pidiv2 x+ xsin x* x+
  x# .04422 2x d x* 4x l x* x+ pidiv2 x+ xsin x* x-
  x# .04316 4x d x* lp x- 3x l x* x- pidiv2 x+ xsin x* x-
  x# .04232 d l x- 2x f x* x- pidiv2 x+ xsin x* x-
  x# .03894 3x lp x* l x- pidiv2 x+ xsin x* x-
  x# .0381 3x d x* lp x+ 2x l x* x- pidiv2 x+ xsin x* x+
  x# .03734 2x d x* lp x- l x- 2x f x* x+ pidiv2 x+ xsin x* x+
  x# .03729 d 2x lp x* x+ pidiv2 x+ xsin x* x+
  x# .03682 4x d x* lp x+ l x+ pidiv2 x+ xsin x* x+
  x# .03379 d lp x+ 2x f x* x- pidiv2 x+ xsin x* x+
  x# .03265 lp 2x l x* x+ 2x f x* x- pidiv2 x+ xsin x* x+
  x# .03143 2x d x* 2x f x* x+ pidiv2 x+ xsin x* x+
  x# .03024 2x d x* lp x- 2x l x* x- 2x f x* x+ pidiv2 x+ xsin x* x+
  x# .02948 d 2x lp x* x- pidiv2 x+ xsin x* x-
  x# .02939 4x d x* 4x l x* x- pidiv2 x+ xsin x* x-
  x# .0291 2x d x* 3x l x* x- 2x f x* x- pidiv2 x+ xsin x* x+
  x# .02855 2x d x* 3x lp x* x- l x+ pidiv2 x+ xsin x* x-
  x# .02839 2x d x* 2x lp x* x- 2x f x* x- pidiv2 x+ xsin x* x+
  x# .02698 4x d x* lp x- l x- 2x f x* x- pidiv2 x+ xsin x* x-
  x# .02674 lp 4x l x* x- pidiv2 x+ xsin x* x-
  x# .02658 4x d x* 2x lp x* x+ 2x l x* x- pidiv2 x+ xsin x* x+
  x# .02471 d l x- 2x f x* x+ pidiv2 x+ xsin x* x-
  x# .02436 6x d x* lp x- 3x l x* x- pidiv2 x+ xsin x* x-
  x# .02399 4x d x* lp x+ 3x l x* x- pidiv2 x+ xsin x* x-
  x# .02368 d 3x l x* x+ pidiv2 x+ xsin x* x+
  x# .02334 2x d x* lp x- 4x l x* x- pidiv2 x+ xsin x* x+
  x# .02304 lp 4x l x* x+ pidiv2 x+ xsin x* x+
  x# .02127 3x lp x* pidiv2 x+ xsin x* x+
  x# .02079 4x d x* lp x- 2x l x* x+ pidiv2 x+ xsin x* x-
  x# .02008 2x d x* 3x l x* x- 2x f x* x+ pidiv2 x+ xsin x* x-
;

\ a(p,0,r) series
: rm-p0 ( rm -- rm' )
  x# 1.0587 2x t x* 2x ju x* x- 2x d x* x+ l x- x# 90.11969000000001 dtr x* x+ xsin x* x+
  x# .72783 18x ve x* 16x t x* x- 2x l x* x- x# 116.54311 dtr x* x+ xsin x* x+
  x# .68256 18x ve x* 16x t x* x- x# 296.54574 dtr x* x+ xsin x* x+
  x# .59827 3x ve x* 3x t x* x- 2x d x* x+ l x- x# 89.98187 dtr x* x+ xsin x* x+
  x# .45648 ll l x+ f x- x# 270.00126 dtr x* x+ xsin x* x+
  x# .45276 ll l x- f x- x# 90.00128 dtr x* x+ xsin x* x+
  x# .41011 2x t x* 3x ju x* x- 2x d x* x+ l x- x# 280.06924 dtr x* x+ xsin x* x+
  x# .20497 t ju x- 2x d x* x- x# 91.79862 dtr x* x+ xsin x* x+
  x# .20473 18x ve x* 16x t x* x- 2x d x* x- l x- x# 116.54222 dtr x* x+ xsin x* x+
  x# .20367 18x ve x* 16x t x* x- 2x d x* x+ l x- x# 296.54299 dtr x* x+ xsin x* x+
  x# .16644 2x ve x* 2x t x* x- 2x d x* x- x# 90.36386 dtr x* x+ xsin x* x+
  x# .1578 4x t x* 8x ma x* x- 3x ju x* x+ l x+ x# 194.98833 dtr x* x+ xsin x* x+
  x# .1578 4x t x* 8x ma x* x- 3x ju x* x+ l x- x# 14.98841 dtr x* x+ xsin x* x+
  x# .15751 t ju x- 2x d x* x- l x+ x# 91.74578 dtr x* x+ xsin x* x+
  x# .1445 2x t x* 2x ju x* x- l x- x# 89.97863 dtr x* x+ xsin x* x+
  x# .13811 ve t x- l x- x# 270.00993 dtr x* x+ xsin x* x+
  x# .13477 18x ve x* 16x t x* x- 2x d x* x- x# 116.53978 dtr x* x+ xsin x* x+
  x# .12671 18x ve x* 16x t x* x- 2x d x* x+ 2x l x* x- x# 296.54238 dtr x* x+ xsin x* x+
  x# .12666 t ju x- l x- x# 91.22751 dtr x* x+ xsin x* x+
  x# .12362 ve t x- 2x d x* x- x# 269.98523 dtr x* x+ xsin x* x+
  x# .12047 2x ve x* 2x t x* x- 2x d x* x+ l x- x# 269.99692 dtr x* x+ xsin x* x+
  x# .11998 ve t x- l x+ x# 90.01606 dtr x* x+ xsin x* x+
  x# .11617 2x ve x* 2x t x* x- 2x d x* x- l x+ x# 90.31081 dtr x* x+ xsin x* x+
  x# .11256 4x t x* 8x ma x* x- 3x ju x* x+ 2x d x* x+ l x- x# 197.11421 dtr x* x+ xsin x* x+
  x# .11251 4x t x* 8x ma x* x- 3x ju x* x+ 2x d x* x- l x+ x# 17.11263 dtr x* x+ xsin x* x+
  x# .11226 4x t x* 8x ma x* x- 3x ju x* x+ 2x d x* x+ x# 196.69224 dtr x* x+ xsin x* x+
  x# .11216 4x t x* 8x ma x* x- 3x ju x* x+ 2x d x* x- x# 16.68897 dtr x* x+ xsin x* x+
  x# .10689 ll 2x d x* x+ f x- x# 270.00092 dtr x* x+ xsin x* x+
  x# .10504 t ju x- l x+ x# 271.06726 dtr x* x+ xsin x* x+
  x# .1006 ve t x- 2x d x* x- l x+ x# 269.98452 dtr x* x+ xsin x* x+
  x# 9.932e-02 3x ve x* 3x t x* x- l x- x# 90.1054 dtr x* x+ xsin x* x+
  x# .09554 ll 2x d x* x- f x- x# 90.00096 dtr x* x+ xsin x* x+
  x# .08508 ll f x- x# 270.00061 dtr x* x+ xsin x* x+
  x# 7.9450e-02 4x ve x* 4x t x* x- 2x d x* x+ l x- x# 89.99224 dtr x* x+ xsin x* x+
  x# .07725 2x t x* 3x ju x* x- l x- x# 280.16516 dtr x* x+ xsin x* x+
  x# 7.054e-02 6x ve x* 8x t x* x- 2x d x* x+ l x- x# 77.22087 dtr x* x+ xsin x* x+
  x# 6.313e-02 2x ju x* 5x l x* x- l x+ x# 256.56163 dtr x* x+ xsin x* x+
;

\ a(p,1,r) series
: rm-p1 ( rm -- rm' )
  x# .51395 t1 x* 2x d x* lp x- pidiv2 x+ xsin x* x+
  x# .38245 t1 x* 2x d x* lp x- l x- pidiv2 x+ xsin x* x+
  x# .32654 t1 x* lp l x- pidiv2 x+ xsin x* x+
  x# .26396 t1 x* lp l x+ x# 270 dtr x* x+ xsin x* x+
  x# .12302 t1 x* lp x# 270 dtr x* x+ xsin x* x+
  x# .07754 t1 x* 2x d x* lp x+ x# 270 dtr x* x+ xsin x* x+
  x# .06068 t1 x* 2x d x* lp x+ l x- x# 270 dtr x* x+ xsin x* x+
  x# .0497 t1 x* 2x d x* 2x lp x* x- pidiv2 x+ xsin x* x+
  x# .04194 t1 x* d lp x+ pidiv2 x+ xsin x* x+
  x# .03222 t1 x* 2x d x* lp x- l x+ pidiv2 x+ xsin x* x+
  x# .02529 t1 x* 2x d x* lp x- 2x l x* x- x# 270 dtr x* x+ xsin x* x+
  x# .0249 t1 x* 2x d x* 2x lp x* x- l x- pidiv2 x+ xsin x* x+
  x# .00149 t2 x* 2x d x* lp x- pidiv2 x+ xsin x* x+
  x# .00111 t2 x* 2x d x* lp x- l x- pidiv2 x+ xsin x* x+
;

\ geocentric ecliptic longitude (arc seconds)
\ a(c,0,v) series
: dv-c0 ( -- dv )
  x# 22639.58578 l xsin x*
  x# 4586.4383 2x d x* l x- xsin x* x+
  x# 2369.91394 2x d x* xsin x* x+
  x# 769.02571 2x l x* xsin x* x+
  x# 666.4171 lp xsin x* x-
  x# 411.59567 2x f x* xsin x* x-
  x# 211.65555 2x d x* 2x l x* x- xsin x* x+
  x# 205.43582 2x d x* lp x- l x- xsin x* x+
  x# 191.9562 2x d x* l x+ xsin x* x+
  x# 164.72851 2x d x* lp x- xsin x* x+
  x# 147.32129 lp l x- xsin x* x-
  x# 124.98812 d xsin x* x-
  x# 109.38029 lp l x+ xsin x* x-
  x# 55.17705 2x d x* 2x f x* x- xsin x* x+
  x# 45.0996 l 2x f x* x+ xsin x* x-
  x# 39.53329 l 2x f x* x- xsin x* x+
  x# 38.42983 4x d x* l x- xsin x* x+
  x# 36.12381 3x l x* xsin x* x+
  x# 30.77257 4x d x* 2x l x* x- xsin x* x+
  x# 28.39708 2x d x* lp x+ l x- xsin x* x-
  x# 24.35821 2x d x* lp x+ xsin x* x-
  x# 18.58471 d l x- xsin x* x-
  x# 17.95446 d lp x+ xsin x* x+
  x# 14.53027 2x d x* lp x- l x+ xsin x* x+
  x# 14.3797 2x d x* 2x l x* x+ xsin x* x+
  x# 13.89906 4x d x* xsin x* x+
  x# 13.19406 2x d x* 3x l x* x- xsin x* x+
  x# 9.67905 lp 2x l x* x- xsin x* x-
  x# 9.36586 2x d x* l x- 2x f x* x+ xsin x* x-
  x# 8.60553 2x d x* lp x- 2x l x* x- xsin x* x+
  x# 8.45310 d l x+ xsin x* x-
  x# 8.05016 2x d x* 2x lp x* x- xsin x* x+
  x# 7.63015 lp 2x l x* x+ xsin x* x-
  x# 7.44749 2x lp x* xsin x* x-
  x# 7.37119 2x d x* 2x lp x* x- l x- xsin x* x+
  x# 6.38315 2x d x* l x+ 2x f x* x- xsin x* x-
  x# 5.74161 2x d x* 2x f x* x+ xsin x* x-
  x# 4.37401 4x d x* lp x- l x- xsin x* x+
  x# 3.99761 2x l x* 2x f x* x+ xsin x* x-
  x# 3.20969 3x d x* l x- xsin x* x-
  x# 2.91454 2x d x* lp x+ l x+ xsin x* x-
  x# 2.73189 4x d x* lp x- 2x l x* x- xsin x* x+
  x# 2.56794 2x lp x* l x- xsin x* x-
  x# 2.5212 2x d x* 2x lp x* x+ l x- xsin x* x-
  x# 2.48889 2x d x* lp x+ 2x l x* x- xsin x* x+
  x# 2.14607 2x d x* lp x- 2x f x* x- xsin x* x+
  x# 1.97773 4x d x* l x+ xsin x* x+
  x# 1.93368 4x l x* xsin x* x+
  x# 1.87076 4x d x* lp x- xsin x* x+
  x# 1.75297 d 2x l x* x- xsin x* x-
  x# 1.43716 2x d x* lp x+ 2x f x* x- xsin x* x-
  x# 1.37257 2x l x* 2x f x* x- xsin x* x-
  x# 1.26182 d lp x+ l x+ xsin x* x+
  x# 1.22412 3x d x* 2x l x* x- xsin x* x-
  x# 1.18683 4x d x* 3x l x* x- xsin x* x+
  x# 1.177 2x d x* lp x- 2x l x* x+ xsin x* x+
  x# 1.16169 2x lp x* l x+ xsin x* x-
  x# 1.07769 d lp x+ l x- xsin x* x+
  x# 1.0595 2x d x* 3x l x* x+ xsin x* x+
  x# .99022 2x d x* l x+ 2x f x* x+ xsin x* x-
  x# .94828 2x d x* 4x l x* x- xsin x* x+
  x# .75168 2x d x* 2x lp x* x- l x+ xsin x* x+
  x# .66938 lp 3x l x* x- xsin x* x-
  x# .63521 4x d x* lp x+ l x- xsin x* x-
  x# .58399 d 2x l x* x+ xsin x* x-
  x# .58331 d 2x f x* x- xsin x* x-
  x# .57156 6x d x* 2x l x* x- xsin x* x+
  x# .56064 2x d x* 2x l x* x- 2x f x* x- xsin x* x-
  x# .55692 d lp x- xsin x* x-
  x# .54592 lp 3x l x* x+ xsin x* x-
  x# .53571 2x d x* 2x l x* x- 2x f x* x+ xsin x* x-
  x# .4784 2x d x* lp x- 3x f x* x- xsin x* x+
  x# .45379 2x d x* 2x l x* x+ 2x f x* x- xsin x* x-
  x# .42622 2x d x* lp x- l x- 2x f x* x+ xsin x* x-
  x# .42033 4x f x* xsin x* x+
  x# .4134 lp 2x f x* x+ xsin x* x+
  x# .40423 3x d x* xsin x* x+
  x# .39451 6x d x* l x- xsin x* x+
  x# .38213 2x d x* lp x- 2x f x* x+ xsin x* x-
  x# .37451 2x d x* lp x- l x+ 2x f x* x- xsin x* x-
  x# .35758 4x d x* lp x+ 2x l x* x- xsin x* x-
  x# .34965 d lp x+ 2x l x* x- xsin x* x+
  x# .33979 2x d x* 3x lp x* x- xsin x* x+
  x# .32866 3x l x* 2x f x* x+ xsin x* x-
  x# .30872 4x d x* 2x lp x* x- l x- xsin x* x+
  x# .30155 lp l x- 2x f x* x- xsin x* x+
  x# .30086 4x d x* l x- 2x f x* x- xsin x* x+
  x# .2942 2x d x* 2x lp x* x- 2x l x* x- xsin x* x+
  x# .29255 6x d x* 3x l x* x- xsin x* x+
  x# .29022 2x d x* lp x+ 2x l x* x+ xsin x* x-
;

\ a(p,2,v) series
: dv-p2 ( dv -- dv' )
  x# .00487 t2 x* lp xsin x* x+
  x# .0015 t2 x* 2x d x* lp x- l x- xpi x+ xsin x* x-
  x# .0012 t2 x* 2x d x* lp x- xpi x+ xsin x* x-
  x# .00108 t2 x* lp l x- xsin x* x+
  x# .0008 t2 x* lp l x+ xsin x* x+
;

\ a(p,0,v) series
: dv-p0 ( dv -- dv' )
  x# 14.24883 18x ve x* 16x t x* x- l x- dtr x# 26.54261 x* x+ xsin x* x+
  x# 7.06304 ll f x- dtr x# .00094 x* x+ xsin x* x+
  x# 1.14307 2x t x* 2x ju x* x- 2x d x* x+ l x- dtr x# 180.11977 x* x+ xsin x* x+
  x# .901140 4x t x* 8x ma x* x- 3x ju x* x+ dtr x# 285.98707 x* x+ xsin x* x+
  x# .82155 ve t x- dtr x# 180.00988 x* x+ xsin x* x+
  x# .78811 18x ve x* 16x t x* x- 2x l x* x- dtr x# 26.54324 x* x+ xsin x* x+
  x# .7393 18x ve x* 16x t x* x- dtr x# 26.5456 x* x+ xsin x* x+
  x# .64371 3x ve x* 3x t x* x- 2x d x* x+ l x- dtr x# 179.98144 x* x+ xsin x* x+
  x# .6388 t ju x- dtr x# 1.2289 x* x+ xsin x* x+
  x# .56341 10x ve x* 3x t x* x- l x- dtr x# 333.30551 x* x+ xsin x* x+
  x# .49331 ll l x+ f x- x# .00127 dtr x* x+ xsin x* x+
  x# .49141 ll l x- f x- x# .00127 dtr x* x+ xsin x* x+
  x# .44532 2x t x* 3x ju x* x- 2x d x* x+ l x- x# 10.07001 dtr x* x+ xsin x* x+
  x# .36061 ll f x+ x# .00071 dtr x* x+ xsin x* x+
  x# .34355 2x ve x* 3x t x* x- x# 269.95393 dtr x* x+ xsin x* x+
  x# .32455 t 2x ma x* x- x# 318.13776 dtr x* x+ xsin x* x+
  x# .30155 2x ve x* 2x t x* x- x# .20448 dtr x* x+ xsin x* x+
  x# .28938 t d x+ f x- x# 95.13523 dtr x* x+ xsin x* x+
  x# .28281 2x t x* 3x ju x* x- 2x d x* x+ 2x l x* x- x# 10.03835 dtr x* x+ xsin x* x+
  x# .24515 2x t x* 2x ju x* x- 2x d x* x+ 2x l x* x- x# .08642 dtr x* x+ xsin x* x+
;

\ a(p,1,v) series
: dv-p1 ( dv -- dv' )
  x# 1.6768 t1 x* lp xsin x* x+
  x# .51642 t1 x* 2x d x* lp x- l x- xpi x+ xsin x* x+
  x# .41383 t1 x* 2x d x* lp x- xpi x+ xsin x* x+
  x# .37115 t1 x* lp l x- xsin x* x+
  x# .2756 t1 x* lp l x+ xsin x* x+
  x# .25425 t1 x* 18x ve x* 16x t x* x- l x- x# 114.5655 dtr x* x+ xsin x* x+
  x# 7.1178e-02 t1 x* 2x d x* lp x+ l x- xsin x* x+
  x# .06128 t1 x* 2x d x* lp x+ xsin x* x+
  x# .04516 t1 x* d lp x+ xpi x+ xsin x* x+
  x# .04048 t1 x* 2x d x* 2x lp x* x- xpi x+ xsin x* x+
  x# .03747 t1 x* 2x lp x* xsin x* x+
  x# .03707 t1 x* 2x d x* 2x lp x* x- l x- xpi x+ xsin x* x+
  x# .03649 t1 x* 2x d x* lp x- l x+ xpi x+ xsin x* x+
  x# .02438 t1 x* lp 2x l x* x- xsin x* x+
  x# .02165 t1 x* 2x d x* lp x- 2x l x* x- xpi x+ xsin x* x+
  x# .01923 t1 x* lp 2x l x* x+ xsin x* x+
;

\ geocentric ecliptic latitude (arc seconds)
\ a(c,0,u) series
: pl-c0 ( -- pl )
  x# 18461.23868 f xsin x*
  x# 1010.16707 l f x+ xsin x* x+
  x# 999.69358 l f x- xsin x* x+
  x# 623.65243 2x d x* f x- xsin x* x+
  x# 199.48374 2x d x* l x- f x+ xsin x* x+
  x# 166.5741 2x d x* l x- f x- xsin x* x+
  x# 117.26069 2x d x* f x+ xsin x* x+
  x# 61.91195 2x l x* f x+ xsin x* x+
  x# 33.3572 2x d x* l x+ f x- xsin x* x+
  x# 31.75967 2x l x* f x- xsin x* x+
  x# 29.57658 2x d x* lp x- f x- xsin x* x+
  x# 15.56626 2x d x* 2x l x* x- f x- xsin x* x+
  x# 15.12155 2x d x* l x+ f x+ xsin x* x+
  x# 12.09414 2x d x* lp x+ f x- xsin x* x-
  x# 8.86814 2x d x* lp x- l x- f x+ xsin x* x+
  x# 7.95855 2x d x* lp x- f x+ xsin x* x+
  x# 7.43455 2x d x* lp x- l x- f x- xsin x* x+
  x# 6.73143 lp l x- f x- xsin x* x-
  x# 6.57957 4x d x* l x- f x- xsin x* x+
  x# 6.46007 lp f x+ xsin x* x-
  x# 6.29648 3x f x* xsin x* x-
  x# 5.63235 lp l x- f x+ xsin x* x-
  x# 5.3684 d f x+ xsin x* x-
  x# 5.31127 lp l x+ f x+ xsin x* x-
  x# 5.07591 lp l x+ f x- xsin x* x-
  x# 4.83961 lp f x- xsin x* x-
  x# 4.80574 d f x- xsin x* x-
  x# 3.98405 3x l x* f x+ xsin x* x+
  x# 3.67446 x+ 4x d x* f x- xsin x+  \ + not *, as in the BASIC
  x# 2.99848 4x d x* l x- f x+ xsin x* x+
  x# 2.79864 l 3x f x* x- xsin x* x+
  x# 2.41388 4x d x* 2x l x* x- f x+ xsin x* x+
  x# 2.18631 2x d x* 3x f x* x- xsin x* x+
  x# 2.14617 2x d x* 2x l x* x+ f x- xsin x* x+
  x# 1.76598 2x d x* lp x- l x+ f x- xsin x* x+
  x# 1.62442 2x d x* 2x l x* x- f x+ xsin x* x-
  x# 1.5813 3x l x* f x- xsin x* x+
  x# 1.51975 2x d x* 2x l x* x+ f x+ xsin x* x+
  x# 1.51563 2x d x* 3x l x* x- f x- xsin x* x-
  x# 1.31782 2x d x* lp x+ l x- f x+ xsin x* x-
  x# 1.26427 2x d x* lp x+ f x+ xsin x* x-
  x# 1.19187 4x d x* f x+ xsin x* x+
  x# 1.13461 2x d x* lp x- l x+ f x+ xsin x* x+
  x# 1.08578 2x d x* 2x lp x* x- f x- xsin x* x+
  x# 1.01938 l 3x f x* x+ xsin x* x-
  x# .822710 2x d x* lp x+ l x+ f x- xsin x* x-
  x# .80422 d lp x+ f x- xsin x* x+
  x# .80259 d lp x+ f x+ xsin x* x+
  x# .79319 lp 2x l x* x- f x- xsin x* x-
  x# .79101 2x d x* lp x+ l x- f x- xsin x* x-
  x# .66741 d l x+ f x+ xsin x* x-
  x# .65022 2x d x* lp x- 2x l x* x- f x- xsin x* x+
  x# .63881 lp 2x l x* x+ f x+ xsin x* x-
  x# .63371 4x d x* 2x l x* x- f x- xsin x* x+
  x# .59577 4x d x* lp x- l x- f x- xsin x* x+
  x# .58893 d l x+ f x- xsin x* x-
  x# .47338 4x d x* l x+ f x- xsin x* x+
  x# .42989 d l x- f x- xsin x* x-
  x# .41494 4x d x* lp x- f x- xsin x* x+
  x# .3835 2x d x* 2x lp x* x- f x+ xsin x* x+
  x# .35183 3x d x* f x- xsin x* x-
  x# .33881 4x d x* lp x- l x- f x+ xsin x* x+
  x# .32906 2x d x* l x- 3x f x* x- xsin x* x+
;

\ a(p,0,u) series
: pl-p0 ( pl -- pl' )
  x# 8.04508 ll x# 180.00071 dtr x* x+ xsin x* x+
  x# 1.51021 t d x+ x# 276.68007 dtr x* x+ xsin x* x+
  x# .63037 18x ve x* 16x t x* x- l x- f x+ x# 26.54287 dtr x* x+ xsin x* x+
  x# .63014 18x ve x* 16x t x* x- l x- f x- x# 26.54272 dtr x* x+ xsin x* x+
  x# .45586 ll l x- x# .00075 dtr x* x+ xsin x* x+
  x# .41571 ll l x+ x# 180.00069 dtr x* x+ xsin x* x+
  x# .32622 ll 2x f x* x- x# .00086 dtr x* x+ xsin x* x+
  x# .29854 ll 2x d x* x- x# .00072 dtr x* x+ xsin x* x+
;

\ a(p,1,u) series
: pl-p1 ( pl -- pl' )
  x# .0743 t1 x* 2x d x* lp x- f x- xpi x+ xsin x* x+
  x# .03043 t1 x* 2x d x* lp x+ f x- xsin x* x+
  x# .02229 t1 x* 2x d x* lp x- l x- f x+ xpi x+ xsin x* x+
  x# .01999 t1 x* 2x d x* lp x- f x+ xpi x+ xsin x* x+
  x# .01869 t1 x* 2x d x* lp x- l x- f x- xpi x+ xsin x* x+
  x# .01696 t1 x* lp l x- f x- xsin x* x+
  x# .01623 t1 x* lp f x+ xsin x* x+
;

\ geocentric position vector of the moon (km) into vec
: moon { D: jdate vec -- }
  jdate obliq-lp { D: dpsi D: deps D: obliq }
  jdate moon-time
  moon-args
  rm-c0 rm-p0 rm-p1 { D: rmm }
  dv-c0 dv-p2 dv-p0 dv-p1 { D: dv }
  ll atr dv x* x+ dpsi x+ modulo { D: plon }
  atr pl-c0 pl-p0 pl-p1 x* { D: plat }

  \ geocentric right ascension and declination
  plon xsin obliq xcos x* plat xtan obliq xsin x* x- { D: a }
  plon xcos { D: b }
  a b atan3 { D: rasc }
  plat xsin obliq xcos x* plat xcos obliq xsin x* plon xsin x* x+ xasin { D: decl }

  rmm rasc xcos x* decl xcos x* vec 1 v!
  rmm rasc xsin x* decl xcos x* vec 2 v!
  rmm decl xsin x* vec 3 v!
;

\ ---------------------------------------------------------------------------
\ 7. Position of the sun and topocentric coordinates
\ ---------------------------------------------------------------------------

\ precision ephemeris of the sun: geocentric position vector (km) into vec
: sun { D: jd vec -- }
  jd x# 2451545.0 x- x# 3652500.0 x/ { D: u }

  \ nutation in longitude
  x# 2.18 u x# -3375.7 u x# 0.36 x* x+ x* x+ { D: a1 }
  x# 3.51 u x# 125666.39 u x# 0.1 x* x+ x* x+ { D: a2 }
  x# 0.0000001 x# -834.0 a1 xsin x* x# 64.0 a2 xsin x* x- x* { D: psi }

  \ nutation in obliquity
  x# 0.0000001 u x*
  x# -226938 u x# -75 u x# 96926 u x# -2491 u x# 12104
  x* x- x* x+ x* x+ x* x+ x* { D: deps }
  x# 0.0000001 x# 4090928.0 x# 446.0 a1 xcos x* x+ x# 28.0 a2 xcos x* x+ x*
  { D: meps }
  meps deps x+ { D: eps }
  eps xsin { D: seps }
  eps xcos { D: ceps }

  0x 0x 0x { D: dl D: dr D: w }
  51 1 do
    i xsa i xsb u x* x+ to w
    dl i xsl w xsin x* x+ to dl
    i xsr x0<> if dr i xsr w xcos x* x+ to dr then
  loop

  dl x# 0.0000001 x* x# 4.9353929 x+ x# 62833.196168 u x* x+ modulo to dl
  x# 149597870.691 dr x# 0.0000001 x* x# 1.0001026 x+ x* to dr
  dl x# 0.0000001
  x# -993.0 x# 17.0 x# 3.1 x# 62830.14 u x* x+ xcos x* x+ x* x+ psi x+ modulo
  { D: rlsun }
  0x { D: rb }

  \ geocentric declination and right ascension
  rlsun xcos { D: crl }
  rlsun xsin { D: srl }
  rb xcos { D: crb }
  rb xsin { D: srb }
  ceps srb x* seps crb x* srl x* x+ xasin { D: decl }
  seps xnegate srb x* ceps crb x* srl x* x+ { D: sra }
  crb crl x* { D: cra }
  sra cra atan3 { D: rasc }

  dr rasc xcos x* decl xcos x* vec 1 v!
  dr rasc xsin x* decl xcos x* vec 2 v!
  dr decl xsin x* vec 3 v!
;

\ ground site position vector (km) into vec
: gsite { D: angle vec -- }
  obslat xsin { D: slat }
  obslat xcos { D: clat }
  angle xsin { D: sangle }
  angle xcos { D: cangle }
  1x 2x flat x* flat flat x* x- slat x* slat x* x- xsqrt { D: b }
  reqm b x/ x# 0.001 obsalt x* x+ { D: c }
  reqm 1x flat x- x* 1x flat x- x* b x/ x# 0.001 obsalt x* x+ { D: d }
  c clat x* cangle x* vec 1 v!
  c clat x* sangle x* vec 2 v!
  d slat x* vec 3 v!
;

\ eci position vector robj to topocentric coordinates in dvec:
\ azimuth, elevation (radians), then the topocentric position vector
: eci2topo { D: gast robj -- }
  gast obslong x+ modulo { D: obslst }
  obslst rsite gsite
  4 1 do robj i v@ rsite i v@ x- rhoijk i v! loop
  rhoijk rhohatijk uvector

  \ eci-to-sez transformation matrix
  obslat xsin { D: sobslat }
  obslat xcos { D: cobslat }
  obslst xsin { D: sobslst }
  obslst xcos { D: cobslst }
  sobslat cobslst x* 1 1 m!
  sobslat sobslst x* 1 2 m!
  cobslat xnegate 1 3 m!
  sobslst xnegate 2 1 m!
  cobslst 2 2 m!
  0x 2 3 m!
  cobslat cobslst x* 3 1 m!
  cobslat sobslst x* 3 2 m!
  sobslat 3 3 m!

  rhohatijk rhohatsez matxvec

  rhohatsez 2 v@ rhohatsez 1 v@ xnegate atan3 dvec 1 v!
  rhohatsez 3 v@ xasin dvec 2 v!
  rhoijk 1 v@ dvec 3 v!
  rhoijk 2 v@ dvec 4 v!
  rhoijk 3 v@ dvec 5 v!
;

\ ---------------------------------------------------------------------------
\ 8. Time scales and root finding
\ ---------------------------------------------------------------------------

\ number of leap seconds for utc julian day
: findleap { D: jday -- D: leapsecond }
  jday 1 jdleap x<= if 1 leapsec exit then
  jday 28 jdleap x>= if 28 leapsec exit then
  1 { n }
  begin n 28 < while
    jday n jdleap x>= jday n 1+ jdleap x< and if n leapsec exit then
    1 +to n
  repeat
  0x
;

\ one term of the TDB correction: coef * Sin(dtr * (rate * t + phase))
: tdb-term { D: coef D: rate D: t D: phase -- D: term }
  coef dtr rate t x* phase x+ x* xsin x*
;

\ UTC julian date to TDB julian date
\ Reference Frames in Astronomy and Geophysics, Kovalevsky et al., 1989
: utc2tdb { D: jdutc -- D: jdtdb }
  jdutc findleap x# 32.184 x+ x# 86400.0 x/ { D: corr }
  jdutc corr x+ { D: jdtt }
  jdtt x# 2451545.0 x- x# 36525.0 x/ { D: t }

  \ correction in microseconds
  x# 1656.675 x# 35999.3729 t x# 357.5287 tdb-term to corr
  corr x# 22.418 x# 32964.467 t x# 246.199 tdb-term x+ to corr
  corr x# 13.84  x# 71998.746 t x# 355.057 tdb-term x+ to corr
  corr x# 4.77   x# 3034.906  t x# 25.463  tdb-term x+ to corr
  corr x# 4.677  x# 34777.259 t x# 230.394 tdb-term x+ to corr
  corr x# 10.216 t x* x# 35999.373 t x# 243.451 tdb-term x+ to corr
  corr x# 0.171  t x* x# 71998.746 t x# 240.98  tdb-term x+ to corr
  corr x# 0.027  t x* x# 1222.114  t x# 194.661 tdb-term x+ to corr
  corr x# 0.027  t x* x# 3034.906  t x# 336.061 tdb-term x+ to corr
  corr x# 0.026  t x* x# -20.186   t x# 9.382   tdb-term x+ to corr
  corr x# 0.007  t x* x# 29929.562 t x# 264.911 tdb-term x+ to corr
  corr x# 0.006  t x* x# 150.678   t x# 59.775  tdb-term x+ to corr
  corr x# 0.005  t x* x# 9037.513  t x# 256.025 tdb-term x+ to corr
  corr x# 0.043  t x* x# 35999.373 t x# 151.121 tdb-term x+ to corr

  \ corrections to days
  x# 0.000001 corr x* x# 86400.0 x/ to corr
  jdtt corr x+
;

\ real root of a single non-linear function: Brent's method.  One word serves
\ both brent and jbrent of the BASIC; func is ( x -- fx ).
: brent { D: x1 D: x2 D: tol func -- D: xroot D: froot }
  x# 2.23e-16 { D: eps }
  0x 0x 0x { D: c D: d D: e }
  0x 0x 0x 0x { D: p D: q D: r D: s }
  0x 0x 0x 0x { D: tol1 D: xm D: xmin D: tmp }
  x1 x2 { D: a D: b }
  a func execute { D: fa }
  b func execute { D: fb }
  fb { D: fcc }
  0 { iter }
  begin iter 50 < while
    1 +to iter

    fb fcc x* x0> if
      a to c
      fa to fcc
      b a x- to d
      d to e
    then

    fcc xabs fb xabs x< if
      b to a
      c to b
      a to c
      fb to fa
      fcc to fb
      fa to fcc
    then

    2x eps x* b xabs x* halfx tol x* x+ to tol1
    halfx c b x- x* to xm

    xm xabs tol1 x<= fb x0= or if b fb exit then

    e xabs tol1 x>= fa xabs fb xabs x> and if
      fb fa x/ to s
      a c x= if
        2x xm x* s x* to p
        1x s x- to q
      else
        fa fcc x/ to q
        fb fcc x/ to r
        s 2x xm x* q x* q r x- x* b a x- r 1x x- x* x- x* to p
        q 1x x- r 1x x- x* s 1x x- x* to q
      then
      p x0> if q xnegate to q then
      p xabs to p
      e q x* xabs to xmin
      3x xm x* q x* tol1 q x* xabs x- to tmp
      xmin tmp x< if tmp to xmin then
      2x p x* xmin x< if
        d to e
        p q x/ to d
      else
        xm to d
        d to e
      then
    else
      xm to d
      d to e
    then

    b to a
    fb to fa

    d xabs tol1 x> if
      b d x+ to b
    else
      b xm xsgn tol1 x* x+ to b
    then

    b func execute to fb
  repeat
  b fb
;

\ objective function for tdb2utc
: jdfunc ( jdutc -- fx ) utc2tdb jdsaved x- ;

\ TDB julian day to UTC julian day
: tdb2utc { D: jdtdb -- D: jdutc }
  jdtdb to jdsaved
  jdsaved x# 0.1 x- jdsaved x# 0.1 x+ x# 1.0e-8 ['] jdfunc brent 2drop
;

\ ---------------------------------------------------------------------------
\ 9. Eclipse objective function, minimum and root bracketing
\ ---------------------------------------------------------------------------

\ solar eclipse objective function; x = elapsed simulation time (days)
: sefunc { D: x -- D: fx }
  jdtdbi x x+ { D: jdtdb }
  jdtdb rmoon moon
  jdtdb rsun sun
  4 1 do rsun i v@ rmoon i v@ x- rm2s i v! loop

  \ topocentric coordinates of the moon
  jdtdb tdb2utc gast2 { D: gast }
  gast rmoon eci2topo
  dvec 3 v@ rtmoon 1 v!
  dvec 4 v@ rtmoon 2 v!
  dvec 5 v@ rtmoon 3 v!
  rtmoon vecmag { D: rtmmoon }

  \ current topocentric elevation of the moon (radians)
  dvec 2 v@ to elev-minima

  \ shadow axis unit position vector
  rm2s vecmag { D: rm2smag }
  4 1 do rm2s i v@ xnegate rm2smag x/ uaxis i v! loop

  \ moon-to-observer unit position vector
  4 1 do rtmoon i v@ xnegate rtmmoon x/ um2o i v! loop

  \ penumbra shadow angle
  radmoon rtmmoon x/ xasin radsun radmoon x+ rm2smag x/ xasin x+ { D: pangle }

  \ separation angle between anti-sun and moon-to-observer vectors
  uaxis um2o vdot xacos pangle x-
;

\ one-dimensional minimization: Brent's method
: minima { D: a D: b D: tolm -- D: xmin D: fmin }
  x# 2.23e-16 { D: epsm }
  x# 0.38196601125 { D: c }
  0x 0x 0x 0x { D: d D: e D: t2 D: p }
  0x 0x 0x 0x { D: q D: r D: u D: fu }
  0x 0x { D: xm D: tol1 }
  a c b a x- x* x+ { D: x }
  x x { D: w D: v }
  x sefunc { D: fx }
  fx fx { D: fw D: fv }
  0 { iter }
  begin
    1 +to iter
    iter 50 > if
      ." error in function minima!" cr
      ." (more than 50 iterations)" cr
      abort
    then

    halfx a b x+ x* to xm
    tolm x xabs x* epsm x+ to tol1
    2x tol1 x* to t2

    x xm x- xabs t2 halfx b a x- x* x- x<= if x fx exit then

    e xabs tol1 x> if
      x w x- fx fv x- x* to r
      x v x- fx fw x- x* to q
      x v x- q x* x w x- r x* x- to p
      2x q r x- x* to q
      q x0> if p xnegate to p then
      q xabs to q
      e to r
      d to e
    then

    p xabs halfx q x* r x* xabs x>=
    p q a x x- x* x<= or
    p q b x x- x* x>= or if
      x xm x>= if a x x- to e else b x x- to e then
      c e x* to d
    else
      p q x/ to d
      x d x+ to u
      u a x- t2 x< b u x- t2 x< or if
        xm x x- xsgn tol1 x* to d
      then
    then

    d xabs tol1 x>= if
      x d x+ to u
    else
      x d xsgn tol1 x* x+ to u
    then

    u sefunc to fu

    fu fx x<= if
      u x x>= if x to a else x to b then
      w to v
      fw to fv
      x to w
      fx to fw
      u to x
      fu to fx
    else
      u x x< if u to a else u to b then
      fu fw x<= w x x= or if
        w to v
        fw to fv
        u to w
        fu to fw
      else
        fu fv x<= v x x= or v w x= or if
          u to v
          fu to fv
        then
      then
    then
  again
;

\ bracket a single root of the objective function
: broot { D: x1in D: x2in D: factor D: dxmax -- D: x1out D: x2out }
  x1in sefunc { D: f1 }
  x1in { D: x3 }
  x2in x1in x- { D: dx }
  0x { D: f2 }
  begin
    \ geometrically accelerate the second point
    x2in factor x2in x3 x- x* x+ to x2in
    x2in sefunc to f2

    \ check to see if rectification is required
    x2in x3 x- xabs dxmax x> if x2in dx x- to x3 then

    f1 f2 x* x0<
  until
  x1in x2in
;

\ ---------------------------------------------------------------------------
\ 10. Calendar and printing
\ ---------------------------------------------------------------------------

\ the part of the julian day formula for gregorian dates
: gregorian-b ( y -- b )
  x# 100.0 x/ xtrunc { D: a }
  2x a x- a 4x x/ xtrunc x+
;

\ gregorian date to julian day
: julian { D: month D: day D: year -- D: jday }
  year month 0x 0x { D: y D: m D: b D: c }
  m 2x x<= if
    y 1x x- to y
    m 12x x+ to m
  then
  y x0< if x# -0.75 to c then
  year x# 1582.0 x>= if
    year x# 1582.0 x> if
      y gregorian-b to b
    else
      month 10x x>= if
        month 10x x> if
          y gregorian-b to b
        else
          day 4x x> if
            day x# 14.0 x> if
              y gregorian-b to b
            else
              ." this date does not exist!!" cr abort
            then
          then
        then
      then
    then
  then
  x# 365.25 y x* c x+ xtrunc x# 30.6001 m 1x x+ x* xtrunc x+
  day x+ b x+ x# 1720994.5 x+
;

\ julian day to gregorian date
: gdate { D: jday -- D: month D: day D: year }
  jday halfx x+ xtrunc { D: z }
  jday halfx x+ z x- { D: f }
  z { D: a }
  0x { D: alpha }
  z x# 2299161 x>= if
    z x# 1867216.25 x- x# 36524.25 x/ xtrunc to alpha
    z 1x x+ alpha x+ alpha 4x x/ xtrunc x- to a
  then
  a x# 1524.0 x+ { D: b }
  b x# 122.1 x- x# 365.25 x/ xtrunc { D: c }
  x# 365.25 c x* xtrunc { D: d }
  b d x- x# 30.6001 x/ xtrunc { D: e }
  b d x- x# 30.6001 e x* xtrunc x- f x+ { D: day }
  e x# 13.5 x< if e 1x x- else e 13 n>x x- then { D: month }
  month x# 2.5 x> if c x# 4716.0 x- else c x# 4715.0 x- then { D: year }
  month day year
;

: tab ( -- ) 9 emit ;
: blank ( -- ) space cr ;

\ Str$() of a whole number
: x.int ( x -- ) x>n (.) ;

\ Str$(f, 0, n), by FloatToStr's method so that it rounds the same way
: x.fixed { D: f n -- }
  halfx n 10^ x/ { D: rounding }
  f x0> if f rounding x+ to f then
  f x0< if
    f rounding x- to f
    [char] - emit
    f xnegate to f
  then
  f xtrunc x.int
  n 0> if
    [char] . emit
    f f xfloor x- to f
    n 0 ?do
      f 10x x* to f
      f xfloor 2dup x>n [char] 0 + emit
      f 2swap x- to f
    loop
  then
;

: month. ( n -- )
  case
    1 of ." January" endof
    2 of ." February" endof
    3 of ." March" endof
    4 of ." April" endof
    5 of ." May" endof
    6 of ." June" endof
    7 of ." July" endof
    8 of ." August" endof
    9 of ." September" endof
    10 of ." October" endof
    11 of ." November" endof
    12 of ." December" endof
  endcase
;

\ print julian day as calendar date and UTC time
: jd2str { D: jdutc -- }
  jdutc gdate { D: month D: day D: year }
  ." calendar date  " tab month x>n month. tab space tab
  day xfloor x.int tab space tab year x.int cr
  blank
  24x day day xfloor x- x* { D: thr0 }
  thr0 xfloor { D: thr }
  60x thr0 thr x- x* { D: tmin0 }
  tmin0 xfloor { D: tmin }
  60x tmin0 tmin x- x* { D: tsec }
  ." UTC time       " tab thr x.int ."  hours " tmin x.int ."  minutes "
  tsec 2 x.fixed ."  seconds" cr
;

\ print decimal degrees as degrees, minutes, seconds
: .dms { D: dd -- }
  dd xabs { D: d1 }
  d1 xtrunc { D: dg }
  d1 dg x- 60x x* to d1
  d1 xtrunc { D: m }
  d1 m x- 60x x* { D: s }
  dd x0< if
    dg x0<> if
      dg xnegate to dg
    else
      m x0<> if m xnegate to m else s xnegate to s then
    then
  then
  dg x.int ."  deg " m x.int ."  min " s 2 x.fixed ."  sec"
;

\ print solar eclipse conditions
: eclprint { iflag D: jdtdb -- }
  iflag 1 = if
    blank blank ." begin penumbral phase of solar eclipse" cr blank
    jdtdb to jdprint
  then
  iflag 2 = if
    blank blank ." greatest eclipse conditions" cr blank
  then
  iflag 3 = if
    blank blank ." end penumbral phase of solar eclipse" cr blank
  then

  jdtdb tdb2utc { D: jdutc }
  jdutc jd2str
  blank
  ." UTC julian day     " tab jdutc 8 x.fixed cr

  \ topocentric coordinates of the moon
  jdutc gast2 { D: gast }
  jdtdb rmoon moon
  gast rmoon eci2topo
  blank
  ." topocentric coordinates" cr
  blank
  ." lunar azimuth angle    " tab rtd dvec 1 v@ x* .dms cr
  blank
  ." lunar elevation angle  " tab rtd dvec 2 v@ x* .dms cr

  \ topocentric coordinates of the sun
  jdtdb rsun sun
  gast rsun eci2topo
  blank
  ." solar azimuth angle    " tab rtd dvec 1 v@ x* .dms cr
  blank
  ." solar elevation angle  " tab rtd dvec 2 v@ x* .dms cr

  iflag 3 = if
    blank
    ." event duration   " tab 24x jdtdb jdprint x- x* 8 x.fixed ."  hours" cr
    blank
  then
;

\ ---------------------------------------------------------------------------
\ 11. Eclipse search
\ ---------------------------------------------------------------------------

0x 2value ti
0x 2value tf
0x 2value dt
0x 2value dtsml
0x 2value tcur
0x 2value df
0x 2value dflft
0x 2value fmin1
0x 2value ftemp
0x 2value tmin1
0x 2value ft
0x 2value el
0x 2value er
0 value iend

\ compute and display eclipse events around the extrema time topt
: events1 { D: topt -- }
  x# 0.25 { D: factor }                  \ geometric acceleration factor
  x# 600.0 x# 86400.0 x/ { D: dxmax }    \ rectification interval
  x# 1.0e-8 { D: rtol }                  \ root-finding convergence tolerance
  x# 30.0 x# 86400.0 x/ { D: step }

  \ event start conditions
  topt topt step x- factor dxmax broot rtol ['] sefunc brent
  { D: troot D: froot }
  troot ti x< if
    ti to troot
    ti sefunc to froot
  then
  1 jdtdbi troot x+ eclprint

  \ greatest eclipse conditions
  2 jdtdbi topt x+ eclprint

  \ event end conditions
  topt topt step x+ factor dxmax broot rtol ['] sefunc brent
  to froot to troot
  troot tf x> if
    tf to troot
    tf sefunc to froot
  then
  3 jdtdbi troot x+ eclprint

  troot to trr
;

\ what the BASIC does on reaching the final time, in both search loops.
\ Result: 0 = carry on, 1 = search finished, 2 = dflft was positive
: at-final-time ( -- code )
  iend 1 = if 1 exit then
  tf sefunc to fmin1
  tf dtsml x- sefunc to ftemp
  fmin1 ftemp x- to df
  tf to tmin1
  df x0< if
    fmin1 x0< if tmin1 events1 then
    1 exit
  then
  dflft x0> if 2 exit then
  tf to er
  1 to iend
  0
;

\ slope of the objective function at tcur
: slope ( -- )
  tcur sefunc to ft
  tcur dtsml x- sefunc to ftemp
  ft ftemp x- to df
;

\ find where the function first starts decreasing; true = search finished
: find-decreasing ( -- finished? )
  begin df x0> while
    tcur dt x+ to tcur
    tcur tf x>= if
      at-final-time dup 1 = if drop true exit then
      2 = if false exit then
    then
    slope
  repeat
  false
;

\ find where the function first starts increasing; true = search finished
: find-increasing ( -- finished? )
  begin
    tcur to el
    df to dflft
    tcur dt x+ to tcur
    tcur tf x>= if
      at-final-time if true exit then
    then
    slope
    df x0>
  until
  false
;

\ predict solar eclipse events.  The BASIC's three "For ... To 100000" loops
\ are plain loops here: their limits are never reached in a real search.
: ecl-event { D: start D: finish D: step D: small -- }
  start to ti
  finish to tf
  step to dt
  small to dtsml
  0 to iend

  \ check the initial time for a minimum
  ti sefunc to fmin1
  ti to tmin1
  ti dtsml x+ sefunc to ftemp
  ftemp fmin1 x- to df
  df to dflft
  ti to el
  el to er
  ti to tcur

  \ if the slope is positive and the minimum is negative,
  \ calculate event conditions at the initial time
  df x0> fmin1 x0< and if tmin1 events1 then

  begin
    find-decreasing if exit then
    find-increasing if exit then
    tcur to er

    \ calculate minimum using Brent's method
    el er x# 1.0e-6 minima to fmin1 to tmin1
    er to el
    df to dflft

    \ if the minimum is negative and the topocentric elevation angle of
    \ the moon is positive, calculate event conditions for this minimum
    fmin1 x0< elev-minima x0> and if
      tmin1 events1
      trr to tcur
    then
  again
;

\ ---------------------------------------------------------------------------
\ 12. Input and the main program
\ ---------------------------------------------------------------------------

80 constant line-size
line-size buffer: line-buf
0 value line-len
0 value line-pos

\ Input: a line of comma separated fields
: read-line ( -- )
  ." ? " line-buf line-size accept to line-len
  0 to line-pos
  cr
;

: next-field ( -- c-addr u )
  line-pos { p0 }
  begin
    line-pos line-len < if
      line-buf line-pos + c@ [char] , <>
    else
      false
    then
  while
    1 +to line-pos
  repeat
  line-buf p0 + line-pos p0 -
  1 +to line-pos
;

: next-number ( -- x ) next-field str>x ;

0x 2value cmonth
0x 2value cday
0x 2value cyear

\ request the initial calendar date
: getdate ( -- )
  begin
    blank
    ." please input the initial calendar date" cr
    blank
    ." (month [1 - 12], day [1 - 31], year [yyyy])" cr
    ." < for example, october 21, 1986 is input as 10,21,1986 >" cr
    ." < b.c. dates are negative, a.d. dates are positive >" cr
    ." < the day of the month may also include a decimal part >" cr
    blank
    read-line
    next-number to cmonth
    next-number to cday
    next-number to cyear
    cmonth 1x x>= cmonth 12x x<= and cday 1x x>= and cday 31x x<= and
  until
;

0x 2value in-deg
0x 2value in-min
0x 2value in-sec
false value in-minus-zero

\ Left$(deg$, 2) = "-0"
: minus-zero? ( c-addr u -- flag )
  2 >= if
    dup c@ [char] - = swap 1+ c@ [char] 0 = and
  else
    drop false
  then
;

: read-dms ( -- )
  read-line
  next-field 2dup minus-zero? to in-minus-zero str>x to in-deg
  next-number to in-min
  next-number to in-sec
;

\ the degree, minute and second fields are in range
: dms-ok? { D: limit -- flag }
  in-deg xabs limit x<=
  in-min x0< 0= and in-min 60x x<= and
  in-sec x0< 0= and in-sec 60x x<= and
;

\ degrees, minutes and seconds to radians.  minus-zero? marks a negative angle
\ under one degree, which the degree field alone cannot show.
: dms>rad { D: deg D: mins D: secs minus-zero? -- D: rad }
  minus-zero? if
    dtr xnegate mins 60x x/ secs 3600x x/ x+ x* exit
  then
  deg x0= if
    dtr mins 60x x/ secs 3600x x/ x+ x* exit
  then
  dtr deg xsgn deg xabs mins 60x x/ x+ secs 3600x x/ x+ x* x*
;

\ request observer latitude, longitude and altitude
: observer ( -- )
  begin
    ." please input the geographic latitude of the observer" cr
    ." (degrees [-90 to +90], minutes [0 - 60], seconds [0 - 60])" cr
    ." (north latitudes are positive, south latitudes are negative)" cr
    read-dms
    x# 90.0 dms-ok?
  until
  in-deg in-min in-sec in-minus-zero dms>rad to obslat

  begin
    cr
    ." please input the geographic longitude of the observer" cr
    ." (degrees [0 - 360], minutes [0 - 60], seconds [0 - 60])" cr
    ." (east longitude is positive, west longitude is negative)" cr
    read-dms
    x# 360.0 dms-ok?
  until
  in-deg in-min in-sec in-minus-zero dms>rad to obslong

  blank
  ." please input the altitude of the observer (meters)" cr
  ." (positive above sea level, negative below sea level)" cr
  read-line
  next-number to obsalt
;

\ print n, four digits with leading zeros
: .4digits ( n -- )
  dup 1000 / [char] 0 + emit 1000 mod
  dup 100 / [char] 0 + emit 100 mod
  dup 10 / [char] 0 + emit 10 mod
  [char] 0 + emit
;

\ search ndays from jdtdbi and report the time taken
: eclipse-search { D: ndays -- }
  systick::systick-counter { start }
  blank
  ." searching for solar eclipse ..." cr
  blank
  0x ndays x# 0.25 x# 0.1 ecl-event
  systick::systick-counter start - { ticks }
  ." Time taken : " tab space
  ticks 10000 / (.) [char] . emit ticks 10000 mod .4digits
  tab ."  Seconds" cr
;

\ the BASIC program: prompt for everything, then search
: solar-eclipse ( -- )
  blank
  ." program solar_eclipse" cr
  ." =====================" cr
  getdate
  blank
  observer
  cmonth cday cyear julian utc2tdb to jdtdbi
  blank
  ." please input the search duration in days" cr
  read-line
  next-number eclipse-search
;

\ the benchmark inputs, without the prompts
: reference-run ( -- )
  x# 39 x# 40 x# 36 false dms>rad to obslat
  x# -104 x# 57 x# 12 false dms>rad to obslong
  x# 1644 to obsalt
  x# 12 x# 1 x# 2000 julian utc2tdb to jdtdbi
  x# 30 eclipse-search
;

end-module> import
