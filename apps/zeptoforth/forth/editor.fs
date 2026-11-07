\ editor.fs: room for a whole program in zeptoed, whose 64 KB by default holds a file of less than that with its undo. MIT licence, as zeptoforth.

\ Compile to flash
compile-to-flash

: init-zeptoed-heap ( -- ) 524288 to zeptoed-heap-size ;

initializer init-zeptoed-heap

\ Reboot
reboot
