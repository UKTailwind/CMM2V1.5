\ black_scholes.fs: the Black-Scholes values of a European call and put, in
\ double precision.
\
\ include black_scholes.fs, then option-table. It prints two worked examples and
\ a table of options, checks them, and says how many checks failed.
\
\ Rates and volatilities are fractions, so 0.1de0 is 10%, and times are in years.

float64 import

\ 1 - N(x) for x >= 0, N being the cumulative normal distribution. This is
\ Abramowitz and Stegun 26.2.17, which is good to 7.5e-8
: normal-tail { D: x -- D: tail }
  dv1 0.2316419de0 x dv* dv1 dv+ dv/ { D: t }
  1.330274429de0 t dv* -1.821255978de0 dv+
  t dv* 1.781477937de0 dv+
  t dv* -0.356563782de0 dv+
  t dv* 0.319381530de0 dv+
  t dv*
  x x dv* dv-half dv* dvexp dv*
  dv2 dvpi dv* dvsqrt dv/
;

\ N(x)
: cum-normal { D: x -- D: n }
  x dv0< if x dvnegate normal-tail else dv1 x normal-tail dv- then
;

\ The values of a call and a put, from the stock price, the strike price, the
\ interest rate, the volatility and the time to expiry
: black-scholes { D: s D: k D: r D: vol D: t -- D: call D: put }
  vol t dvsqrt dv* { D: sd }
  s k dv/ dvln  vol vol dv* dv2 dv/ r dv+ t dv*  dv+  sd dv/ { D: d1 }
  d1 sd dv- { D: d2 }
  r t dv* dvnegate dvexp k dv* { D: pv }
  d1 cum-normal s dv*  d2 cum-normal pv dv*  dv-
  d2 dvnegate cum-normal pv dv*  d1 dvnegate cum-normal s dv*  dv-
;

\ What follows is the table and its checks

create months 1 , 3 , 6 , 12 ,
variable failures

: month ( index -- n ) cells months + @ ;
: years ( n -- f ) n>dv 12de0 dv/ ;
: check ( flag -- ) 0= if 1 failures +! then ;
: near? ( f1 f2 -- flag ) dv- dvabs 0.00005de0 dv< ;

\ An integer, and a price to four places, each at the right of width columns
: .right { n width -- } n 0 <# #s #> width over - spaces type ;
: .price { D: f width -- }
  f 10000de0 dv* dvround-half-even dv>n
  0 <# # # # # [char] . hold #s #> width over - spaces type
;

\ The call and put on a stock at 100, with interest at 10% and volatility at 20%
: option-values { strike n -- D: call D: put }
  100de0 strike n>dv 0.1de0 0.2de0 n years black-scholes
;

\ Does call - put = stock - strike * exp(-rt)? It must, whatever N is
: parity? { strike n -- flag }
  strike n option-values dv-
  100de0 0.1de0 n years dv* dvnegate dvexp strike n>dv dv* dv-
  dv- dvabs 1de-12 dv<
;

: .heading ( -- )
  ."          Calls, by months to expiry     Puts, by months to expiry" cr
  ." Strike"
  2 0 do 4 0 do i month 8 .right loop 2 spaces loop cr
;

: .row { strike -- }
  strike 6 .right
  4 0 do strike i month option-values 2drop 8 .price loop
  2 spaces
  4 0 do strike i month option-values 2swap 2drop 8 .price loop
  cr
  4 0 do strike i month parity? check loop
;

\ A call and a put, printed and checked against what they should be
: .option { D: call D: put D: want-call D: want-put -- }
  ." Call " call 8 .price ."     Put " put 8 .price cr
  call want-call near? check
  put want-put near? check
;

: examples ( -- )
  ." Stock 42, strike 40, interest 10%, volatility 20%, 6 months" cr
  42de0 40de0 0.1de0 0.2de0 0.5de0 black-scholes 4.7594de0 0.8086de0 .option cr
  ." Stock 31.55, strike 22.75, interest 5%, volatility 50%, 3.5 years" cr
  31.55de0 22.75de0 0.05de0 0.5de0 3.5de0 black-scholes
  16.5039de0 4.0516de0 .option
;

: option-table ( -- )
  0 failures !
  cr ." Black-Scholes in double precision" cr cr
  examples cr
  ." Stock 100, interest 10%, volatility 20%" cr cr
  .heading
  5 0 do i 10 * 80 + .row loop
  0de0 cum-normal 0.5de0 near? check
  1.96de0 cum-normal 0.9750de0 near? check
  cr failures @ . ." checks failed" cr
;
