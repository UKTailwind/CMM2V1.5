\ input and I/O. Never GP10 (pin 40), so never SPI bus 1. Wires are looked for first, not assumed
0 console-screen
: now ( -- us ) micros drop ;
create raw 8 allot
: t-input
  0 keydown 0 s" keydown 0: no keys held" eq
  1 keydown 0 s" keydown 1: 0" eq
  7 keydown 0 s" keydown 7: no modifiers" eq
  9 keydown 0 s" keydown 9 is nothing" eq
  raw 8 $FF fill
  raw keys-raw 0 s" keys-raw counts none" eq
  raw 2 + c@ 0 s" keys-raw wrote its report" eq
  mouse { x y b w f }
  ." mouse " x . y . b . w . f . cr
  f if
    123 87 mouse-set
    mouse drop 2drop 87 s" mouse-set y" eq 123 s" mouse-set x" eq
    -5 -5 mouse-set
    mouse drop 2drop 0 s" mouse-set clamps y" eq 0 s" mouse-set clamps x" eq
  else
    ." no mouse: mouse-set not tested" cr
  then
  0 gamepad { lx ly bt }
  bt -1 s" gamepad 0 is none" eq
  99 gamepad 2drop . cr
  1 gamepad { lx1 ly1 bt1 }
  ." gamepad 1 " lx1 . ly1 . bt1 . cr
;
t-input

: wired? ( out in -- flag ) { o i }
  o pin-out pull-none setpin
  i pin-in pull-down setpin
  0 o pin! 2 ms i pin@ 0=
  1 o pin! 2 ms i pin@ 0<> and
  0 o pin! 2 ms i pin@ 0= and
  o pin-off pull-none setpin
  i pin-off pull-none setpin
;
: .wire ( out in -- flag ) 2dup ." GP" swap . ." to GP" . wired? dup if ." wired" else ." NOT wired" then cr ;
0 8 .wire constant w0
1 9 .wire constant w1
3 11 .wire constant w3
4 12 .wire constant w4
5 13 .wire constant w5
6 14 .wire constant w6
7 15 .wire constant w7

: t-pin-errors
  [: 99 pin-out pull-none setpin ;] ['] x-no-such-io s" setpin 99 raises x-no-such-io" raises
  [: 6 9 pull-none setpin ;] ['] x-not-possible s" setpin mode 9 raises x-not-possible" raises
  [: 13 pin-analog pull-none setpin ;] ['] x-not-possible s" GP13 analog raises x-not-possible" raises
  [: 99 pin@ ;] ['] x-no-such-io s" pin@ 99 raises x-no-such-io" raises
  [: 21 adc 2drop ;] ['] x-not-possible s" adc on a pin not set up raises x-not-possible" raises
  [: 99 adc ;] ['] x-no-such-io s" adc 99 raises x-no-such-io" raises
  [: 7 1000 100 pwm ;] ['] x-no-such-io s" pwm on GP7 raises x-no-such-io" raises
  [: 3 0 100 pwm ;] ['] x-not-possible s" pwm at 0 Hz raises x-not-possible" raises
  [: 7 pwm-stop ;] ['] x-no-such-io s" pwm-stop GP7 raises x-no-such-io" raises
;
t-pin-errors

: t-pin-self
  \ without a wire, an input's pull says the pin works; an output reads its own level
  21 pin-in pull-up setpin 2 ms
  21 pin@ 1 s" GP21 in, pull up reads 1" eq
  21 pin-in pull-down setpin 2 ms
  21 pin@ 0 s" GP21 in, pull down reads 0" eq
  21 pin-out pull-none setpin
  1 21 pin! 21 pin@ 1 s" GP21 out 1 reads 1" eq
  0 21 pin! 21 pin@ 0 s" GP21 out 0 reads 0" eq
  21 pin-od pull-none setpin
  0 21 pin! 21 pin@ 0 s" GP21 open drain low reads 0" eq
  21 pin-off pull-none setpin
;
t-pin-self

: t-adc
  26 pin-analog pull-none setpin
  26 adc { v uv }
  ." GP26 reads " v . uv . cr
  v 0>= v 65536 < and s" adc in range" check
  v 3300000 65535 */ uv - abs 2000 < s" adc's microvolts are its value of 3.3 V" check
  26 pin-off pull-none setpin
;
t-adc

: pwm-count ( gp -- edges high samples ) { gp }
  0 0 0 { edges high n }
  gp pin@ { last }
  now
  begin
    gp pin@ dup last > if edges 1+ to edges then dup to last
    high + to high n 1+ to n
    dup now swap - 100000 >
  until
  drop edges high n
;
\ with no wires a PWM pin is watched by itself: pin@ reads the port's input register whatever drives the pin
: pwm-is { gp hz duty a u -- }
  gp pwm-count { e h n }
  ." GP" gp . ." edges in 100 ms " e . ." high " h 100 n */ . ." %" cr
  e hz 9 10 */ > e hz 11 10 */ < and h 100 n */ duty 5 - > and h 100 n */ duty 5 + < and a u check
;
: t-pwm
  3 1000 32768 pwm
  5 ms
  3 100 50 s" PWM GP3 1 kHz 50%, watched on the pin" pwm-is
  3 2000 16384 pwm
  5 ms
  3 200 25 s" PWM GP3 2 kHz 25%" pwm-is
  2 2000 49152 pwm
  5 ms
  2 200 75 s" PWM GP2 75% beside it" pwm-is
  3 200 25 s" GP3 unchanged by GP2" pwm-is
  2 pwm-stop
  5 ms
  2 pwm-count drop drop 0 s" pwm-stop GP2 stops it" eq
  3 200 25 s" GP3 goes on after GP2 stops" pwm-is
  3 pwm-stop
  5 ms
  3 pwm-count drop drop 0 s" pwm-stop GP3 stops it" eq
;
t-pwm

create tx 16 allot
create rx 16 allot
: t-i2c
  [: 0 $50 0 0 0 0 true i2c-xfer ;] ['] x-not-possible s" i2c-xfer on a closed bus raises x-not-possible" raises
  [: 2 100 100 i2c-open ;] ['] x-no-such-io s" i2c-open bus 2 raises x-no-such-io" raises
  [: 0 123 100 i2c-open ;] ['] x-firmware s" i2c-open at 123 kHz raises x-firmware" raises
  ." its message: " io-error type cr
  0 100 100 i2c-open
  ." I2C addresses that answer on bus 0:"
  128 0 do 0 i 0 0 0 0 true i2c-xfer 0= if i . then loop cr
  0 $50 0 0 0 0 true i2c-xfer ." address $50 says " . cr
  [: 0 200 0 0 0 0 true i2c-xfer ;] ['] x-not-possible s" address 200 raises x-not-possible" raises
  0 i2c-close
  0 i2c-close
  1 400 0 i2c-open
  1 i2c-close
;
t-i2c

: t-spi
  [: 2 1000000 0 spi-open ;] ['] x-no-such-io s" spi-open bus 2 raises x-no-such-io" raises
  [: 0 tx rx 4 spi-xfer ;] ['] x-not-possible s" spi-xfer on a closed bus raises x-not-possible" raises
  0 1000000 0 spi-open
  tx 16 $A5 fill rx 16 $5A fill
  0 tx rx 4 spi-xfer
  ." SPI read back " rx c@ . rx 3 + c@ . rx 4 + c@ . cr
  rx 4 + c@ $5A s" spi-xfer: 4 bytes and no more" eq
  rx c@ $5A <> s" spi-xfer wrote what came in" check
  0 0 rx 4 spi-xfer
  0 tx 0 4 spi-xfer
  0 spi-close
  [: 0 tx rx 4 spi-xfer ;] ['] x-not-possible s" spi-xfer after close raises x-not-possible" raises
;
t-spi

\ COM1's RX from a bit-banged byte on GP9 if wired; its TX is read on its own pin, GP0
: send-bit ( b -- ) 9 pin! 833 delay-us ;
: send-byte ( c -- )
  0 send-bit
  8 0 do dup 1 and send-bit 1 rshift loop drop
  1 send-bit
;
: recv-byte ( -- c | -1 )
  now
  begin 0 pin@ while dup now swap - 200000 > if drop -1 exit then repeat
  drop
  1250 delay-us
  0 8 0 do 0 pin@ i lshift or 833 delay-us loop
;
: t-com
  [: 3 9600 8 0 1 0 com-open ;] ['] x-no-such-io s" com-open 3 raises x-no-such-io" raises
  [: 1 com-avail ;] ['] x-not-possible s" com-avail closed raises x-not-possible" raises
  [: tx 1 1 com-write ;] ['] x-not-possible s" com-write closed raises x-not-possible" raises
  [: rx 1 1 com-read ;] ['] x-not-possible s" com-read closed raises x-not-possible" raises
  1 1200 8 0 1 256 com-open
  [: 0 pin-out pull-none setpin ;] ['] x-pin-in-use s" a COM pin raises x-pin-in-use" raises
  1 com-avail 0 s" nothing waiting" eq
  rx 16 1 com-read 0 s" com-read does not wait" eq
  w1 if
    9 pin-out pull-none setpin 1 9 pin!
    5 ms
    $55 send-byte $A7 send-byte
    5 ms
    1 com-avail 2 s" COM1 received two" eq
    rx 16 1 com-read 2 s" com-read took two" eq
    rx c@ $55 s" the first byte" eq
    rx 1+ c@ $A7 s" the second byte" eq
    9 pin-off pull-none setpin
  else
    ." COM1 RX: no wire, not tested" cr
  then
  $C3 tx c!
  tx 1 1 com-write
  recv-byte $C3 s" COM1 sent a byte, watched on the pin" eq
  $5A tx c!
  tx 1 1 com-write
  recv-byte $5A s" and another" eq
  1 com-close
  1 com-close
  [: 1 com-avail ;] ['] x-not-possible s" after com-close raises x-not-possible" raises
  2 9600 7 1 2 0 com-open
  2 com-close
;
t-com
1 console-screen
ends
.s
