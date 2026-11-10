/* app.ld: a loadable program for the CMM2 (tools/sdk). Linked as one block at __app_base (0xD1800000, where the
   G2 loads it, unless build.py --base says otherwise; a G1 moves it by its relocations) and loaded as one: code,
   constants, the init/fini arrays and data, then .bss, which the loader zeroes. mkimage.py makes the file from the
   ELF this gives with --emit-relocs.
   .axi (build.py --axi, a G2's programs only): the code, constants, data and .bss (as zeros) of the objects named, linked to run in the
   processor's 512 KB of AXI RAM at 0x24000000 but carried at the image's start, where cmm2_axi_load() (crt0.c) copies
   them from once the program's display is in the SDRAM (the firmware's modes clear that RAM); build.py writes the
   objects' patterns in cmm2axi.ld, empty without --axi, when .text starts at the image's start as before. */
ENTRY(_start)

SECTIONS
{
    . = DEFINED(__app_base) ? __app_base : 0xD1800000;
    __image_start = .;
    /* .itcm (build.py --itcm, a G2's programs only): the functions named, linked to run in the ITCM above the firmware's
       code (0xA000-0xFFFF: the V8 firmware's ends at 0x9718) and carried at the image's start, copied there by
       cmm2_axi_load() too. First, so that its sections are not taken by .axi's patterns for the same objects. A call
       between it and the AXI RAM or the SDRAM goes through a veneer the linker makes (more than 16 MB apart) */
    .itcm 0x0000A000 : AT(__image_start) {
        __itcm_start = .;
        INCLUDE cmm2itcm.ld
        . = ALIGN(8);
        __itcm_end = .;
    }
    __itcm_load = LOADADDR(.itcm);
    ASSERT(__itcm_end <= 0x10000, "the --itcm functions are more than the ITCM's 24 KB at 0xA000")
    .axi 0x24000000 : AT(__image_start + SIZEOF(.itcm)) {
        __axi_start = .;
        INCLUDE cmm2axi.ld
        . = ALIGN(8);
        __axi_end = .;
    }
    __axi_load = LOADADDR(.axi);
    . = __image_start + SIZEOF(.itcm) + SIZEOF(.axi);
    .text : AT(ADDR(.text)) {                   /* (loaded where it runs: the sections after it too) */
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
    .data : {
        . = ALIGN(4);
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN(4);
    }
    __image_end = .;
    .bss (NOLOAD) : {
        . = ALIGN(8);
        __bss_start__ = .;
        *(.bss .bss.* .gnu.linkonce.b.* COMMON)
        . = ALIGN(8);
        __bss_end__ = .;
    }
    end = .;
    /DISCARD/ : { *(.comment) *(.note*) }
}
