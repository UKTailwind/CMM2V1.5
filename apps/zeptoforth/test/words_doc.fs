\ every example in WORDS.md, as printed there
0 console-screen
: load-it ( -- ) s" A:/level1.dat" o-read file-open file-close ;
: there? ( -- flag ) ['] load-it try dup ['] x-no-file = if drop false else ?raise true then ;
there? 0= s" doc: there? says false for a file that is not there" check
: load-it2 ( -- ) s" A:/zeptoforth/zt/a.bin" o-read file-open file-close ;
: there2? ( -- flag ) ['] load-it2 try dup ['] x-no-file = if drop false else ?raise true then ;
there2? s" doc: and true for one that is" check
create buf 16 allot
: save-it ( -- ) s" A:/hello.txt" o-write o-create or file-open >r s" Hello" r@ file-write drop r> file-close ;
: show-it ( -- ) s" A:/hello.txt" o-read file-open >r buf 16 r@ file-read r> file-close buf swap type ;
save-it show-it cr
: list-it ( -- )
  s" A:/" dir-open >r
  begin r@ dir-read while
    if drop type ." /" else -rot type space . then cr
  repeat
  r> dir-close
;
list-it
: picture ( -- )
  0 console-screen
  1 8 mode
  black cls
  40 40 200 120 2 red blue box
  260 40 200 120 16 yellow midgreen rbox
  600 100 60 3 1de0 cyan orange circle
  300 260 s" zeptoforth" s" CM" 1 3 white black text
  s" A:/zeptoforth/zt/picture.png" save-screen
;
picture
s" A:/zeptoforth/zt/picture.png" exists? s" doc: picture saved" check
create stmt 256 allot
: page-write ( n -- )
  s" PAGE WRITE " { addr u } addr stmt u move
  stmt u + swap format-integer nip u + stmt swap basic
;
0 page-write
\ a quote through s\": this file is not there, so the firmware's message says the command got as far as the file
: song ( -- ) s\" PLAY WAV \"A:/nothing.wav\"" basic ;
' song ' x-firmware s" doc: PLAY WAV of nothing raises x-firmware" raises
s\" at the prompt: \"quoted\"" type cr
io-error type cr
s" PLAY STOP" basic
s" MM.INFO(SOUND)" basic-eval type cr
ends
.s
1 console-screen
