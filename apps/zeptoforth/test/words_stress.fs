\ tasks hammering the services under preemption: sentinels under multi-cell answers show a stack disturbed; each file task writes and reads back its own file
task import
0 console-screen
variable errs
variable going
variable n1
variable n2
variable n3
variable n4
\ the answers themselves are checked too: the tick runs Forth on this stack, and an answer below a stale dp is what it would overwrite
datetime 2drop drop constant d0 constant m0 constant y0
variable bad-answers
: hammer1
  begin going @ while
    1111 2222 datetime
    60 u< 0= if 1 bad-answers +! then 60 u< 0= if 1 bad-answers +! then 24 u< 0= if 1 bad-answers +! then
    d0 <> if 1 bad-answers +! then m0 <> if 1 bad-answers +! then y0 <> if 1 bad-answers +! then
    2222 <> if 1 errs +! then 1111 <> if 1 errs +! then
    1 n1 +!
  repeat
;
: hammer2
  begin going @ while
    3333 mouse 2drop 2drop drop 3333 <> if 1 errs +! then
    cwd$ s" A:/zeptoforth" equal-strings? 0= if 1 errs +! then
    4444 s" A:/zeptoforth/zt/a.bin" file-stat 2drop 2drop 2drop drop 10010 <> if 1 errs +! then
    4444 <> if 1 errs +! then
    1 n2 +!
  repeat
;
create b1 512 allot
create r1 512 allot
create b2 512 allot
create r2 512 allot
: same? ( a b n -- flag ) 0 ?do over i + c@ over i + c@ <> if 2drop false unloop exit then loop 2drop true ;
: filer { a u wb rb cnt }
  begin going @ while
    wb 512 cnt @ fill
    a u o-write o-create or file-open { h }
    wb 512 h file-write 512 <> if 1 errs +! then
    h file-close
    a u o-read file-open to h
    rb 512 h file-read 512 <> if 1 errs +! then
    h file-close
    wb rb 512 same? 0= if 1 errs +! then
    1 cnt +!
  repeat
;
: spawn-all
  0 ['] hammer1 320 256 1024 spawn run
  0 ['] hammer2 320 256 1024 spawn run
  s" A:/zeptoforth/zt/f1.bin" b1 r1 n3 5 ['] filer 320 256 1024 spawn run
  s" A:/zeptoforth/zt/f2.bin" b2 r2 n4 5 ['] filer 320 256 1024 spawn run
;
: t-stress
  0 errs ! 0 bad-answers ! 0 n1 ! 0 n2 ! 0 n3 ! 0 n4 ! true going !
  spawn-all
  micros drop { t0 } 0 { draws }
  begin micros drop t0 - 5000000 < while
    draws 800 mod draws 600 mod red pixel
    draws 800 mod 1+ draws 600 mod pixel@ drop
    draws 1+ to draws
  repeat
  false going !
  500 ms
  1 console-screen
  ." datetime " n1 @ . ." stat " n2 @ . ." files " n3 @ . n4 @ . ." draws " draws . ." errors " errs @ . ." bad answers " bad-answers @ . cr
  errs @ 0 s" no stack or file disturbed" eq
  bad-answers @ 0 s" no answer disturbed" eq
  n1 @ 1000 > s" the datetime task ran" check
  n2 @ 100 > s" the stat task ran" check
  n3 @ 5 > s" one file task ran" check
  n4 @ 5 > s" the other file task ran" check
  draws 1000 > s" the main task ran" check
;
t-stress
ends
.s
