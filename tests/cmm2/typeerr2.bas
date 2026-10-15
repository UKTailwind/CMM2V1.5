' typeerr2.bas - P1 step 5b.1: a member of a type not defined before it is an error at RUN, on its line
Print "this line must not run"
Type Outer
  inner As Inner
End Type
Type Inner
  x As INTEGER
End Type
