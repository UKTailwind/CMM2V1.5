\ cmm2.fs: the firmware's services as Forth words, in the module cmm2: the files, the screen, sound and music, the keyboard, mouse
\ and gamepads, the I/O pins and the system. WORDS.md says what each does. MIT licence, as zeptoforth.
\
\ Each word is a number in zf_services.c's table, given to the kernel's cmm2-call, and a check of what came back: the firmware
\ returns errors, and these words raise them, as exceptions of the kind files.fs has. Arguments come in MMBasic's order, and
\ strings as ( addr bytes ).

\ Compile to flash
compile-to-flash

begin-module cmm2

  systick import

  \ FatFs's errors, by the code the firmware gives (06_services's table)
  variable file-error
  : x-disk-error ( -- ) ." disk error" cr ;
  : x-not-ready ( -- ) ." the drive is not ready" cr ;
  : x-no-file ( -- ) ." no such file" cr ;
  : x-no-path ( -- ) ." no such path" cr ;
  : x-bad-name ( -- ) ." the name is not valid" cr ;
  : x-denied ( -- ) ." access denied" cr ;
  : x-exists ( -- ) ." the file is there already" cr ;
  : x-not-open ( -- ) ." not an open file or directory" cr ;
  : x-no-drive ( -- ) ." no such drive" cr ;
  : x-no-filesystem ( -- ) ." no file system on the drive" cr ;
  : x-no-memory ( -- ) ." not enough memory" cr ;
  : x-too-many-files ( -- ) ." too many open files" cr ;
  : x-file-error ( -- ) ." file error " file-error @ . cr ;

  \ The firmware's message for its last -4
  : io-error ( -- addr bytes ) 86 cmm2-call ;

  \ What the I/O, drawing and BASIC services report
  : x-no-such-io ( -- ) ." no such GP number, bus or port" cr ;
  : x-pin-in-use ( -- ) ." the pin is in use" cr ;
  : x-not-possible ( -- ) ." not possible: that mode on that pin, or a bus or port not open" cr ;
  : x-firmware ( -- ) io-error type cr ;
  : x-too-long ( -- ) ." too long" cr ;

  \ The screen's
  : x-bad-mode ( -- ) ." that mode cannot be set" cr ;
  : x-frame-buffer ( -- ) ." a FRAMEBUFFER of another depth is in the way" cr ;
  : x-no-display ( -- ) ." there is no display" cr ;
  : x-bad-palette ( -- ) ." the palette needs depth 8 and a range in 0-255" cr ;
  : x-bad-copy ( -- ) ." a copy's size, pitch or address is odd or out of range" cr ;
  : x-no-font ( -- ) ." no such font" cr ;

  \ Sound's
  : x-sound-busy ( -- ) ." sound is already playing" cr ;
  : x-no-room ( -- ) ." no room for the sound" cr ;
  : x-no-sound ( -- ) ." the sound would not start" cr ;
  : x-not-midi ( -- ) ." not a MIDI file" cr ;
  : x-midi-timing ( -- ) ." the MIDI file's timing is not supported" cr ;

  begin-module cmm2-internal

    \ The exception for a file service's error
    : file-exception ( n -- xt )
      case
        -1 of ['] x-disk-error endof
        -3 of ['] x-not-ready endof
        -4 of ['] x-no-file endof
        -5 of ['] x-no-path endof
        -6 of ['] x-bad-name endof
        -7 of ['] x-denied endof
        -8 of ['] x-exists endof
        -9 of ['] x-not-open endof
        -11 of ['] x-no-drive endof
        -13 of ['] x-no-filesystem endof
        -17 of ['] x-no-memory endof
        -18 of ['] x-too-many-files endof
        ['] x-file-error swap
      endcase
    ;

    \ The exception for an I/O service's error
    : io-exception ( n -- xt )
      case
        -1 of ['] x-no-such-io endof
        -2 of ['] x-pin-in-use endof
        -3 of ['] x-not-possible endof
        ['] x-firmware swap
      endcase
    ;

    \ What a file service gave, if it is not an error
    : ?file ( n -- n ) dup 0< if dup file-error ! file-exception ?raise then ;

    \ What an I/O service gave, if it is not an error
    : ?io ( n -- n ) dup 0< if io-exception ?raise then ;

    \ What a drawing or BASIC service gave: only -3 (too long) and -4 (the firmware's message) mean anything there
    : ?basic ( n -- n ) dup -3 = triggers x-too-long dup 0< if ['] x-firmware ?raise then ;

    \ What mode gave
    : ?mode ( n -- )
      dup -1 = triggers x-bad-mode
      -2 = triggers x-frame-buffer
    ;

    \ What the stream gave
    : ?audio ( n -- n )
      dup -2 = triggers x-sound-busy
      dup -3 = triggers x-no-room
      dup 0< triggers x-no-sound
    ;

    \ What music-play gave: the stream's errors are 10 lower (zf_services.c)
    : ?music ( n -- )
      dup -1 = triggers x-not-midi
      dup -2 = triggers x-midi-timing
      dup -3 = triggers x-no-room
      dup -12 = triggers x-sound-busy
      dup -13 = triggers x-no-room
      0< triggers x-no-sound
    ;

    \ The frames shown so far
    : frames ( -- count ) 25 cmm2-call ;

  end-module> import

  \ The console and the time

  \ Whether the console is drawn on the screen as well as sent to the serial port
  : console-screen ( flag -- ) 0 cmm2-call ;

  \ The microseconds since the board started
  : micros ( -- d ) 1 cmm2-call ;

  \ Wait, with nothing else running: for short waits. ms lets other tasks run
  : delay-us ( us -- ) 2 cmm2-call ;

  \ MMBasic's clock
  : datetime ( -- year month day hour minute second ) 3 cmm2-call ;

  \ Memory

  \ A block of MMBasic's heap, outside Forth's memory: 64 can be held, with the OPL chips
  : alloc ( bytes -- addr ) 58 cmm2-call dup 0= triggers x-no-memory ;
  : release ( addr -- ) 59 cmm2-call ;

  \ Files

  \ file-open's modes, combined with or
  1 constant o-read
  2 constant o-write
  4 constant o-create
  8 constant o-append

  : file-open ( addr bytes mode -- handle ) 4 cmm2-call ?file ;
  : file-close ( handle -- ) 5 cmm2-call ?file drop ;
  : file-read ( addr bytes handle -- count ) 6 cmm2-call ?file ;
  : file-write ( addr bytes handle -- count ) 7 cmm2-call ?file ;
  : file-seek ( pos handle -- ) 8 cmm2-call ?file drop ;
  : file-tell ( handle -- pos ) 9 cmm2-call ?file ;
  : file-size ( handle -- size ) 10 cmm2-call ?file ;
  : file-sync ( handle -- ) 11 cmm2-call ?file drop ;

  \ End a file where the handle is. It may come back with a new handle
  : file-truncate ( handle -- handle' ) 95 cmm2-call ?file ;
  : file-remove ( addr bytes -- ) 12 cmm2-call ?file drop ;
  : file-rename ( from-addr from-bytes to-addr to-bytes -- ) 13 cmm2-call ?file drop ;

  \ A file's or directory's details
  : file-stat ( addr bytes -- size dir? year month day hour minute second ) 14 cmm2-call ?file drop ;

  \ Is a file or directory there?
  : exists? ( addr bytes -- flag ) 15 cmm2-call ?file 0<> ;

  \ Directories

  : mkdir ( addr bytes -- ) 16 cmm2-call ?file drop ;

  \ The firmware removes an empty directory as it removes a file
  : rmdir ( addr bytes -- ) file-remove ;

  : dir-open ( addr bytes -- handle ) 17 cmm2-call ?file ;

  \ The next entry: the name stays until this handle's next dir-read
  : dir-read ( handle -- addr bytes size dir? true | false )
    18 cmm2-call ?file 0= if 2drop 2drop false else true then
  ;

  : dir-close ( handle -- ) 19 cmm2-call ?file drop ;
  : chdir ( addr bytes -- ) 20 cmm2-call ?basic drop ;

  \ The current directory: it stays until cwd$ is next used
  : cwd$ ( -- addr bytes ) 21 cmm2-call ?file drop ;

  continue-module cmm2-internal

    \ List a directory's entries
    : list-entries ( handle -- )
      >r
      begin r@ dir-read while
        if drop type ." /" else -rot type space . then cr
      repeat
      r> drop
    ;

    \ Read an open file into a new block
    : load-block ( handle -- block size )
      >r r@ file-size dup alloc swap 2dup r> file-read drop
    ;

  end-module

  \ List the current directory
  : files ( -- ) cwd$ dir-open dup >r ['] list-entries try r> dir-close ?raise ;

  \ A whole file in a block of memory, for the words that want one (music-play); release gives it back
  : file-load ( addr bytes -- block size )
    o-read file-open dup >r ['] load-block try r> file-close ?raise
  ;

  \ The console's font, 0 to 5 from 5 x 8 to 16 x 32, or -1 for the one that suits the mode
  : console-font ( index -- ) 93 cmm2-call 0< triggers x-no-font ;

  \ The console's size in characters, 0 for all there is
  : console-size ( cols rows -- ) 94 cmm2-call 0< triggers x-bad-mode ;

  \ The screen

  : mode ( mode depth -- ) 22 cmm2-call ?mode ;
  : mode-scaled ( mode depth scale -- ) 23 cmm2-call ?mode ;

  \ Palette entries from R, G, B bytes; shown from the next frame, which it waits for
  : palette ( start count addr -- ) 24 cmm2-call 0< triggers x-bad-palette ;

  \ Until the next frame starts, with other tasks running meanwhile. As video_wait, not more than 50 ms
  : wait-frame ( -- )
    frames systick-counter
    begin
      pause
      over frames = over systick-counter swap - 500 u< and
    while
    repeat
    2drop
  ;

  \ The page drawing goes to
  : gfx-target ( -- fb width height pitch bpp copies ) 26 cmm2-call 0< triggers x-no-display ;

  \ The size of the page drawing goes to
  : mm.hres ( -- pixels ) gfx-target 2drop 2drop nip ;
  : mm.vres ( -- pixels ) gfx-target 2drop drop nip nip ;

  \ The copies still to finish
  : dma2d-busy ( -- count ) 28 cmm2-call ;

  \ width bytes from each of height rows by the DMA2D, which goes on while the program does: three can wait, so this waits for a place
  : dma2d-copy ( src src-pitch dst dst-pitch width height -- )
    begin dma2d-busy 3 >= while pause repeat
    27 cmm2-call 0< triggers x-bad-copy
  ;

  \ MMBasic's colours, as RGB() gives them
  : rgb ( r g b -- colour ) swap 8 lshift or swap 16 lshift or $0F000000 or ;
  $00000000 constant black
  $0F0000FF constant blue
  $0F00FF00 constant green
  $0F00FFFF constant cyan
  $0FFF0000 constant red
  $0FFF00C0 constant magenta
  $0FFFFF00 constant yellow
  $0FA52A2A constant brown
  $0F404040 constant gray
  $0F808080 constant lightgray
  $0FFFFFFF constant white
  $0FFFA500 constant orange
  $0FFFA0AB constant pink
  $0FFFD700 constant gold
  $0FFA8072 constant salmon
  $0FF5F5DC constant beige
  $0FFF80FF constant lilac
  $0FFF40FF constant fuchsia
  $0FFF4000 constant rust
  $0F0080FF constant cerulean
  $0F008000 constant midgreen
  $0F0040FF constant cobalt
  $0F004000 constant myrtle

  \ A fill or background of -1 is none
  : cls ( colour -- ) 29 cmm2-call ?basic drop ;
  : pixel ( x y colour -- ) 30 cmm2-call ?basic drop ;
  : line ( x1 y1 x2 y2 width colour -- ) 31 cmm2-call ?basic drop ;
  : box ( x y w h width colour fill -- ) 32 cmm2-call ?basic drop ;
  : rbox ( x y w h radius colour fill -- ) 33 cmm2-call ?basic drop ;

  \ aspect is a double: 1de0 is round
  : circle ( x y r width aspect colour fill -- ) 34 cmm2-call ?basic drop ;

  : triangle ( x1 y1 x2 y2 x3 y3 colour fill -- ) 35 cmm2-call ?basic drop ;

  \ just is TEXT's ("" or "CM"...), font and scale 0 the console's
  : text ( x y addr bytes just-addr just-bytes font scale colour bg -- ) 36 cmm2-call ?basic drop ;

  \ The console's font
  : font ( font scale -- ) 37 cmm2-call ?basic drop 2drop ;

  \ The size of the console font's characters
  : mm.fontwidth ( -- pixels ) 0 0 37 cmm2-call ?basic drop drop ;
  : mm.fontheight ( -- pixels ) 0 0 37 cmm2-call ?basic drop nip ;

  \ A font's character size
  : font-size ( font -- width height ) 38 cmm2-call 0< triggers x-no-font ;

  \ Font 10-16 from memory, in DefineFont's layout; 0 puts back the one it replaced
  : font-define ( font addr -- ) 39 cmm2-call 0< triggers x-no-font ;

  \ The console's colours, and the drawing's when none is given
  : colour ( colour bg -- ) 40 cmm2-call ?basic drop ;

  \ An area of the page to or from memory: 3 bytes a pixel, blue, green, red
  : blit-read ( x y w h addr -- ) 41 cmm2-call ?basic drop ;
  : blit-write ( x y w h addr -- ) 42 cmm2-call ?basic drop ;

  \ PIXEL(x, y): 0 to $FFFFFF
  : pixel@ ( x y -- colour ) 43 cmm2-call ?basic drop ;

  \ The screen to a PNG file, or a BMP if the name ends in .BMP
  : save-screen ( addr bytes -- ) 44 cmm2-call ?basic drop ;

  \ IMAGE RESIZE_FAST: page -1 is the FRAMEBUFFER, -2 the page as it is
  : image-resize ( x y w h new-x new-y new-w new-h page keep-black -- ) 45 cmm2-call ?basic drop ;

  \ Sound

  \ The stream: frames of two 16-bit samples, left then right. The firmware calls for them from an interrupt, which cannot be
  \ Forth: audio-write leaves them in a queue, and silence goes out when it is empty

  : audio-start ( -- rate ) 46 cmm2-call ?audio ;
  : audio-stop ( -- ) 47 cmm2-call ;

  \ As many frames as the queue has room for
  : audio-write ( addr frames -- taken ) 48 cmm2-call ;

  \ The room in the queue
  : audio-room ( -- frames ) 49 cmm2-call ;

  \ All the frames, as the queue makes room
  : audio-play ( addr frames -- )
    begin dup 0> while
      2dup audio-write dup 0= if pause then
      tuck - -rot 4 * + swap
    repeat
    2drop
  ;

  \ A MIDI file in memory, which must stay there while it plays; bank 0 is the firmware's instruments
  : music-play ( addr bytes loop? bank -- ) 50 cmm2-call ?music ;
  : music-stop ( -- ) 51 cmm2-call ;
  : music-volume ( volume -- ) 52 cmm2-call ;
  : music-pause ( paused? -- ) 53 cmm2-call ;
  : music-playing? ( -- flag ) 54 cmm2-call ;
  : music-rate ( divisor -- ) 55 cmm2-call ;

  \ What is playing: 0 nothing; $100 is added once the last PLAY EFFECT has ended
  : sound-state ( -- state ) 56 cmm2-call ;

  \ PLAY TONE
  : tone ( left-hz right-hz ms -- ) 57 cmm2-call ?basic drop ;

  \ The firmware's OPL3
  : opl-new ( rate -- chip ) 60 cmm2-call dup 0= triggers x-no-memory ;
  : opl-write ( chip reg value -- ) 61 cmm2-call ;
  : opl-generate ( chip addr frames -- ) 62 cmm2-call ;
  : opl-free ( chip -- ) 63 cmm2-call ;

  \ Input

  \ KEYDOWN(n): 0 how many are held, 1-6 each key, 7 the modifiers
  : keydown ( n -- value ) 64 cmm2-call ;

  \ The USB keyboard's last report, 8 bytes at addr
  : keys-raw ( addr -- count ) 65 cmm2-call ;

  \ The mouse: flag is false, with all else 0, if there is none
  : mouse ( -- x y buttons wheel flag ) 66 cmm2-call ;
  : mouse-set ( x y -- ) 67 cmm2-call ;

  \ The pad in USB slot n, 1 up: buttons is -1 if there is none
  : gamepad ( n -- lx ly buttons ) 68 cmm2-call ;

  \ I/O, by the PicoMite's GP numbers

  \ setpin's modes, and its pulls for an input
  0 constant pin-off
  1 constant pin-in
  2 constant pin-out
  3 constant pin-od
  4 constant pin-analog
  0 constant pull-none
  1 constant pull-up
  2 constant pull-down

  : setpin ( gp mode pull -- ) 69 cmm2-call ?io drop ;
  : pin@ ( gp -- level ) 70 cmm2-call ?io ;
  : pin! ( level gp -- ) 71 cmm2-call ;

  \ 0-65535 and the microvolts
  : adc ( gp -- value microvolts ) 72 cmm2-call ?io swap ;

  : pwm ( gp freq duty -- ) 73 cmm2-call ?io drop ;
  : pwm-stop ( gp -- ) 74 cmm2-call ?io drop ;

  : i2c-open ( bus khz timeout -- ) 75 cmm2-call ?io drop ;
  : i2c-close ( bus -- ) 76 cmm2-call ?io drop ;

  \ 0, 1 for a NACK, 2 for a timeout, as MM.I2C: a device that does not answer is an answer
  : i2c-xfer ( bus addr w wn r rn stop? -- status ) 77 cmm2-call ?io ;

  : spi-open ( bus hz mode -- ) 78 cmm2-call ?io drop ;
  : spi-close ( bus -- ) 79 cmm2-call ?io drop ;

  \ tx 0 sends zeros, rx 0 drops what comes in
  : spi-xfer ( bus tx rx n -- ) 80 cmm2-call ?io drop ;

  : com-open ( com baud bits parity stop rxbuf -- ) 81 cmm2-call ?io drop ;
  : com-close ( com -- ) 82 cmm2-call ?io drop ;
  : com-write ( addr bytes com -- ) 83 cmm2-call ?io drop ;

  \ What has come, up to bytes: it does not wait
  : com-read ( addr bytes com -- count ) 84 cmm2-call ?io ;
  : com-avail ( com -- count ) 85 cmm2-call ?io ;

  \ The system

  : cpu-speed ( -- hz ) 87 cmm2-call ;

  \ One MMBasic statement, constants only
  : basic ( addr bytes -- ) 88 cmm2-call ?basic drop ;

  \ An MMBasic expression's value as text: it stays until basic-eval is next used
  : basic-eval ( addr bytes -- addr' bytes' ) 89 cmm2-call ?basic drop ;

  \ MMBasic's date and time, in the same buffer
  : date$ ( -- addr bytes ) s" DATE$" basic-eval ;
  : time$ ( -- addr bytes ) s" TIME$" basic-eval ;

  \ A statement tokenised once, and those tokens run
  : basic-prep ( addr bytes tokens size -- length ) 90 cmm2-call ?basic ;
  : basic-exec ( tokens -- ) 91 cmm2-call ?basic drop ;

  \ Back to MMBasic, which reports a code that is not 0
  : bye-code ( code -- ) 92 cmm2-call ;

end-module> import

\ Reboot
reboot
