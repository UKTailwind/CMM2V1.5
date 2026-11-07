\ cmm2_fs.fs: the firmware's files as zeptoforth's file system, for zeptoed and fat32-tools. They work through a <fat32-file> and a <fat32-dir> in space they allot by those classes'
\ sizes, so the classes here add no fields, and keep what they need in fields of the kind they would have had. The firmware finds a path itself, so a <cmm2-dir> only holds where it
\ is, and a name goes to the firmware with that in front. MMBasic's names apply: A:/dir/name, or relative to the current directory. MIT licence, as zeptoforth.

\ Compile to flash
compile-to-flash

begin-module cmm2-fs

  oo import
  rtc import
  cmm2 import
  fat32 import

  begin-module cmm2-fs-internal

    \ What a file keeps: the firmware's handle
    : file-handle ( file -- addr ) fat32-internal::file-start-cluster ;

    \ And a directory: its path, a slot of those below or 0 for the current directory, and the firmware's handle while it is being read
    : dir-path ( dir -- addr ) fat32-internal::dir-start-cluster ;
    : dir-handle ( dir -- addr ) fat32-internal::dir-offset ;

    \ The paths of the directories that are open, each a length then its text, 0 if free
    16 constant path-count
    260 constant path-size
    path-count path-size * buffer: paths

    \ A name with a directory in front, and another for the other name of a rename
    260 buffer: joined
    260 buffer: joined2

    \ A slot for a path
    : new-path { addr bytes -- slot }
      path-count 0 ?do
        paths i path-size * + { slot }
        slot @ 0= if
          bytes slot !
          addr slot cell+ bytes move
          slot unloop exit
        then
      loop
      ['] x-too-many-files ?raise
    ;

    \ Give a slot back
    : free-path ( slot -- ) ?dup if 0 swap ! then ;

    \ A slot's text
    : path$ ( slot -- addr bytes ) dup cell+ swap @ ;

    \ Is a name complete in itself?
    : absolute? { addr bytes -- flag }
      bytes 0= if false exit then
      addr c@ [char] / = if true exit then
      bytes 1 > if addr 1+ c@ [char] : = else false then
    ;

    \ A name as the firmware wants it
    : full-name { addr bytes dir out -- addr' bytes' }
      dir dir-path @ 0= addr bytes absolute? or if addr bytes exit then
      dir dir-path @ path$ { path path-bytes }
      path out path-bytes move
      [char] / out path-bytes + c!
      addr out path-bytes 1+ + bytes move
      out path-bytes 1+ bytes +
    ;

    \ The letter's capital
    : capital ( c -- c' ) dup [char] a >= over [char] z <= and if $20 - then ;

    \ A name in an entry's 8.3, cut down to fit: the firmware's may be longer
    : short-name { addr bytes entry -- }
      entry short-file-name 8 $20 fill
      entry short-file-ext 3 $20 fill
      bytes { dot }
      bytes 0 ?do addr i + c@ [char] . = if i to dot then loop
      dot 8 min 0 ?do addr i + c@ capital entry short-file-name i + c! loop
      dot bytes < if
        bytes dot 1+ - 3 min 0 ?do addr dot 1+ i + + c@ capital entry short-file-ext i + c! loop
      then
    ;

  end-module> import

  \ A file
  <fat32-file> begin-class <cmm2-file>
  end-class

  <cmm2-file> begin-implement

    :noname ( fs file -- )
      dup <object>->new
      tuck fat32-internal::file-fs !
      false over fat32-internal::file-open !
      -1 swap file-handle !
    ; define new

    :noname ( file -- )
      dup file-open? if dup close-file then
      <object>->destroy
    ; define destroy

    :noname { file -- }
      file file-handle @ cmm2::file-close
      false file fat32-internal::file-open !
    ; define close-file

    :noname ( addr bytes file -- count ) file-handle @ cmm2::file-read ; define read-file

    :noname ( addr bytes file -- count ) file-handle @ cmm2::file-write ; define write-file

    :noname { file -- } file file-handle @ cmm2::file-truncate file file-handle ! ; define truncate-file

    :noname { offset whence file -- }
      whence case
        seek-set of offset endof
        seek-cur of file file-handle @ cmm2::file-tell offset + endof
        seek-end of file file-handle @ cmm2::file-size offset + endof
      endcase
      file file-handle @ cmm2::file-seek
    ; define seek-file

    :noname ( file -- offset ) file-handle @ cmm2::file-tell ; define tell-file

    :noname ( file -- bytes ) file-handle @ cmm2::file-size ; define file-size@

  end-implement

  \ A directory
  <fat32-dir> begin-class <cmm2-dir>
  end-class

  continue-module cmm2-fs-internal

    \ A file opened in a directory: the firmware's handle kept in a new object
    : open-as { addr bytes file dir mode -- }
      dir fat32-internal::dir-fs @ <cmm2-file> file init-object
      addr bytes dir joined full-name mode cmm2::file-open file file-handle !
      true file fat32-internal::file-open !
    ;

    \ A directory at a path
    : dir-at { addr bytes new-dir dir -- }
      dir fat32-internal::dir-fs @ <cmm2-dir> new-dir init-object
      bytes if addr bytes dir joined full-name new-path new-dir dir-path ! then
    ;

    \ Is a date one? A file may have none worth the name
    : good-date? ( date-time -- flag ) ['] update-dotw try if drop false else true then ;

    \ A name's date in an entry, which the firmware gives by name
    : entry-dates ( addr bytes dir entry -- )
      date-time-size [: { addr bytes dir entry date-time }
        addr bytes dir joined full-name cmm2::file-stat
        date-time date-time-second c!
        date-time date-time-minute c!
        date-time date-time-hour c!
        date-time date-time-day c!
        date-time date-time-month c!
        date-time date-time-year !
        2drop
        0 date-time date-time-msec h!
        date-time good-date? if
          date-time entry create-date-time!
          date-time entry modify-date-time!
        then
      ;] with-aligned-allot
    ;

    \ What the firmware knows of a name: 0 nothing, 1 a file, 2 a directory
    : kind { addr bytes dir -- kind }
      addr bytes dir joined full-name 2dup cmm2::exists? if
        cmm2::file-stat 2drop 2drop 2drop swap drop if 2 else 1 then
      else
        2drop 0
      then
    ;

  end-module

  <cmm2-dir> begin-implement

    :noname ( fs dir -- )
      dup <object>->new
      tuck fat32-internal::dir-fs !
      true over fat32-internal::dir-open !
      0 over dir-path !
      -1 swap dir-handle !
    ; define new

    :noname ( dir -- )
      dup dir-open? if dup close-dir then
      <object>->destroy
    ; define destroy

    :noname { new-dir dir -- }
      dir fat32-internal::dir-fs @ <cmm2-dir> new-dir init-object
      dir dir-path @ ?dup if path$ new-path new-dir dir-path ! then
    ; define clone-dir

    :noname { dir -- }
      dir dir-handle @ 0>= if dir dir-handle @ cmm2::dir-close -1 dir dir-handle ! then
      dir dir-path @ free-path
      0 dir dir-path !
      false dir fat32-internal::dir-open !
    ; define close-dir

    :noname ( addr bytes xt dir -- ) swap execute ; define with-path

    :noname { addr bytes dir -- flag } addr bytes dir kind 0<> ; define path-exists?

    :noname { entry dir -- entry-read? }
      dir dir-handle @ 0< if
        dir dir-path @ ?dup if path$ else cmm2::cwd$ then cmm2::dir-open dir dir-handle !
      then
      <fat32-entry> entry init-object
      dir dir-handle @ cmm2::dir-read if
        { addr bytes size dir? }
        entry init-blank-entry
        addr bytes entry short-name
        size entry entry-file-size !
        dir? if $10 else 0 then entry file-attributes c!
        addr bytes dir entry entry-dates
        true
      else
        false
      then
    ; define read-dir

    :noname ( addr bytes new-file dir -- ) [ cmm2::o-read cmm2::o-write or cmm2::o-create or ] literal open-as ; define create-file

    :noname ( addr bytes file dir -- ) [ cmm2::o-read cmm2::o-write or ] literal open-as ; define open-file

    :noname ( addr bytes dir -- ) joined full-name cmm2::file-remove ; define remove-file

    :noname { addr bytes new-dir dir -- }
      addr bytes dir joined full-name cmm2::mkdir
      addr bytes new-dir dir dir-at
    ; define create-dir

    :noname ( addr bytes opened-dir dir -- ) dir-at ; define open-dir

    :noname ( addr bytes dir -- ) joined full-name cmm2::file-remove ; define remove-dir

    :noname { new-addr new-bytes addr bytes dir -- }
      addr bytes dir joined full-name
      new-addr new-bytes dir joined2 full-name
      cmm2::file-rename
    ; define rename

    :noname ( dir -- empty? )
      <fat32-entry> class-size [: swap read-dir not ;] with-aligned-allot
    ; define dir-empty?

    :noname ( addr bytes dir -- exists? ) kind 0<> ; define exists?

    :noname ( addr bytes dir -- file? ) kind 1 = ; define file?

    :noname ( addr bytes dir -- dir? ) kind 2 = ; define dir?

  end-implement

  \ The file system
  <base-fat32-fs> begin-class <cmm2-fs>
  end-class

  continue-module cmm2-fs-internal

    \ A directory for as long as xt runs ( ... xt fs -- ... ) ( xt: ... dir -- ... )
    : with-dir ( xt fs -- )
      <fat32-dir> class-size [: { xt fs dir }
        dir fs root-dir@
        dir xt try
        dir close-dir
        dir destroy
        ?raise
      ;] with-aligned-allot
    ;

  end-module

  <cmm2-fs> begin-implement

    :noname ( dir fs -- ) swap <cmm2-dir> swap init-object ; define root-dir@
    :noname ( dir fs -- ) root-dir@ ; define current-dir@
    :noname ( addr bytes xt fs -- ) with-dir ; define with-root-path
    :noname ( addr bytes fs -- exists? ) drop cmm2::exists? ; define root-path-exists?
    :noname ( addr bytes xt fs -- ) ['] with-create-file-at-path swap with-dir ; define with-create-file-at-root-path
    :noname ( addr bytes xt fs -- ) ['] with-open-file-at-path swap with-dir ; define with-open-file-at-root-path
    :noname ( addr bytes xt fs -- ) ['] with-create-dir-at-path swap with-dir ; define with-create-dir-at-root-path
    :noname ( addr bytes xt fs -- ) ['] with-open-dir-at-path swap with-dir ; define with-open-dir-at-root-path
    :noname ( fs -- ) drop 96 cmm2-call ; define flush
    :noname ( fs -- fs' ) ; define real-fs@
    :noname ( fs -- open? ) drop true ; define fs-open?

  end-implement

  continue-module cmm2-fs-internal

    <cmm2-fs> class-size aligned-buffer: the-fs

    \ At the start: nothing open, and this the file system of fat32-tools and zeptoed
    : init-cmm2-fs ( -- )
      paths path-count path-size * 0 fill
      <cmm2-fs> the-fs init-object
      the-fs fat32-tools::current-fs!
    ;

    initializer init-cmm2-fs

  end-module

end-module

\ Reboot
reboot
