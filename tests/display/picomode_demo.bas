' picomode_demo.bas - OPTION PICOMITE HDMI to look at (run it with OPTION PICOMITE HDMI set). Each screen is one of
' the PicoMite's modes at one of its resolutions: a white frame round MM.HRES x MM.VRES and a yellow diagonal, which
' should start at the monitor's top left corner, with black beyond the frame's right and bottom edges where the CMM2
' mode that shows it is larger. A key goes on to the next. RUN "picomode_demo", "800x600 1024x768" shows only the
' resolutions given; with none, all eight.
Dim rs$(7) = ("640x480", "720x400", "800x600", "848x480", "800x480", "1024x600", "1280x720", "1024x768")
Dim integer r, m, y, c(4) = (RGB(blue), RGB(0, 128, 0), RGB(red), RGB(magenta), RGB(0, 128, 128))
For r = 0 To 7
  If MM.CmdLine$ <> "" And Instr(MM.CmdLine$, rs$(r)) = 0 Then Continue For
  Execute "Resolution " + rs$(r)
  For m = 1 To 5
    If m = 4 And r > 1 Then Continue For
    Mode m
    CLS c(m - 1)
    Box 0, 0, MM.HRes, MM.VRes, 1, RGB(white)
    Line 0, 0, MM.HRes - 1, MM.VRes - 1, 1, RGB(yellow)
    y = MM.FontHeight
    Text 8, y, "MODE " + Str$(m) + " at " + rs$(r), "LT", , , RGB(white), c(m - 1)
    Text 8, y * 2, "screen " + Str$(MM.HRes) + "x" + Str$(MM.VRes), "LT", , , RGB(white), c(m - 1)
    Text 8, y * 3, "CMM2 mode " + Str$(MM.Info(MODE)), "LT", , , RGB(white), c(m - 1)
    Text 8, y * 4, "press a key", "LT", , , RGB(yellow), c(m - 1)
    Do : Loop Until Inkey$ <> ""
  Next
Next
Mode 1
Print "The end: this MODE 1 stays at the prompt until EDIT or FM."
