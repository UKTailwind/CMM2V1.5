/* xip.ld: a runtime the firmware carries in its own flash and runs where it is (tools/sdk, build.py --xip; cmm2img.h
   CMM2XIP_*): crt0.c's cmm2xip_header first, then the code, the constants and .data's values, at __xip_base (build.py
   --xip-base; mmbrun's 0x08141000, CMM2XIP_FLASH, by default), and .data and .bss at 0x30004000 (CMM2XIP_RAM), where the
   loader copies and zeroes them. Nothing is moved, so there are no relocations. build.py makes OUT.bin, the flash's
   bytes, from the ELF this gives. */
ENTRY(_start)

SECTIONS
{
    . = DEFINED(__xip_base) ? __xip_base : 0x08141000;
    __xip_start = .;
    .text : {
        KEEP(*(.xip.header))                    /* crt0.c's cmm2xip_header */
        KEEP(*(.text.entry))                    /* crt0.c's _start */
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7 .glue_7t .vfp11_veneer .v4_bx)
    }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) }
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    }
    .preinit_array : {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    }
    .init_array : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    }
    .fini_array : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }
    . = ALIGN(4);
    __data_load__ = .;
    .data 0x30004000 : AT(__data_load__) {
        __data_start__ = .;
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_end__ = .;
    }
    __xip_end = __data_load__ + SIZEOF(.data);
    .bss (NOLOAD) : {
        . = ALIGN(8);
        __bss_start__ = .;
        *(.bss .bss.* .gnu.linkonce.b.* COMMON)
        . = ALIGN(8);
        __bss_end__ = .;
    }
    end = .;
    ASSERT(__bss_end__ <= 0x30008000, "the runtime's .data and .bss are more than CMM2XIP_RAM's 16 KB")
    /DISCARD/ : { *(.comment) *(.note*) }
}
