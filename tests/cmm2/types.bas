' types.bas - P1 step 5b.1: TYPE ... END TYPE.  The types are made when the program is prepared and each block
' is passed over when it runs; LIST TYPE shows them with their offsets and sizes (the PicoMite's layout: numbers
' and nested types on 8 bytes, strings packed, the size padded to the type's alignment).  A command's name can be a
' member (COLOUR AS INTEGER), and a variable can still be called TYPE.
Type Point
  x As INTEGER
  y As INTEGER
End Type

Type Person
  age As INTEGER
  name As STRING LENGTH 20
  height As FLOAT
End Type

Type Shape   ' a comment on the TYPE line
  ' a comment line
  Rem another

  colour As INTEGER
  text As STRING LENGTH 5
  origin As Point
  pts(3) As Point
  tag As STRING LENGTH 3
  flags(1,2) As INT
End Type

Print "before the block"
Type Inline
  a As FLOAT
End Type
Print "after the block"

List Type
List Type person

T1 : T2 : T3 : T4
End

Sub T1
  Local type
  type = 3
  Print "type ="; type
End Sub

Sub T2
  Local type(2)
  type(1) = 4
  Print "type(1) ="; type(1)
End Sub

Sub T3
  Local type$
  type$ = "five"
  Print "type$ = "; type$
End Sub

Sub T4
  Local type%
  type% = 6
  Print "type% ="; type%
End Sub
