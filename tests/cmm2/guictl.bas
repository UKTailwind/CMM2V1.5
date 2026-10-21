' GUI LISTBOX, GUI SLIDER and GUI CLICK, the PicoMite's, driven by GUI CLICK (it clicks without a mouse): CTRLVAL,
' choosing from a list and scrolling it, a slider set and dragged, the GUI interrupts, and the errors. Needs OPTION
' MAXCTRLS (20 or more) and a mouse for the drag (MOUSE SET moves it while the button is held).
Option Explicit
Option Default Integer
Dim fruit$(4) Length 16 = ("Apple", "Banana", "Cherry", "Date", "Elderberry")
Dim items$(11) Length 12, nums(3), two$(2, 2), k, downs, ups
For k = 0 To 11 : items$(k) = "Item" + Str$(k + 1) : Next k
Mode 8, 16
Option Console Serial
Cls
GUI LISTBOX #1, fruit$(), 40, 30, 200, 28, RGB(WHITE), RGB(BLUE)
GUI LISTBOX #2, items$(), 300, 30, 200, 28, RGB(WHITE), RGB(BLUE), 5
GUI SLIDER #3, 40, 400, 220, 30, RGB(CYAN), RGB(BLACK), 0, 100
GUI SLIDER #4, 560, 100, 30, 180, RGB(GREEN), RGB(BLACK), 0, 10, 1
Print "the mouse's button up:"; Click(UP)                  ' (held down, as once in a regression, no GUI CLICK can register)
Print "created:"; CtrlVal(#1); CtrlVal(#2); CtrlVal(#3); CtrlVal(#4)
CtrlVal(#1) = 2 : Print "list set to 2:"; CtrlVal(#1)
CtrlVal(#1) = 9 : Print "to 9:"; CtrlVal(#1)
CtrlVal(#1) = -3 : Print "to -3:"; CtrlVal(#1)
CtrlVal(#3) = 150 : Print "slider set to 150:"; CtrlVal(#3)
CtrlVal(#3) = 25.5 : Print "to 25.5:"; CtrlVal(#3)

' the list: its pop-up opens below it, a row (the control's height, 28) an item
GUI CLICK 140, 44
Print "open: under it"; Pixel(100, 58 + 3 * 28 + 14) <> 0
GUI CLICK 140, 58 + 3 * 28 + 14
Print "picked:"; CtrlVal(#1); " "; fruit$(CtrlVal(#1)); ", closed:"; Pixel(100, 58 + 3 * 28 + 14) = 0
GUI CLICK 140, 44 : GUI CLICK 620, 470
Print "a click outside it:"; CtrlVal(#1); ", closed:"; Pixel(100, 58 + 3 * 28 + 14) = 0
' five rows of twelve: an arrow above them and one below (rows from y 86, the down arrow at 226)
GUI CLICK 400, 44
GUI CLICK 400, 240 : GUI CLICK 400, 240
GUI CLICK 400, 100
Print "scrolled two, the first row:"; CtrlVal(#2); " "; items$(CtrlVal(#2))
GUI CLICK 400, 44 : GUI LISTBOX CANCEL
Print "LISTBOX CANCEL:"; CtrlVal(#2); ", closed:"; Pixel(400, 150) = 0
GUI LISTBOX CANCEL

' the sliders: the horizontal one's thumb from x 55 (0) to 245 (100), the vertical one's from y 265 (0) to 115 (10)
GUI CLICK 150, 415 : Print "slider clicked at 150:"; CtrlVal(#3)
GUI CLICK 50, 415 : Print "at 50:"; CtrlVal(#3)
GUI CLICK 575, 190 : Print "vertical, at the middle:"; CtrlVal(#4)
GUI CLICK 575, 140 : Print "near the top:"; CtrlVal(#4)
GUI CLICK 150, 415
GUI CLICK DOWN : Pause 150
Mouse Set 2, 630, 415 : Pause 200
Print "dragged past its end:"; CtrlVal(#3)
GUI CLICK UP : Pause 150

' the interrupts: the list's opening touch is a down, the item chosen an up
GUI INTERRUPT IntDown, IntUp
GUI CLICK 140, 44 : GUI CLICK 140, 58 + 14
Pause 50
Print "interrupts:"; downs; ups; ", picked"; CtrlVal(#1); ", LASTREF"; Touch(LASTREF)
GUI INTERRUPT 0

' a control deleted with its pop-up open: the GUI goes on
GUI CLICK 140, 44 : GUI DELETE #1
GUI CLICK 200, 415 : Print "after deleting an open list, the slider:"; CtrlVal(#3)

Try "GUI LISTBOX #5, nums(), 40, 100, 100, 20"
Try "GUI LISTBOX #5, two$(), 40, 100, 100, 20"
Try "GUI SLIDER #3, 40, 100, 100, 20"
MakeLocal
MakeStatic
GUI DELETE ALL
Mode 8
Option Console Both
End

Sub IntDown
  downs = downs + 1
End Sub

Sub IntUp
  ups = ups + 1
End Sub

Sub MakeLocal
  Local l$(3)
  On Error Skip
  GUI LISTBOX #5, l$(), 40, 100, 100, 20
  Print "a LOCAL array: "; MM.ErrMsg$
End Sub

Sub MakeStatic
  Static s$(2)
  On Error Skip
  GUI LISTBOX #6, s$(), 40, 100, 100, 20
  Print "a STATIC array: "; Choice(MM.ErrNo, MM.ErrMsg$, "taken")
End Sub

Sub Try c$
  On Error Skip
  Execute c$
  Print c$; ": "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
End Sub
