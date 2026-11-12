# Appendix O – Companion Manuals

Some features are large enough to have a manual of their own rather than an entry in the listings. Some of these were written for the Colour Maximite 2 and are included in its firmware distribution zip file; others were written for the PicoMite and describe features that the Colour Maximite 2 shares with it. Appendix N says which keyword is described where.

{widths: 30,45,25}
| Manual | Covers | Notes |
|---|---|---|
| Colour_Maximite_2_App_User_Guide.pdf | Using the programs that come with the firmware: installing, starting, using and leaving MicroPython, Doom, Heretic, Quake, ScummVM, the NES, ZX Spectrum, Amstrad CPC, Commodore 64, Atari 8-bit, Apple //e, Amiga 500, TRS-80 and BBC Micro emulators, the 386 PC, the C compiler, the VT100 terminal, zeptoforth and WordGrinder. | The Colour Maximite 2's own. |
| Colour_Maximite_2_Loadable_Programs.pdf | Writing loadable programs in C and C++ (RUN "name.app"): the tools that build them, how they are loaded and run, the services that the firmware gives them (the console, files, screen, keyboard, mouse, sound, the FM synthesiser and the I/O pins) and porting existing software. | The Colour Maximite 2's own, in the firmware's distribution. |
| Colour_Maximite_2_C_Compiler.pdf | Compiling C and MMBasic programs on the Colour Maximite 2 with the CC command: the language and library, what is and is not translated from MMBasic, and the speed to expect. | The Colour Maximite 2's own. |
| Colour_Maximite_2_386_PC_Emulator.pdf | Using pc386, the 386 PC emulator (RUN "pc386.app"): its files and keys, the configuration file of each PC, disk images, moving a PC to another Colour Maximite 2, and installing MS-DOS and Windows 3.1. | The Colour Maximite 2's own, in the firmware's distribution. |
| GUI Controls and Programming.pdf | The GUI controls - buttons, switches, LEDs, check boxes, spin boxes, text and number boxes, gauges and the date, time and position controls - together with CTRLVAL() and MSGBOX(). | The Colour Maximite 2's own. |
| The 3D engine.pdf | The DRAW3D command and function: cameras, objects, rotation and hidden line removal. | The Colour Maximite 2's own. |
| Graphics Programming on the CMM2 | Pages, layers, sprites and the other advanced graphics features, written for games developers (also on the Back Shed forum). | The Colour Maximite 2's own. |
| MMBasic_Structures_Manual.pdf | User defined types: TYPE, END TYPE, STRUCT and the STRUCT() function. | The PicoMite's; the same on the Colour Maximite 2. |
| FM_User_Manual.pdf | The two panel file manager opened with the FM command. | The PicoMite's. The chapter File Manager says what differs here (the drives, and no mouse yet). |
| TILEMAP_User_Manual.pdf | Building scrolling backgrounds with the TILEMAP command and function. | The PicoMite's; the same on the Colour Maximite 2. |
| PLAY_SAMPLE_User_Manual.pdf | Playing sampled sound effects with PLAY SAMPLE. | The PicoMite's; the same on the Colour Maximite 2. |
| USB_CDC_Host_User_Manual.pdf | Using USB serial devices. | The PicoMite's: its COM3 to COM6 are COM4 to COM7 here. |
| mmb2csub.pdf | The mmb2csub tool, which turns a BASIC SUB or FUNCTION into a CSUB without writing any C, so that the busiest routines of a program run as machine code. | Use --target cmm2 to build CSUBs for the Colour Maximite 2. |
