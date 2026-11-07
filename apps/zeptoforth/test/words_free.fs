\ after a reboot or Ctrl-C: all of it free again. Each of these fails if the reset did not happen: hold-all took every handle and block
create hs 8 cells allot
create ds 4 cells allot
create bs 64 cells allot
create tx 8 allot
: now ( -- us ) micros drop ;
: t-free
  8 0 do s" A:/zeptoforth/zt/a.bin" o-read file-open hs i cells + ! loop
  8 0 do hs i cells + @ file-close loop
  ." eight files opened again" cr true s" files were given back" check
  4 0 do s" A:/" dir-open ds i cells + ! loop
  4 0 do ds i cells + @ dir-close loop
  true s" directories were given back" check
  64 0 do 64 alloc bs i cells + ! loop
  64 0 do bs i cells + @ release loop
  true s" blocks were given back" check
  sound-state $FF and 0 s" the stream stopped" eq
  [: 16 font-size ;] ['] x-no-font s" the font is gone" raises
  mm.fontwidth 8 s" the console is back on font 1: width" eq
  mm.fontheight 12 s" the console is back on font 1: height" eq
  s" MM.INFO(PIN 33)" basic-eval s" Unused" equal-strings? s" GP21 unconfigured" check
  0 0 { edges last }
  3 pin@ to last
  now
  begin
    3 pin@ dup last > if edges 1+ to edges then to last
    dup now swap - 100000 >
  until drop
  edges 0 s" PWM stopped" eq
  [: 1 com-avail ;] ['] x-not-possible s" COM1 closed" raises
  [: 0 $50 0 0 0 0 true i2c-xfer ;] ['] x-not-possible s" I2C closed" raises
  [: 0 tx tx 1 spi-xfer ;] ['] x-not-possible s" SPI closed" raises
  black cls
  ." X"
  0 { lit } 16 0 do 16 0 do i j pixel@ 0<> if lit 1+ to lit then loop loop
  lit 0> s" the console is on the screen again" check
;
t-free
ends
.s
