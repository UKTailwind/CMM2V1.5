\ files: write, read back, compare; each error path; test files stay on the card
10000 constant big
create wbuf big allot
create rbuf big allot
variable h
: pat ( i -- c ) dup 7 * swap 8 rshift xor 255 and ;
: fillw big 0 do i pat wbuf i + c! loop ;
: clearr rbuf big 0 fill ;
: same? ( addr1 addr2 n -- flag ) 0 ?do over i + c@ over i + c@ <> if 2drop false unloop exit then loop 2drop true ;
fillw
: dir$ s" A:/zeptoforth/zt" ;
: a$ s" A:/zeptoforth/zt/a.bin" ;
: b$ s" A:/zeptoforth/zt/b.bin" ;
: c$ s" A:/zeptoforth/zt/c.bin" ;
: d$ s" A:/zeptoforth/zt/d.bin" ;
: nofile$ s" A:/zeptoforth/zt/nothing.bin" ;
: nopath$ s" A:/zeptoforth/nodir/x.bin" ;
: sub$ s" A:/zeptoforth/zt/sub" ;

: t-mkdir
  dir$ exists? not if dir$ mkdir then
  dir$ exists? s" mkdir: directory there" check
  [: dir$ mkdir ;] ['] x-exists s" mkdir on an existing one raises x-exists" raises
;
t-mkdir

: t-write
  a$ o-write o-create or file-open h !
  wbuf 4000 h @ file-write 4000 s" file-write counts 4000" eq
  h @ file-tell 4000 s" file-tell 4000" eq
  h @ file-sync
  a$ file-stat 2drop 2drop 2drop drop 4000 s" file-sync: on the card, size 4000" eq
  wbuf 4000 + big 4000 - h @ file-write big 4000 - s" file-write the rest" eq
  h @ file-size big s" file-size big" eq
  h @ file-close
;
t-write

: t-read
  clearr
  a$ o-read file-open h !
  h @ file-size big s" size after reopen" eq
  rbuf big h @ file-read big s" file-read all" eq
  wbuf rbuf big same? s" read back compares" check
  rbuf 16 h @ file-read 0 s" at the end file-read gives 0" eq
  5000 h @ file-seek
  h @ file-tell 5000 s" file-seek, file-tell" eq
  clearr
  rbuf 16 h @ file-read 16 s" read 16 after seek" eq
  wbuf 5000 + rbuf 16 same? s" those 16 are the file's" check
  big 10 - h @ file-seek
  rbuf 100 h @ file-read 10 s" a short read at the end" eq
  [: wbuf 4 h @ file-write ;] ['] x-denied s" write to a file open to read raises x-denied" raises
  h @ file-close
  [: h @ file-tell ;] ['] x-not-open s" file-tell on a closed handle raises x-not-open" raises
  [: h @ file-close ;] ['] x-not-open s" file-close twice raises x-not-open" raises
  [: rbuf 4 99 file-read ;] ['] x-not-open s" file-read of handle 99 raises x-not-open" raises
;
t-read

: t-append
  a$ o-write o-append or file-open h !
  wbuf 10 h @ file-write 10 s" append 10" eq
  h @ file-close
  a$ o-read file-open h !
  h @ file-size big 10 + s" size after append" eq
  big h @ file-seek
  rbuf 10 h @ file-read 10 s" read the appended" eq
  wbuf rbuf 10 same? s" appended bytes compare" check
  h @ file-close
;
t-append

: t-inplace
  a$ o-read o-write or file-open h !
  100 h @ file-seek
  s" HELLO" h @ file-write 5 s" write in place" eq
  100 h @ file-seek
  rbuf 5 h @ file-read 5 s" read that back" eq
  rbuf 5 s" HELLO" equal-strings? s" it says HELLO" check
  h @ file-size big 10 + s" size unchanged" eq
  h @ file-close
;
t-inplace

: t-rename
  b$ exists? if b$ file-remove then
  c$ exists? if c$ file-remove then
  b$ o-write o-create or file-open h ! wbuf 77 h @ file-write drop h @ file-close
  b$ c$ file-rename
  b$ exists? 0= s" rename: the old name gone" check
  c$ exists? s" rename: the new name there" check
  c$ file-stat 2drop 2drop 2drop drop 77 s" rename: its size kept" eq
  [: c$ a$ file-rename ;] ['] x-exists s" rename onto a file raises x-exists" raises
  [: nofile$ d$ file-rename ;] ['] x-no-file s" rename of nothing raises x-no-file" raises
;
t-rename

: t-remove
  d$ o-write o-create or file-open h ! h @ file-close
  d$ exists? s" made one to remove" check
  d$ file-remove
  d$ exists? 0= s" file-remove: gone" check
  [: d$ file-remove ;] ['] x-no-file s" file-remove of nothing raises x-no-file" raises
;
t-remove

: t-stat
  a$ file-stat { size dir y m d hh mm ss }
  size big 10 + s" file-stat size" eq
  dir 0 s" file-stat: not a directory" eq
  datetime 2drop drop { yy mo dd }
  y yy s" file-stat year is today's" eq
  m mo s" file-stat month is today's" eq
  d dd s" file-stat day is today's" eq
  dir$ file-stat { size2 dir2 y2 m2 d2 h2 mi2 s2 }
  dir2 -1 s" file-stat: a directory" eq
  size2 0 s" file-stat: a directory's size 0" eq
  s" A:/" file-stat 2drop 2drop 2drop nip -1 s" file-stat of a root" eq
  [: nofile$ file-stat ;] ['] x-no-file s" file-stat of nothing raises x-no-file" raises
  nofile$ exists? 0= s" exists? false for nothing" check
  nopath$ exists? 0= s" exists? false for no path" check
  a$ exists? s" exists? true for a file" check
  dir$ exists? s" exists? true for a directory" check
;
t-stat

: t-open-errors
  [: nofile$ o-read file-open ;] ['] x-no-file s" open of nothing raises x-no-file" raises
  [: nopath$ o-read file-open ;] ['] x-no-path s" open on no path raises x-no-path" raises
  [: nopath$ o-write o-create or file-open ;] ['] x-no-path s" create on no path raises x-no-path" raises
  [: s" Z:/x" o-read file-open ;] ['] x-no-drive s" open on no drive raises x-no-drive" raises
  [: wbuf 300 o-read file-open ;] ['] x-bad-name s" a 300-byte name raises x-bad-name" raises
;
t-open-errors

create hs 9 cells allot
: t-many
  8 0 do a$ o-read file-open hs i cells + ! loop
  [: a$ o-read file-open ;] ['] x-too-many-files s" a ninth file raises x-too-many-files" raises
  8 0 do hs i cells + @ file-close loop
  a$ o-read file-open dup 0>= s" after closing, a file opens" check file-close
;
t-many

variable seen-a
variable seen-c
variable seen-sub
variable entries
variable a-size
: note-entry ( addr bytes size dir? -- )
  1 entries +!
  { addr bytes size dir? }
  addr bytes s" a.bin" equal-strings? if true seen-a ! size a-size ! dir? 0 s" list: a.bin not a directory" eq then
  addr bytes s" c.bin" equal-strings? if true seen-c ! then
  addr bytes s" sub" equal-strings? if true seen-sub ! dir? -1 s" list: sub is a directory" eq then
;
: t-dir
  sub$ exists? not if sub$ mkdir then
  0 seen-a ! 0 seen-c ! 0 seen-sub ! 0 entries ! 0 a-size !
  dir$ dir-open h !
  begin h @ dir-read while note-entry repeat
  h @ dir-read 0 s" dir-read after the end still says false" eq
  h @ dir-close
  seen-a @ s" list sees a.bin" check
  seen-c @ s" list sees c.bin" check
  seen-sub @ s" list sees sub" check
  a-size @ big 10 + s" list gives a.bin's size" eq
  entries @ 3 >= s" list: all the entries" check
  [: nopath$ dir-open ;] ['] x-no-path s" dir-open of nothing raises x-no-path" raises
  [: h @ dir-read ;] ['] x-not-open s" dir-read closed raises x-not-open" raises
  [: h @ dir-close ;] ['] x-not-open s" dir-close twice raises x-not-open" raises
;
t-dir

: t-chdir
  cwd$ s" A:/zeptoforth" equal-strings? s" cwd$ at the start" check
  dir$ chdir
  cwd$ dir$ equal-strings? s" chdir, cwd$" check
  s" a.bin" exists? s" a relative name after chdir" check
  s" sub" chdir
  cwd$ sub$ equal-strings? s" chdir relative" check
  s" A:/zeptoforth" chdir
  cwd$ s" A:/zeptoforth" equal-strings? s" chdir back" check
  [: s" A:/zeptoforth/nodir" chdir ;] ['] x-firmware s" chdir to nothing raises x-firmware" raises
  ." its message: " io-error type cr
  cwd$ s" A:/zeptoforth" equal-strings? s" where it was" check
;
t-chdir

: t-rmdir
  s" A:/zeptoforth/zt/gone" 2dup mkdir exists? s" made a directory to remove" check
  s" A:/zeptoforth/zt/gone" rmdir
  s" A:/zeptoforth/zt/gone" exists? 0= s" rmdir: gone" check
  [: dir$ rmdir ;] ['] x-denied s" rmdir of a full one raises x-denied" raises
;
t-rmdir

: t-load
  a$ file-load { block size }
  size big 10 + s" file-load's size" eq
  block 100 + 5 s" HELLO" equal-strings? s" file-load has its bytes" check
  wbuf block 100 same? s" and from the start" check
  block release
  [: nofile$ file-load ;] ['] x-no-file s" file-load of nothing raises x-no-file" raises
;
t-load
files
ends
.s
