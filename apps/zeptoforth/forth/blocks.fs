\ blocks.fs: zeptoforth's blocks on the SD card, in place of src/common/forth/block.fs, which keeps them in the QSPI flash: the same words, for src/common/forth/edit.fs and for programs.
\ They are records in blocks.dat in the current directory, each a block's id and its 1024 bytes, with a table in RAM of what is where. A block found stays good for the next few
\ finds, as a memory-mapped one would for as long as it is not written. load, list and what they use are block.fs's. MIT licence, as zeptoforth.

\ Compile to flash
compile-to-flash

begin-module block

  internal import
  cmm2 import

  \ Block size
  1024 constant block-size

  \ Write failure exception
  : x-block-write-fail ( -- ) ." unable to write block" cr ;

  begin-module block-internal

    \ The most blocks there can be
    4096 constant max-blocks

    \ A record, and what a deleted one has for its id
    block-size cell+ constant record-size
    -1 constant no-block

    \ blocks.dat's handle, or -1 until it is wanted, and its records
    variable handle
    variable records
    max-blocks cells buffer: ids

    \ What has been found
    4 constant cache-count
    cache-count block-size * buffer: cache
    variable cache-next

    \ Open the file, if it is not, and see what is there
    : open-blocks ( -- )
      handle @ 0>= if exit then
      s" blocks.dat" 2dup exists? if o-read o-write or else o-read o-write or o-create or then file-open handle !
      handle @ file-size record-size / max-blocks min records !
      records @ 0 ?do
        i record-size * handle @ file-seek
        ids i cells + cell handle @ file-read drop
      loop
    ;

    \ Where a block is, or -1
    : index-of ( id -- index | -1 )
      open-blocks
      records @ 0 ?do
        dup ids i cells + @ = if drop i unloop exit then
      loop
      drop -1
    ;

    \ Where a new block can go, or -1
    : free-index ( -- index | -1 )
      no-block index-of dup 0>= if exit then
      drop records @ dup max-blocks = if drop -1 then
    ;

    \ A record's id written out
    : write-id ( index -- )
      dup record-size * handle @ file-seek
      ids swap cells + cell handle @ file-write drop
    ;

    \ The start
    : init-block ( -- )
      -1 handle !
      0 records !
      0 cache-next !
    ;

    initializer init-block

    \ Truncate a string to its first invalid character
    : truncate-invalid ( b-addr u -- b-addr u )
      swap tuck swap
      begin
        dup 0> if
          over c@
          dup $20 >= swap $7F <> and if
            1- swap 1+ swap false
          else
            true
          then
        else
          true
        then
      until
      drop over -
    ;

  end-module> import

  \ Invalid block id exception
  : x-invalid-block-id ( -- ) ." invalid block id" cr ;

  \ Block not found
  : x-block-not-found ( -- ) ." block not found" cr ;

  \ Get the number of free blocks
  : free-block-count@ ( -- count )
    open-blocks
    max-blocks records @ -
    records @ 0 ?do ids i cells + @ no-block = if 1+ then loop
  ;

  \ Does a block exist?
  : block? ( id -- flag ) index-of 0>= ;

  \ A block's contents, or 0
  : find-block ( id -- addr | 0 )
    index-of dup 0< if drop 0 exit then
    cache cache-next @ block-size * + { index addr }
    cache-next @ 1+ cache-count umod cache-next !
    index record-size * cell + handle @ file-seek
    addr block-size handle @ file-read drop
    addr
  ;

  \ Write a block
  : block! { addr id -- success }
    id no-block = triggers x-invalid-block-id
    id index-of dup 0< if drop free-index then { index }
    index 0< if false exit then
    id ids index cells + !
    index records @ = if 1 records +! then
    index write-id
    addr block-size handle @ file-write drop
    handle @ file-sync
    true
  ;

  \ Delete a block
  : delete-block ( id -- )
    index-of dup 0>= if
      no-block ids 2 pick cells + !
      write-id
      handle @ file-sync
    else
      drop
    then
  ;

  \ Delete a range of blocks
  : delete-blocks ( start-id count -- ) over + swap ?do i delete-block loop ;

  \ Delete all the blocks
  : erase-all-blocks ( -- )
    open-blocks
    handle @ file-close
    -1 handle !
    s" blocks.dat" file-remove
  ;

  \ Copy a single block
  : copy-block ( src-id dest-id -- )
    over block? if
      over find-block over block! averts x-block-write-fail
    else
      dup block? if dup delete-block then
    then
    2drop
  ;

  \ Copy blocks
  : copy-blocks { src-id dest-id count -- }
    src-id dest-id < if
      count 0 ?do src-id count + 1- i - dest-id count + 1- i - copy-block loop
    else
      count 0 ?do src-id i + dest-id i + copy-block loop
    then
  ;

  \ Insert blocks, pushing extant blocks to higher indices as far as the first gap that takes them
  : insert-blocks { start-id count -- }
    start-id { gap } 0 { empty }
    begin empty count < while
      gap block? if 0 to empty else empty 1+ to empty then
      gap 1+ to gap
    repeat
    gap count - { end-id }
    end-id start-id - 0 ?do
      end-id 1- i - dup count + copy-block
    loop
    start-id count delete-blocks
  ;

  continue-module block-internal

    \ Get the current block being interpreted
    : current-block ( -- addr ) eval-data @ @ find-block ;

    \ Get the current line offset being interpreted
    : current-line ( -- u ) eval-data @ cell+ @ ;

    \ Advance the line being interpreted
    : advance-line ( -- ) 64 eval-data @ cell+ +! ;

    \ Refill from a block
    : block-refill ( -- )
      advance-line
      current-block current-line + 64 truncate-invalid feed-input
    ;

    \ Get whether the current line is the end of a block
    : block-eof ( -- eof? )
      current-line [ 1024 64 - ] literal >=
    ;

  end-module

  \ Load a block
  : load ( id -- )
    dup block? averts x-block-not-found
    [:
      0 >r >r rp@
      dup @ find-block 64 truncate-invalid ['] block-refill ['] block-eof
      evaluate-with-input
      rdrop rdrop
    ;] try
    ?raise
  ;

  \ List a block
  : list ( id -- )
    dup block? averts x-block-not-found
    find-block
    16 0 ?do
      cr i 9 < if space then i 1+ . dup i 64 * + 64 truncate-invalid type
    loop
    cr drop
  ;

  \ Load a range of blocks, ignoring nonexistent blocks
  : load-range ( start-id end-id -- )
    1+ swap ?do
      i find-block dup if
        dup block-size + swap ?do
          i 64 truncate-invalid evaluate
        64 +loop
      else
        drop
      then
    loop
  ;

  \ List a range of blocks, ignoring nonexistent blocks
  : list-range ( start-id end-id -- )
    1+ swap ?do
      i find-block dup if
        dup block-size + swap ?do
          cr i 64 truncate-invalid type
        64 +loop
      else
        drop
      then
    loop
    cr
  ;

end-module> import

\ Load a block
: load ( id -- ) load ;

\ List a block
: list ( id -- ) list ;

\ Load a range of blocks, ignoring nonexistent blocks
: load-range ( start-id end-id -- ) load-range ;

\ List a range of blocks, ignoring nonexistent blocks
: list-range ( start-id end-id -- ) list-range ;

\ Reboot
reboot
