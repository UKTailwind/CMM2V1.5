# Appendix B – The Program File

A program file (.APP) is made by mkimage.py and read by the firmware's loader (Src/Loader.c). It has three parts: a 64-byte header, the image, and the relocations. Every number is little-endian.

## The Header

{widths: 12,12,20,56}
| Offset | Size | Field | Meaning |
|---|---|---|---|
| 0 | 8 | magic | "CMM2IMG" and a zero byte. |
| 8 | 2 | hdr_version | The header's version: 1. |
| 10 | 2 | api_version | The version of the services the program needs (mkimage's --api). |
| 12 | 1 | kind | 1: an application, which runs and returns to MMBasic. |
| 13 | 1 | flags | Bit 0: the program needs a Generation 2 (--g2). Bit 1: a Generation 1 lends it the display memory (--vram). |
| 14 | 2 | reserved | 0. |
| 16 | 4 | link_base | The address the program was linked at, a multiple of 64KB (0xD1800000). |
| 20 | 4 | image_size | The bytes of the image, which follow the header. |
| 24 | 4 | bss_size | The bytes of .bss, cleared to zero after the image. |
| 28 | 4 | entry | The entry point's offset in the image, plus 1 (a Thumb address). |
| 32 | 4 | stack_size | The stack's size, or 0 for 64KB. |
| 36 | 4 | heap_min | The least heap the program can run with. |
| 40 | 4 | reloc_count | The number of relocation words after the image. |
| 44 | 4 | crc32 | The CRC-32 (zlib's) of the image and the relocation words. |
| 48 | 16 | name | The program's name, padded with zero bytes. |

## The Image

image_size bytes: the program as linked, from link_base: its code, constants, the tables of constructors and destructors, and its initialised data, with any gaps between them filled with zeros. The loader reads it to its place, then clears the .bss after it.

## The Relocations

reloc_count 32-bit words follow the image. Each is the offset in the image of a place that holds an absolute address of the program, in its bottom 28 bits, with its kind in the top 4:

{widths: 12,88}
| Kind | Meaning |
|---|---|
| 0 | A 32-bit word holding an address: the difference is added to it. |
| 1 | A Thumb-2 MOVT instruction, which loads the top half of an address: the top half of the difference is added to its 16-bit immediate. |

The loader only moves a program by a multiple of 64KB, so the bottom half of an address (loaded by a MOVW) never changes and has no relocation. When the program is loaded at link_base (a Generation 2) the relocations are not used. On a Generation 1 the loader reads them into the memory just after the .bss, before the program's heap is set up there, so they need no memory of their own.

The loader checks the CRC before moving anything, and refuses a file whose relocation points outside the image or has an unknown kind ("The program file is damaged").
