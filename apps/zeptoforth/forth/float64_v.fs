\ float64_v.fs: the double words under the names without the d, v+ for dv+ and vsin for dvsin, and 1.5e0 read as a double as 1.5de0 is, for a program written with them.
\ include float64_v.fs, then float64-v import. It is in RAM, so load it again after a reboot. There is no float32 here for them to clash with. MIT licence, as zeptoforth.

begin-module float64-v

  float64 import

  \ The arithmetic and comparisons
  : v+ dv+ ;  : v- dv- ;  : v* dv* ;  : v/ dv/ ;  : vmin dvmin ;  : vmax dvmax ;  : vmod dvmod ;
  : vabs dvabs ;  : vnegate dvnegate ;  : vsqrt dvsqrt ;  : vfract dvfract ;  : vnfract dvnfract ;
  : vceil dvceil ;  : vfloor dvfloor ;
  : v< dv< ;  : v<= dv<= ;  : v= dv= ;  : v<> dv<> ;  : v> dv> ;  : v>= dv>= ;
  : v0< dv0< ;  : v0<= dv0<= ;  : v0= dv0= ;  : v0<> dv0<> ;  : v0> dv0> ;  : v0>= dv0>= ;
  : v>nan? dv>nan? ;  : v>infinite? dv>infinite? ;

  \ Rounding
  : vround-half-even dvround-half-even ;  : vround-half-away-zero dvround-half-away-zero ;
  : vround-half-up dvround-half-up ;  : vround-half-down dvround-half-down ;
  : vround-half-zero dvround-half-zero ;  : vround-half-odd dvround-half-odd ;
  : vround-zero dvround-zero ;  : vround-away-zero dvround-away-zero ;

  \ The maths functions
  : vsin dvsin ;  : vcos dvcos ;  : vtan dvtan ;  : vasin dvasin ;  : vacos dvacos ;  : vatan dvatan ;  : vatan2 dvatan2 ;
  : vsinh dvsinh ;  : vcosh dvcosh ;  : vtanh dvtanh ;  : vasinh dvasinh ;  : vacosh dvacosh ;  : vatanh dvatanh ;
  : vexp dvexp ;  : vexpm1 dvexpm1 ;  : vln dvln ;  : vlnp1 dvlnp1 ;  : vpow dvpow ;  : vi** dvi** ;  : v** dv** ;

  \ Conversions and text. f32>v and v>f32 are the v>dv and dv>v of a value in float32's form
  : n>v n>dv ;  : u>v u>dv ;  : v>n dv>n ;  : v>u dv>u ;  : f32>v v>dv ;  : v>f32 dv>v ;  : f64>v f64>dv ;  : v>f64 dv>f64 ;
  : format-v format-dv ;  : parse-v parse-dv ;  : (v.) (dv.) ;  : v. dv. ;

  \ The constants
  dv1 2constant v1
  dv-1 2constant v-1
  dv2 2constant v2
  dvhalf 2constant vhalf
  dv-half 2constant v-half
  dvpi 2constant vpi
  dv+infinity 2constant +infinity
  dv-infinity 2constant -infinity

  \ The hook already there
  variable saved-hook

  \ Does a number have an exponent?
  : exponent? ( addr bytes -- flag )
    over + swap ?do
      i c@ $DF and [char] E = if unloop true exit then
    loop
    false
  ;

  \ Handle literals: what the hook knows, then a value with an exponent
  : do-handle-number { addr bytes -- flag }
    addr bytes saved-hook @ execute if
      true
    else
      addr bytes exponent? if
        addr bytes parse-dv if
          state @ if swap lit, lit, then true
        else
          2drop false
        then
      else
        false
      then
    then
  ;

  handle-number-hook @ saved-hook !
  ' do-handle-number handle-number-hook !

end-module
