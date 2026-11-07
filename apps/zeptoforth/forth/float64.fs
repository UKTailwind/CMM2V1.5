\ float64.fs: double-precision floating point on the CMM2, in place of src/common/forth/float32.fs. The arithmetic and the maths
\ functions are in the kernel (port/float64.s); these are the rest of float32's words, its rounding rules and tests as they
\ are, for a value of two cells: ( lo hi ), the high word on top. float32's v is dv here, so v+ is dv+ and vsin is dvsin, and
\ a literal has a d before its exponent: 1.5de0, 2de3. float64_v.fs gives a program the names without the d.

\ Compile to flash
compile-to-flash

begin-module float64

  \ Raised for what has no value
  : x-domain-error ( -- ) ." domain error" cr ;

  begin-module float64-internal

    \ Saved handle number hook
    variable saved-handle-number-hook

    \ Does a number have a double's exponent, de or DE? ($DF takes a letter to its capital)
    : check-exponent ( addr bytes -- flag )
      1- over + swap ?do
        i c@ $DF and [char] D = i 1+ c@ $DF and [char] E = and if unloop true exit then
      loop
      false
    ;

    \ Handle numeric literals: an integer first, then a value
    : do-handle-number { addr bytes -- flag }
      addr bytes saved-handle-number-hook @ execute if
        true
      else
        addr bytes check-exponent if
          addr bytes parse-dv if
            state @ if
              swap lit, lit, true
            else
              true
            then
          else
            2drop false
          then
        else
          false
        then
      then
    ;

    \ Initialize
    : init-handle-number ( -- )
      handle-number-hook @ saved-handle-number-hook !
      ['] do-handle-number handle-number-hook !
    ;

    initializer init-handle-number

  end-module> import

  \ One and minus one
  1 n>dv 2constant dv1
  -1 n>dv 2constant dv-1

  \ A half, either way
  1 n>dv 2 n>dv dv/ 2constant dvhalf
  -1 n>dv 2 n>dv dv/ 2constant dv-half

  \ Two
  2 n>dv 2constant dv2

  \ Infinity
  0 $7FF0_0000 2constant dv+infinity
  0 $FFF0_0000 2constant dv-infinity

  \ Pi
  $5444_2D18 $4009_21FB 2constant dvpi

  \ Tests against zero
  : dv0< ( f -- flag ) 0 0 dv< ;
  : dv0<= ( f -- flag ) 0 0 dv<= ;
  : dv0= ( f -- flag ) 0 0 dv= ;
  : dv0<> ( f -- flag ) 0 0 dv<> ;
  : dv0> ( f -- flag ) 0 0 dv> ;
  : dv0>= ( f -- flag ) 0 0 dv>= ;

  \ Is a value not a number?
  : dv>nan? ( f -- flag ) 2dup dv<> ;

  \ Is a value infinite?
  : dv>infinite? ( f -- flag ) dvabs dv+infinity dv= ;

  \ Print a value
  : (dv.) ( f -- ) 40 [: dup >r format-dv r> swap type ;] with-allot ;

  \ Print a value with a space
  : dv. ( f -- ) (dv.) space ;

  \ Round a value to the nearest integer with half rounding up
  : dvround-half-up ( f -- f' )
    2dup dvnfract { D: int } dvfract { D: fract }
    int dv0>= if
      fract dvhalf dv>= if dv1 else 0 0 then
    else
      fract dv-half dv< if dv-1 else 0 0 then
    then
    int dv+
  ;

  \ Round a value to the nearest integer with half rounding down
  : dvround-half-down ( f -- f' )
    2dup dvnfract { D: int } dvfract { D: fract }
    int dv0>= if
      fract dvhalf dv> if dv1 else 0 0 then
    else
      fract dv-half dv<= if dv-1 else 0 0 then
    then
    int dv+
  ;

  \ Round a value to the nearest integer with half rounding towards zero
  : dvround-half-zero ( f -- f' )
    2dup dvnfract { D: int } dvfract { D: fract }
    int dv0>= if
      fract dvhalf dv> if dv1 else 0 0 then
    else
      fract dv-half dv< if dv-1 else 0 0 then
    then
    int dv+
  ;

  \ Round a value to the nearest integer with half rounding towards odd
  : dvround-half-odd ( f -- f' )
    2dup dvnfract dv2 dvmod dv0= if
      dvround-half-up
    else
      dvround-half-down
    then
  ;

  \ Round a value towards zero
  : dvround-zero ( f -- f' ) dvnfract ;

  \ Round a value away from zero
  : dvround-away-zero ( f -- f' )
    2dup dvnfract { D: int } dvfract { D: fract }
    int dv0>= if
      fract dv0<> if dv1 else 0 0 then
    else
      fract dv0<> if dv-1 else 0 0 then
    then
    int dv+
  ;

  \ Exponentiate a value by an integer
  : dvi** ( f exponent -- f' ) n>dv dvpow ;

  \ Exponentiate two values: 0^0 and a negative number to a fraction are domain errors, as they are in float32
  : dv** { D: fb D: fx -- fb^fx }
    fb dv0= fx dv0= and triggers x-domain-error
    fx dvfract dv0= not if fb dv0>= averts x-domain-error then
    fb fx dvpow
  ;

end-module> import

\ Reboot
reboot
