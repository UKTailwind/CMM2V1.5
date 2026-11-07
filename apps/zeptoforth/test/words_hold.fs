\ everything Forth can hold of the firmware, taken and not given back: a reboot or Ctrl-C must return it all
cmm2 import
create glyph 8 c, 8 c, 65 c, 1 c, $FF c, $FF c, $FF c, $FF c, $FF c, $FF c, $FF c, $FF c,
: hold-all
  8 0 do s" A:/zeptoforth/zt/a.bin" o-read file-open drop loop
  4 0 do s" A:/" dir-open drop loop
  64 0 do 64 alloc drop loop
  audio-start drop
  16 glyph font-define
  16 1 font
  21 pin-out pull-none setpin
  3 1000 32768 pwm
  1 9600 8 0 1 0 com-open
  0 100 100 i2c-open
  0 1000000 0 spi-open
  0 console-screen
;
hold-all
: s-9th s" A:/zeptoforth/zt/a.bin" o-read file-open ;
' s-9th try .
: s-5th s" A:/" dir-open ;
' s-5th try .
: s-65th 64 alloc ;
' s-65th try . cr
s" MM.INFO(PIN 33)" basic-eval type cr
." sound " sound-state . cr
.s
