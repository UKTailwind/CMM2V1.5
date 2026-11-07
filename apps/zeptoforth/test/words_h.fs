cmm2 import
variable fails 0 fails !
variable checks 0 checks !
: check ( flag addr bytes -- ) 1 checks +! rot if ." ok   " else ." FAIL " 1 fails +! then type cr ;
: ends ( -- ) ." CHECKS " checks @ . ." FAILS " fails @ . cr ;
: eq ( a b addr bytes -- ) 2>r 2dup = if 2drop 2r> true -rot check else ." got " swap . ." want " . 2r> false -rot check then ;
: raises ( xt xc addr bytes -- ) 2>r swap try = 2r> check ;
