' fonts89.bas - the PicoMite's fonts 8 (4x6) and 9 (8x10), built in since 22 Oct 2026, and font 1 after them
Font 8 : Print MM.FontWidth; MM.FontHeight
Text 0, 0, "Font 8 abc", , 8
Font 9 : Print MM.FontWidth; MM.FontHeight
Text 0, 20, "Font 9 abc", , 9
Font 1 : Print MM.FontWidth; MM.FontHeight
On Error Skip : Font 10 : Print MM.ErrMsg$
