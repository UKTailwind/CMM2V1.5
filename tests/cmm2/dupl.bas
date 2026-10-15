' dupl.bas - P1 step 4: a label defined twice is an error when the program is prepared (the name
' index; before it the first one silently won), as a SUB or FUNCTION defined twice always was.
Print "this line must not run"
GoTo Twice
Twice:
Print "first"
End
Twice:
Print "second"
End
