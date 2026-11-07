create tune
$4D c, $54 c, $68 c, $64 c, $00 c, $00 c, $00 c, $06 c, $00 c, $00 c, $00 c, $01 c, $00 c, $60 c, $4D c, $54 c,
$72 c, $6B c, $00 c, $00 c, $00 c, $1E c, $00 c, $FF c, $51 c, $03 c, $07 c, $A1 c, $20 c, $00 c, $C0 c, $00 c,
$00 c, $90 c, $3C c, $32 c, $60 c, $80 c, $3C c, $00 c, $00 c, $90 c, $40 c, $32 c, $60 c, $80 c, $40 c, $00 c,
$00 c, $FF c, $2F c, $00 c,
52 constant tune-size
\ what the groups left: delay-us, micros, a tokenised statement with values written in, audio-stop stopping music
0 console-screen
: t-delay
  micros drop { t0 }
  5000 delay-us
  micros drop t0 - { us }
  ." delay-us 5000 took " us . cr
  us 5000 >= us 5500 < and s" delay-us" check
;
t-delay

: t-micros
  micros { lo hi }
  micros drop lo - { us }
  ." micros " lo u. hi . cr
  ." a call takes us " us . cr
  us 1000 < s" micros moves on, a little" check
;
t-micros

create stmt 64 allot
create toks 320 allot
create filled 320 allot
variable wi
variable tl
: put ( c -- ) filled wi @ + c! 1 wi +! ;
: put-n ( n -- ) filled wi @ + swap format-integer nip wi +! ;
\ a statement with ? where a value goes, as basic-prep wants it
: prep ( addr bytes -- ) { a u }
  a stmt u move
  u 0 do stmt i + c@ [char] ? = if 1 stmt i + c! then loop
  stmt u toks 320 basic-prep tl !
;
\ the tokens, each byte 1 replaced by the next value: x then y
: run-with ( x y -- ) { x y }
  0 wi ! 0 { k }
  tl @ 0 do
    toks i + c@ 1 = if k 0= if x else y then put-n k 1+ to k else toks i + c@ put then
  loop
  filled basic-exec
;
: t-prep-values
  black cls
  s" PIXEL ?,?,RGB(RED)" prep
  tl @ 0> s" basic-prep with places for values" check
  100 200 run-with
  100 200 pixel@ $E00000 s" basic-exec with values written in" eq
  300 400 run-with
  300 400 pixel@ $E00000 s" the same tokens with other values" eq
  100 400 pixel@ 0 s" nowhere else" eq
;
t-prep-values

: t-audio-stop-music
  20 music-volume
  tune tune-size 0 0 music-play
  100 ms
  music-playing? s" music playing" check
  audio-stop
  music-playing? 0= s" audio-stop stops the music" check
  sound-state $FF and 0 s" and the stream" eq
  127 music-volume
;
t-audio-stop-music
1 console-screen
ends
.s
