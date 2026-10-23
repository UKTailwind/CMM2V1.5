' picomode.bas - OPTION PICOMITE HDMI's MODE and RESOLUTION (PicoMode.c). Run with OPTION PICOMITE HDMI set:
'   python -I tools/goldens.py COM4 OUT --corpus tests/picomite --dir A:/p --nomodemap --pre "OPTION PICOMITE HDMI"
' and OPTION PICOMITE NONE after. A program starts in MODE 1; each MODE is the PicoMite's size on its resolution's
' CMM2 timing (MM.INFO(MODE)), MM.HRES and MM.VRES the PicoMite's; MODE clears, even the same mode; the 4-bit colours
' are HDMI's; font 7's lines are 8 apart; POKE DISPLAY VRES; RESOLUTION in a program; MM.DEVICE$ the name that
' OPTION PICOMITE HDMI stands for.
Option Explicit
Dim integer m, r, a
Dim rs$(7) = ("640x480", "720x400", "800x600", "848x480", "800x480", "1024x600", "1280x720", "1024x768")
Option Console Serial
Print "start:"; MM.Info(MODE); MM.HRes; MM.VRes; " "; MM.Info$(LCDPANEL)
Print "MM.DEVICE$: "; MM.Device$; ", MM.INFO$(DEVICE): "; MM.Info$(DEVICE)
For r = 0 To 7
  Execute "Resolution " + rs$(r)
  Print rs$(r); ":";
  For m = 1 To 5
    If m = 4 And r > 1 Then
      On Error Skip 1
      Mode 4
      Print " [4: "; MM.ErrMsg$; "]";
    Else
      Mode m
      Print " "; m; "="; MM.Info(MODE); MM.HRes; "x"; Str$(MM.VRes);
    EndIf
  Next
  Print
Next
Resolution 640
Mode 2
Print "LCDPANEL: "; MM.Info$(LCDPANEL); ", font"; MM.Info(FONT); MM.FontHeight
Pixel 0, 0, RGB(0, 128, 0) : Print "HDMI green 128: "; Hex$(Pixel(0, 0))
Mode 2
Print "MODE 2 again clears: "; Hex$(Pixel(0, 0))
a = MM.Info(WRITEBUFF)
Print "4-bit rows "; MM.HRes \ 2; " bytes: POKE "; : Poke Byte a + 160, &H21 : Print Hex$(Pixel(0, 1)); " "; Hex$(Pixel(1, 1))
Poke Display Vres 100
Print "POKE DISPLAY VRES 100:"; MM.VRes; : Pixel 5, 150, RGB(255, 255, 255) : Print " "; Hex$(Pixel(5, 99))
On Error Skip 1
Poke Display Hres 100
Print "POKE DISPLAY HRES: "; MM.ErrMsg$
Mode 2
Print "MODE puts it back:"; MM.VRes
Mode 1
Print "MODE 1:"; MM.Info(MODE); MM.Info(TILE HEIGHT)
On Error Skip 1
Mode 6
Print "MODE 6: "; MM.ErrMsg$
Mode 8, 8
Print "MODE 8,8 is the CMM2's:"; MM.Info(MODE); MM.HRes; MM.VRes; " "; MM.Info$(LCDPANEL)
Print "done"
