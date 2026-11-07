\ Copyright (c) 2013? Matthias Koch
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

\ serial.fs: the serial module's words on the CMM2. The console is the firmware's, through the kernel's serial-key and serial-emit
\ (port/console.s), so there is no interrupt-driven IO to set up. The same names as the STM32F746's serial.fs, for task.fs and
\ console.fs.

\ Compile to flash
compile-to-flash

begin-module serial

  internal import

  begin-module serial-internal

    : do-emit ( c -- ) serial-emit ;
    : do-key ( -- c ) serial-key ;
    : do-emit? ( -- flag ) serial-emit? ;
    : do-key? ( -- flag ) serial-key? ;
    : do-flush-console ( -- ) ;

  end-module> import

  \ Handle IO for multitasking
  : task-io ( -- ) ;

  \ Set up the serial console
  : serial-console ( -- )
    ['] do-key key-hook !
    ['] do-emit emit-hook !
    ['] do-emit error-emit-hook !
    ['] do-key? key?-hook !
    ['] do-emit? emit?-hook !
    ['] do-emit? error-emit?-hook !
    ['] do-flush-console flush-console-hook !
    ['] do-flush-console error-flush-console-hook !
  ;

  \ Interrupt-driven IO: the firmware's already
  : enable-serial-int-io ( -- ) serial-console ;
  : disable-serial-int-io ( -- ) ;
  : init-serial-int-io ( -- ) serial-console ;

  \ Old names for compatiblity's sake
  : enable-int-io ( -- ) enable-serial-int-io ;
  : disable-int-io ( -- ) disable-serial-int-io ;
  : init-int-io ( -- ) init-serial-int-io ;

end-module> import

\ Old name for compatibility's sake
serial constant int-io

\ Init
: init ( -- )
  init
  init-serial-int-io
;

\ Reboot
reboot
