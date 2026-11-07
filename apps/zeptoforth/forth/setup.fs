\ setup.fs: the Forth library, in the order of src/stm32f746/forth/setup_full.fs, with the CMM2's own files for what is the firmware's.
\ The F746's clock.fs, erase.fs, gpio.fs, pin.fs, exti.fs, led.fs, uart.fs, adc.fs, spi.fs and qspi.fs have no place here: the firmware has the hardware, and cmm2.fs has the pins, buses
\ and serial ports. Nor have sdcard.fs and simple_fat32.fs: the firmware has the SD card, and cmm2_fs.fs its files. blocks.fs is block.fs on the card.
#include src/common/forth/basic.fs
#include src/common/forth/module.fs
#include src/common/forth/armv6m.fs
#include src/common/forth/fast_basic.fs
#include src/common/forth/value.fs
#include src/common/forth/interrupt.fs
#include src/common/forth/exception.fs
#include src/common/forth/multicore.fs
#include src/common/forth/lambda.fs
#include src/common/forth/fixed.fs
#include ../forth/float64.fs
#include ../forth/systick.fs
#include ../forth/serial.fs
#include src/common/forth/task.fs
#include src/common/forth/full_default.fs
#include src/common/forth/cycles.fs
#include src/common/forth/rtc.fs
#include ../forth/clock.fs
#include ../forth/files.fs
#include ../forth/cmm2.fs
#include ../forth/blocks.fs
#include src/common/forth/edit.fs
#include src/common/forth/block_dev.fs
#include src/common/forth/fat32.fs
#include ../forth/simple_fat32.fs
#include src/common/forth/fat32_tools.fs
#include ../forth/cmm2_fs.fs
compile-to-flash
#include extra/common/dyn_buffer.fs
#include extra/common/zeptoed.fs
#include ../forth/editor.fs
reboot
