\ clock.fs: the date and time from the firmware's clock, where src/common/forth/rtc.fs, which has no clock to read, says 1970. It follows that file, so what is compiled after it, the file
\ system's dates among them, has this date-time@. Also the H743's random number generator, which the firmware has running, as src/stm32f746/forth/rng.fs gives the F746's. MIT licence, as
\ zeptoforth.

\ Compile to flash
compile-to-flash

continue-module rtc

  \ The date and time now
  : date-time@ { date-time -- }
    3 cmm2-call
    date-time date-time-second c!
    date-time date-time-minute c!
    date-time date-time-hour c!
    date-time date-time-day c!
    date-time date-time-month c!
    date-time date-time-year !
    0 date-time date-time-msec h!
    date-time update-dotw
  ;

end-module

begin-module rng

  begin-module rng-internal

    $48021804 constant RNG_SR
    $48021808 constant RNG_DR

  end-module> import

  \ A random number, from the hardware
  : random ( -- u )
    begin RNG_SR @ 1 and until
    RNG_DR @
  ;

end-module

\ Reboot
reboot
