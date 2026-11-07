\ the harness itself: these two must print FAIL, and the count must say 2
false s" DELIBERATE: harness check prints FAIL" check
1 2 s" DELIBERATE: harness eq prints FAIL" eq
fails @ 2 s" DELIBERATE: two fails counted" eq
0 fails ! 0 checks !
0 console-screen

variable h
\ an 8-bit mode's pixels read back in the top three bits of each colour
: at? ( x y colour addr bytes -- ) 2>r $E0E0E0 and -rot pixel@ swap 2r> eq ;
: lit ( x y w h -- n ) { x y w h } 0 h 0 do w 0 do x i + y j + pixel@ 0<> if 1+ then loop loop ;

\ errors of mode first, then the display
: t-mode-errors
  [: 99 8 mode ;] ['] x-bad-mode s" mode 99 raises x-bad-mode" raises
  [: 1 4 mode ;] ['] x-bad-mode s" depth 4 raises x-bad-mode" raises
  [: 7 8 2 mode-scaled ;] ['] x-bad-mode s" scale 2 in a doubled mode raises x-bad-mode" raises
  [: 1 8 3 mode-scaled ;] ['] x-bad-mode s" scale 3 raises x-bad-mode" raises
;
t-mode-errors

: t-target1
  1 8 mode
  gfx-target { fb w h p bpp c }
  w 800 s" gfx-target width in mode 1" eq
  h 600 s" gfx-target height in mode 1" eq
  p 800 s" gfx-target pitch in mode 1" eq
  bpp 8 s" gfx-target bpp" eq
  c 1 s" gfx-target copies 1" eq
  fb 0<> s" gfx-target fb" check
  mm.hres 800 s" mm.hres" eq
  mm.vres 600 s" mm.vres" eq
;
t-target1

: t-primary
  red cls 10 10 red s" cls red" at? 799 599 red s" cls red, far corner" at?
  green cls 10 10 green s" cls green" at?
  blue cls 10 10 blue s" cls blue" at?
  white cls 10 10 white s" cls white" at?
  black cls 10 10 black s" cls black" at?
  0 0 100 100 lit 0 s" nothing lit after cls black" eq
;
t-primary

: t-pixel
  50 60 red pixel
  50 60 red s" pixel" at?
  51 60 black s" not its neighbour" at?
  0 0 100 100 lit 1 s" one pixel lit" eq
;
t-pixel

: t-line
  black cls
  100 20 199 20 1 green line
  100 20 green s" line start" at?
  199 20 green s" line end" at?
  200 20 black s" line stops" at?
  100 19 black s" line one wide" at?
  90 15 120 10 lit 100 s" line: 100 pixels" eq
  100 40 199 40 3 green line
  150 39 black s" width 3: nothing above y" at?
  150 42 green s" width 3: y to y+2" at?
  150 43 black s" width 3 no further" at?
  10 100 60 150 1 yellow line
  35 125 yellow s" a diagonal's middle" at?
;
t-line

: t-box
  black cls
  100 100 50 40 1 red -1 box
  100 100 red s" box corner" at?
  149 139 red s" box far corner" at?
  150 139 black s" box w is its size" at?
  120 120 black s" box -1 fill is none" at?
  200 100 50 40 2 red blue box
  201 101 red s" box width 2" at?
  202 102 blue s" box filled" at?
  225 120 blue s" box filled middle" at?
  300 100 60 50 12 white cyan rbox
  300 100 black s" rbox's corner is cut" at?
  330 100 white s" rbox's edge" at?
  330 125 cyan s" rbox filled" at?
;
t-box

: t-circle
  black cls
  100 100 30 1 1de0 red -1 circle
  130 100 red s" circle's right" at?
  100 70 red s" circle's top" at?
  100 100 black s" circle -1 fill is none" at?
  131 100 black s" circle's radius" at?
  300 100 30 1 2de0 green blue circle
  300 100 blue s" circle filled" at?
  355 100 blue s" aspect 2 reaches 55 across" at?
  300 135 black s" aspect 2 leaves the height at 30" at?
  500 100 30 1 0.5de0 green blue circle
  500 100 blue s" aspect 0.5 filled" at?
  520 100 black s" aspect 0.5 is 15 across" at?
;
t-circle

: t-triangle
  black cls
  100 100 200 100 150 180 red yellow triangle
  150 120 yellow s" triangle filled" at?
  105 170 black s" outside a triangle" at?
  300 100 400 100 350 180 red -1 triangle
  350 100 red s" triangle's edge" at?
  350 120 black s" triangle -1 fill is none" at?
;
t-triangle

: t-text
  black cls
  100 100 s" X" s" " 1 1 white black text
  1 font-size { w h }
  ." font 1 size " w . h . cr
  100 100 w h lit 0> s" text draws" check
  100 100 h + 20 20 lit 0 s" text stays in its cell, below" eq
  100 w + 100 20 20 lit 0 s" text stays in its cell, to the right" eq
  300 300 s" X" s" CM" 1 3 white -1 text
  300 w 3 * 2 / - 300 h 3 * 2 / - w 3 * h 3 * lit 0> s" text CM scale 3 inside its box" check
  0 0 800 600 lit { all }
  300 w 3 * 2 / - 300 h 3 * 2 / - w 3 * h 3 * lit 100 100 w h lit + all s" all that is lit is in those two boxes" eq
  [: 10 10 s" a" s" " 99 1 white black text ;] ['] x-firmware s" text font 99 raises x-firmware" raises
  ." its message: " io-error type cr
  [: 10 10 s" a" s" QQ" 1 1 white black text ;] ['] x-firmware s" text bad justification raises x-firmware" raises
  ." its message: " io-error type cr
  [: 10 10 s" a" s" " 1 99 white black text ;] ['] x-firmware s" text scale 99 raises x-firmware" raises
;
t-text

create glyph 8 c, 8 c, 65 c, 1 c, $FF c, $FF c, $FF c, $FF c, $FF c, $FF c, $FF c, $FF c,
: t-font
  mm.fontwidth mm.fontheight { w0 h0 }
  ." console font " w0 . h0 . cr
  2 1 font
  mm.fontheight 2 font-size nip s" font sets the console's: height" eq
  mm.fontwidth 2 font-size drop s" font sets the console's: width" eq
  1 1 font
  mm.fontwidth 1 font-size drop s" font back to 1" eq
  [: 99 1 font ;] ['] x-firmware s" font 99 raises x-firmware" raises
  [: 99 font-size ;] ['] x-no-font s" font-size 99 raises x-no-font" raises
  [: 16 font-size ;] ['] x-no-font s" font 16 not loaded raises x-no-font" raises
  [: 3 glyph font-define ;] ['] x-no-font s" font-define of 3 (built in) raises x-no-font" raises
  16 glyph font-define
  16 font-size 8 s" font-define: its height" eq 8 s" font-define: its width" eq
  black cls
  50 50 s" A" s" " 16 1 white black text
  0 0 100 100 lit 64 s" font-define: A is an 8 x 8 block" eq
  16 0 font-define
  [: 16 font-size ;] ['] x-no-font s" font-define 0 puts back what it replaced" raises
;
t-font

: count-colour ( colour -- n ) { c } 0 600 0 do 800 0 do i j pixel@ c = if 1+ then loop loop ;
: t-colour
  black cls
  1 console-screen
  red black colour
  ." X"
  0 console-screen
  $E00000 count-colour 0> s" colour red: the console prints red" check
  $E0E0E0 count-colour 0 s" colour red: no white" eq
  1 console-screen
  white black colour
  ." X"
  0 console-screen
  $E0E0E0 count-colour 0> s" colour white: back to white" check
;
t-colour

: >num ( addr bytes -- n ) 0 -rot 0 ?do dup i + c@ [char] 0 - rot 10 * + swap loop drop ;
: t-pixel-basic
  black cls
  100 100 orange pixel
  100 100 pixel@ s" PIXEL(100,100)" basic-eval >num s" pixel@ is MMBasic's PIXEL()" eq
  100 100 pixel@ 0<> s" and it is not black" check
;
t-pixel-basic

create bmphdr 64 allot
create blit 30 20 * 3 * allot
: t-blit
  black cls
  10 10 30 20 1 red green box
  10 10 30 20 blit blit-read
  blit c@ 0 s" blit-read: blue byte of red" eq
  blit 1+ c@ 0 s" blit-read: green byte of red" eq
  blit 2 + c@ 224 s" blit-read: red byte of red" eq
  blit 15 30 * 3 * 15 3 * + + 1+ c@ 224 s" blit-read: green in the middle" eq
  100 100 30 20 blit blit-write
  100 100 red s" blit-write corner" at?
  115 115 green s" blit-write middle" at?
  130 120 black s" blit-write size" at?
  [: 10 10 30 20 blit blit-write ;] try 0 s" blit-write to the same place" eq
;
t-blit

: png$ s" A:/zeptoforth/zt/shot.png" ;
: bmp$ s" A:/zeptoforth/zt/shot.bmp" ;
: t-save
  png$ exists? if png$ file-remove then
  bmp$ exists? if bmp$ file-remove then
  png$ save-screen
  png$ file-stat 2drop 2drop 2drop drop dup ." PNG bytes " . cr 0> s" save-screen PNG" check
  bmp$ save-screen
  bmp$ file-stat 2drop 2drop 2drop drop dup ." BMP bytes " . cr 0> s" save-screen BMP" check
  bmp$ o-read file-open h !
  bmphdr 54 h @ file-read drop h @ file-close
  bmphdr c@ [char] B s" BMP: B" eq
  bmphdr 1+ c@ [char] M s" BMP: M" eq
  bmphdr 18 + @ 800 s" BMP width" eq
  bmphdr 22 + @ 600 s" BMP height" eq
  ." BMP bits " bmphdr 28 + h@ . ." compression " bmphdr 30 + @ . ." image bytes " bmphdr 34 + @ . cr
  [: s" A:/zeptoforth/nodir/x.png" save-screen ;] ['] x-firmware s" save-screen to no path raises x-firmware" raises
  ." its message: " io-error type cr
;
t-save

create crgb 12 c, 34 c, 56 c, 200 c, 100 c, 50 c,
: t-palette
  black cls
  gfx-target 2drop 2drop { fb w }
  200 fb 10 800 * 10 + + c!
  201 fb 10 800 * 11 + + c!
  ." fb " hex fb . decimal ." bytes " fb 10 800 * 10 + + c@ . fb 10 800 * 11 + + c@ . cr
  ." crgb " crgb 6 0 do dup i + c@ . loop drop cr
  200 2 crgb palette
  ." bytes after " fb 10 800 * 10 + + c@ . fb 10 800 * 11 + + c@ . cr
  10 10 pixel@ ." palette entry 200 reads " hex . decimal cr
  11 10 pixel@ ." palette entry 201 reads " hex . decimal cr
  [: 250 10 crgb palette ;] ['] x-bad-palette s" palette past 255 raises x-bad-palette" raises
;
t-palette

: t-target7
  7 8 mode
  gfx-target { fb w h p bpp c }
  w 320 s" mode 7 width" eq
  h 240 s" mode 7 height" eq
  p 640 s" mode 7 pitch" eq
  c 2 s" mode 7 copies 2" eq
  black cls
  100 100 white pixel
  fb 100 p * 100 + + c@ fb 100 p * 100 + w + + c@ s" both copies of the line hold the pixel" eq
  fb 100 p * 100 + + c@ 0<> s" the page has it" check
  1 8 2 mode-scaled
  gfx-target { fb2 w2 h2 p2 bpp2 c2 }
  w2 400 s" mode 1 scaled 2 width" eq
  h2 300 s" mode 1 scaled 2 height" eq
  c2 2 s" scaled 2 copies 2" eq
  1 16 mode
  gfx-target { fb3 w3 h3 p3 bpp3 c3 }
  bpp3 16 s" depth 16 bpp" eq
  p3 1600 s" depth 16 pitch" eq
  black cls
  10 10 $0F123456 pixel
  10 10 pixel@ ." depth 16 reads " hex . decimal cr
  ." depth 16 fb " hex fb3 . decimal ." pitch " p3 . ." copies " c3 . cr
  fb3 10 p3 * 20 + + h@ ." depth 16 page " hex . decimal cr
  fb3 10 p3 * 20 + + @ ." depth 16 word " hex . decimal cr
  0 $80000 0 do fb3 i + h@ 0<> if drop i leave then 2 +loop ." depth 16: first non-zero at byte " . cr
  1 8 mode
;
t-target7

task import
variable spun
: spin begin 1 spun +! pause again ;
0 ' spin 320 128 512 spawn constant spinner
: t-frames
  micros drop { t0 }
  60 0 do wait-frame loop
  micros drop t0 - { us }
  ." 60 frames in us " us . cr
  us 500000 > us 1200000 < and s" wait-frame paces at 50-120 Hz" check
  0 spun ! spinner run
  micros drop to t0
  60 0 do wait-frame loop
  micros drop t0 - to us
  spinner stop
  ." with a task: 60 frames in us " us . ." spun " spun @ . cr
  us 500000 > us 1200000 < and s" wait-frame paces with a task beside it" check
  spun @ 100 > s" the task ran meanwhile" check
;
t-frames

create src 64 64 * allot
: t-dma
  black cls
  src 64 64 * 200 fill
  gfx-target 2drop 2drop { fb w }
  src 64 fb 100 800 * + 100 + 800 64 64 dma2d-copy
  begin dma2d-busy 0= until
  fb 100 800 * + 100 + c@ 200 s" dma2d-copy: first byte" eq
  fb 163 800 * + 163 + c@ 200 s" dma2d-copy: last byte" eq
  fb 164 800 * + 100 + c@ 0 s" dma2d-copy: below stays" eq
  fb 100 800 * + 164 + c@ 0 s" dma2d-copy: to the right stays" eq
  [: src 1+ 63 src 800 3 5 dma2d-copy ;] ['] x-bad-copy s" dma2d-copy odd raises x-bad-copy" raises
  10 0 do src 64 fb 200 800 * + 100 + 800 64 64 dma2d-copy loop
  begin dma2d-busy 0= until
  fb 263 800 * + 163 + c@ 200 s" dma2d-copy: ten in a row, waiting for places" eq
;
t-dma

: t-resize
  black cls
  10 10 20 20 1 red red box
  10 10 20 20 100 100 40 40 -2 -2 image-resize
  100 100 red s" image-resize: its corner" at?
  139 139 red s" image-resize: twice the size" at?
  140 140 black s" image-resize: no further" at?
  [: 0 0 0 0 0 0 0 0 99 0 image-resize ;] ['] x-firmware s" image-resize page 99 raises x-firmware" raises
;
t-resize

create toks 320 allot
: t-basic
  black cls
  s" BOX 100,100,50,50,1,RGB(RED),RGB(BLUE)" basic
  100 100 red s" basic: BOX" at?
  120 120 blue s" basic: BOX filled" at?
  [: s" NOSUCHCOMMAND 1" basic ;] ['] x-firmware s" basic of nonsense raises x-firmware" raises
  ." its message: " io-error type cr
  [: src 300 basic ;] ['] x-too-long s" basic of 300 bytes raises x-too-long" raises
  s" 3*7" basic-eval s" 21" equal-strings? s" basic-eval 3*7" check
  s" MM.VER" basic-eval type cr
  [: s" 1+" basic-eval ;] ['] x-firmware s" basic-eval of nonsense raises x-firmware" raises
  ." its message: " io-error type cr
  s" PIXEL 200,200,RGB(GREEN)" toks 320 basic-prep dup 0> s" basic-prep gives tokens" check ." tokens " . cr
  toks basic-exec
  200 200 green s" basic-exec" at?
  [: s" NOSUCH 1,2" toks 320 basic-prep ;] ['] x-firmware s" basic-prep of nonsense raises x-firmware" raises
;
t-basic

: t-console
  black cls
  0 console-screen
  ." NOT ON THE SCREEN" cr
  0 0 800 600 lit 0 s" console-screen 0: nothing drawn" eq
  1 console-screen
  ." ON THE SCREEN" cr
  0 console-screen
  0 0 800 600 lit 0> s" console-screen 1: drawn" check
;
t-console

: t-pic
  black cls
  20 20 760 560 2 white -1 box
  40 40 200 120 2 red blue box
  260 40 200 120 16 yellow midgreen rbox
  600 100 60 3 1de0 cyan orange circle
  40 200 240 200 140 320 white magenta triangle
  300 260 s" zeptoforth" s" CM" 1 3 white black text
  300 300 s" on the CMM2" s" CM" 2 1 gold -1 text
  300 200 500 330 2 pink line
  600 260 60 1 1.5de0 lilac -1 circle
;
t-pic
1 console-screen
ends
.s
