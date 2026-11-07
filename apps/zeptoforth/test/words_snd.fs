create tune
$4D c, $54 c, $68 c, $64 c, $00 c, $00 c, $00 c, $06 c, $00 c, $00 c, $00 c, $01 c, $00 c, $60 c, $4D c, $54 c,
$72 c, $6B c, $00 c, $00 c, $00 c, $1E c, $00 c, $FF c, $51 c, $03 c, $07 c, $A1 c, $20 c, $00 c, $C0 c, $00 c,
$00 c, $90 c, $3C c, $32 c, $60 c, $80 c, $3C c, $00 c, $00 c, $90 c, $40 c, $32 c, $60 c, $80 c, $40 c, $00 c,
$00 c, $FF c, $2F c, $00 c,
52 constant tune-size
\ sound: the stream's rate by the queue draining, the state words, music timing; a little real sound, short and quiet
0 console-screen
: state sound-state $FF and ;
: now ( -- us ) micros drop ;
1024 4 * constant zb
create zeros 8192 4 * allot
zeros 8192 4 * 0 fill

: t-stream
  state 0 s" nothing playing at the start" eq
  audio-start { rate }
  ." rate " rate . cr
  rate 43000 > rate 45000 < and s" audio-start's rate is about 44100" check
  audio-start rate s" audio-start again gives the rate" eq
  state 23 s" sound-state says P_STREAM" eq
  audio-room 8192 s" the queue is empty" eq
  zeros 8192 audio-write 8192 s" audio-write fills the queue" eq
  zeros 100 audio-write dup ." a write to a full queue took " . cr 1024 <= s" a full queue takes little or none" check
  now { t0 }
  begin audio-room 8192 < while pause repeat
  now t0 - { us }
  ." the queue drained in us " us . cr
  us 120000 > us 230000 < and s" it drains at about the rate" check
  audio-stop
  state 0 s" audio-stop stops it" eq
;
t-stream

\ a real 440 Hz square wave for a tenth of a second, quiet
create sq 4410 4 * allot
: fill-sq
  4410 0 do
    i 100 / 1 and if 1000 else -1000 then $FFFF and dup 16 lshift or sq i 4 * + !
  loop
;
fill-sq
: t-play
  audio-start drop
  now { t0 }
  sq 4410 audio-play
  now t0 - dup ." audio-play returned in us " . cr 50000 < s" audio-play of less than the queue does not wait" check
  zeros 8192 audio-play
  now t0 - { us } ." audio-play of more than the queue took us " us . cr
  us 10000 > s" audio-play waits for room" check
  100 ms
  audio-stop
;
t-play

: t-tone
  400 400 150 tone
  state 2 s" tone: sound-state says P_TONE" eq
  [: audio-start ;] ['] x-sound-busy s" audio-start under a tone raises x-sound-busy" raises
  [: tune tune-size 0 0 music-play ;] ['] x-sound-busy s" music-play under a tone raises x-sound-busy" raises
  300 ms
  state 0 s" the tone ends by itself" eq
  audio-start drop
  state 23 s" audio-start after a tone" eq
  [: 400 400 100 tone ;] ['] x-firmware s" tone over the stream raises x-firmware" raises
  ." its message: " io-error type cr
  audio-stop
  400 400 2000 tone
  s" PLAY STOP" basic
  state 0 s" PLAY STOP through basic" eq
;
t-tone

: t-music
  20 music-volume
  [: zeros 100 0 0 music-play ;] ['] x-not-midi s" zeros raise x-not-midi" raises
  tune tune-size 0 0 music-play
  music-playing? s" music-play: playing" check
  state 23 s" music-play started the stream" eq
  now { t0 }
  begin music-playing? while 10 ms now t0 - 6000000 > if music-stop then repeat
  now t0 - { us } ." the tune took us " us . cr
  us 2500000 > us 4000000 < and s" a tune of 1 s and its 2 s of release" check
  tune tune-size 0 0 music-play
  300 ms
  true music-pause
  now to t0
  1000 ms
  false music-pause
  begin music-playing? while 10 ms now t0 - 8000000 > if music-stop then repeat
  now t0 - to us ." paused for 1 s, then took us " us . cr
  us 3200000 > us 4800000 < and s" music-pause held it" check
  tune tune-size true 0 music-play
  3500 ms
  music-playing? s" a looped tune goes on past its end" check
  music-stop
  music-playing? 0= s" music-stop stops it" check
  4 music-rate
  tune tune-size 0 0 music-play
  200 ms
  music-playing? s" music-rate 4: still plays" check
  music-stop
  1 music-rate
  127 music-volume
  audio-stop
;
t-music

\ an OPL3 chip of its own: nothing before the key goes down, and sound after
create obuf 1024 4 * allot
: nonzero ( -- n ) 0 1024 2 * 0 do obuf i 2 * + h@ 0<> if 1+ then loop ;
: t-opl
  44100 opl-new { chip }
  chip 0<> s" opl-new" check
  chip obuf 1024 opl-generate
  nonzero 0 s" a chip with no key down is silent" eq
  chip $20 $01 opl-write chip $23 $01 opl-write
  chip $40 $10 opl-write chip $43 $20 opl-write
  chip $60 $F0 opl-write chip $63 $F0 opl-write
  chip $80 $77 opl-write chip $83 $77 opl-write
  chip $C0 $31 opl-write
  chip $A0 $98 opl-write chip $B0 $31 opl-write
  chip obuf 1024 opl-generate
  nonzero dup ." samples sounding " . cr 1000 > s" with the key down it sounds" check
  audio-start drop
  6 0 do chip obuf 1024 opl-generate obuf 1024 audio-play loop
  100 ms
  audio-stop
  chip opl-free
;
t-opl

: t-alloc
  1000 alloc { a }
  a 0<> s" alloc" check
  a 1000 $5A fill
  a 999 + c@ $5A s" alloc's block holds" eq
  a release
  [: $7FFFFFF0 alloc ;] ['] x-no-memory s" alloc of too much raises x-no-memory" raises
;
t-alloc

create blocks 70 cells allot
variable got
: take-all 0 got ! 70 0 do 64 alloc blocks i cells + ! 1 got +! loop ;
: t-limit
  ['] take-all try ['] x-no-memory s" the blocks run out: x-no-memory" eq
  got @ dup ." blocks taken " . cr 64 s" 64 blocks" eq
  got @ 0 do blocks i cells + @ release loop
  100 alloc dup 0<> s" after releasing, alloc works" check release
;
t-limit
1 console-screen
ends
.s
