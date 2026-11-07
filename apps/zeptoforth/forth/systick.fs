\ Copyright (c) 2020-2024 Travis Bemann
\
\ Permission is hereby granted, free of charge, to any person obtaining a copy
\ of this software and associated documentation files (the "Software"), to deal
\ in the Software without restriction, including without limitation the rights
\ to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
\ copies of the Software, and to permit persons to whom the Software is
\ furnished to do so, subject to the following conditions:
\ 
\ The above copyright notice and this permission notice shall be included in
\ all copies or substantial portions of the Software.
\ 
\ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
\ IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
\ FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
\ AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
\ LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
\ OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
\ SOFTWARE.

\ systick.fs: the CMM2's time. SysTick is the firmware's and is left alone: the count is worked out from the firmware's
\ microseconds when it is asked for, in the 100 us ticks that zeptoforth counts in, and nothing is interrupted to keep it.
\ The tasker's tick comes from the firmware too (port/tasks.s). The same words as src/common/forth/systick.fs.

\ Compile to flash
compile-to-flash

begin-module systick

  internal import

  begin-module systick-internal

    \ SysTick counter override hook
    variable systick-override-hook

  end-module> import

  \ SysTick vector index
  15 constant systick-vector

  \ SysTick handler: nothing calls it here
  : systick-handler ( -- ) ;

  commit-flash

  \ Set systick override hook; 0 disables overriding systick
  : systick-override-hook! ( xt -- ) \ xt: ( -- ticks )
    systick-override-hook !
  ;

  \ Get systick override hook; 0 means no systick override
  : systick-override-hook@ ( -- xt ) \ xt: ( -- ticks )
    systick-override-hook @
  ;

  \ Turn on systick: it is always on
  : enable-systick ( -- ) ;

  \ Turn off systick: it is the firmware's to keep on
  : disable-systick ( -- ) ;

  \ Initialize SysTick
  : init-systick ( -- ) 0 systick-override-hook ! ;

  \ Initialize SysTick for an auxiliary core
  : init-systick-aux-core ( -- ) ;

  \ Systick divisor: ticks in a millisecond
  : systick-divisor ( -- u ) 10 ;

  \ Make systick-counter read-only
  : systick-counter ( -- u )
    systick-override-hook @ ?dup if execute else cmm2-ticks then
  ;

end-module> import

internal import

commit-flash

\ Init
: init ( -- )
  init
  init-systick
;

\ Wait for n milliseconds
: ms ( u -- )
  systick-divisor * systick-counter
  begin
    dup systick-counter swap - 2 pick u<
  while
    pause
  repeat
  drop drop
;

\ Reboot
reboot
