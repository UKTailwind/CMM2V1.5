\ simple_fat32.fs: what src/common/forth/fat32_tools.fs wants of src/common/forth/simple_fat32.fs, so that it loads as it is. That file is a FAT32 file system on an SD card on SPI, which
\ here is the firmware's: cmm2_fs.fs is the file system, and init-simple-fat32 is not for use. MIT licence, as zeptoforth.

\ Compile to flash
compile-to-flash

begin-module simple-fat32

  oo import
  fat32 import

  <base-fat32-fs> begin-class <simple-fat32-fs>
    method write-through! ( write-through fs -- )
  end-class

  <simple-fat32-fs> begin-implement
    ' abstract-method define write-through!
  end-implement

end-module

\ Reboot
reboot
