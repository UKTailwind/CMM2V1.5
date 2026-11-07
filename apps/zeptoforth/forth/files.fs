\ files.fs: Forth source from the SD card. The names are MMBasic's, relative to its current directory, or A:/dir/name.
\ include program.fs at the prompt or in a file, as a file may include another: up to four deep. MIT licence, as zeptoforth.

\ Compile to flash
compile-to-flash

\ Raised for a file that cannot be opened
: x-cannot-open ( -- ) ." cannot open the file" cr ;

\ Interpret a file's lines ( handle -- ). The handle stays off the data stack: a definition over several lines keeps its
\ control structures there from one line to the next
: read-lines ( handle -- )
  >r
  begin r@ cmm2-line dup 0>= while evaluate repeat
  2drop r> drop
;

\ Interpret a file ( addr bytes -- )
: included ( addr bytes -- )
  cmm2-open dup 0< triggers x-cannot-open
  dup >r ['] read-lines try r> cmm2-close ?raise
;

\ Interpret the file named next ( "name" -- )
: include ( "name" -- ) token included ;

\ Reboot
reboot
