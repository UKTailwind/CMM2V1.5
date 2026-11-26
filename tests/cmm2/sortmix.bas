' sortmix.bas - SORT of integers, floats and strings, every flag, with and without the index array, from a start
' point and for a count, with many equal elements so that the index shows the order equal elements are left in.
' Each case prints a checksum of the array and the index. The expected output was made by 8.00.00b5's bubble sort:
' the merge sort that replaced it (26 Nov 2026) must leave every element and every index entry where it did.
Option Explicit
Option Default Integer
Const M = 1000000007
Dim a(299), ix(299), n, f, c, st, cnt, useix, h, i, ncase
Dim Float b(299)
Dim s$(299) Length 8
Dim al$ = "aAbBc"
Dim seed = 20261126

' the test's own generator, so that every run sorts the same data (RANDOMIZE n gives no repeatable sequence here)
Function Rand(k) As Integer
  seed = (seed * 1103515245 + 12345) Mod 2147483648
  Rand = (seed \ 65536) Mod k
End Function

Function HashI() As Integer
  Local k, v
  v = 7
  For k = 0 To n - 1
    v = (v * 31 + (a(k) + 1000)) Mod M
    If useix Then v = (v * 37 + ix(k)) Mod M
  Next
  HashI = v
End Function

Function HashF() As Integer
  Local k, v
  v = 7
  For k = 0 To n - 1
    v = (v * 31 + Int(b(k) * 4) + 1000) Mod M
    If useix Then v = (v * 37 + ix(k)) Mod M
  Next
  HashF = v
End Function

Function HashS() As Integer
  Local k, j, v
  v = 7
  For k = 0 To n - 1
    v = (v * 31 + Len(s$(k)) + 1) Mod M
    For j = 1 To Len(s$(k))
      v = (v * 31 + Asc(Mid$(s$(k), j, 1))) Mod M
    Next
    If useix Then v = (v * 37 + ix(k)) Mod M
  Next
  HashS = v
End Function

Function Hash(ty) As Integer
  Select Case ty
    Case 0 : Hash = HashI()
    Case 1 : Hash = HashF()
    Case Else : Hash = HashS()
  End Select
End Function

Sub MakeData
  Local k, j, l
  For k = 0 To 299
    a(k) = Rand(21) - 10
    b(k) = Rand(41) / 4 - 5
    l = Rand(5)
    s$(k) = ""
    For j = 1 To l
      s$(k) = s$(k) + Mid$(al$, Rand(5) + 1, 1)
    Next
  Next
End Sub

' the call, with or without the index, the start and the count (each the whole array when 0)
Sub DoSort(t)
  If t = 0 Then
    If useix Then
      If cnt Then
        Sort a(), ix(), f, st, cnt
      ElseIf st Then
        Sort a(), ix(), f, st
      Else
        Sort a(), ix(), f
      EndIf
    Else
      If cnt Then
        Sort a(), , f, st, cnt
      ElseIf st Then
        Sort a(), , f, st
      Else
        Sort a(), , f
      EndIf
    EndIf
  ElseIf t = 1 Then
    If useix Then
      If cnt Then
        Sort b(), ix(), f, st, cnt
      ElseIf st Then
        Sort b(), ix(), f, st
      Else
        Sort b(), ix(), f
      EndIf
    Else
      If cnt Then
        Sort b(), , f, st, cnt
      ElseIf st Then
        Sort b(), , f, st
      Else
        Sort b(), , f
      EndIf
    EndIf
  Else
    If useix Then
      If cnt Then
        Sort s$(), ix(), f, st, cnt
      ElseIf st Then
        Sort s$(), ix(), f, st
      Else
        Sort s$(), ix(), f
      EndIf
    Else
      If cnt Then
        Sort s$(), , f, st, cnt
      ElseIf st Then
        Sort s$(), , f, st
      Else
        Sort s$(), , f
      EndIf
    EndIf
  EndIf
End Sub

Dim t, r
For t = 0 To 2
  For f = 0 To 7
    For useix = 0 To 1
      For r = 0 To 2
        MakeData
        st = 0 : cnt = 0
        If r = 1 Then st = 37
        If r = 2 Then st = 11 : cnt = 150
        DoSort t
        n = 300
        h = Hash(t)
        Print "type"; t; " flags"; f; " index"; useix; " part"; r; " hash"; h
        Inc ncase
      Next
    Next
  Next
Next
' small and odd sizes
For t = 0 To 2
  For c = 1 To 6
    MakeData
    useix = 1 : f = c And 5 : st = 0
    cnt = Choice(c = 6, 1, c * 3 - 1)
    DoSort t
    n = 300
    h = Hash(t)
    Print "type"; t; " count"; cnt; " flags"; f; " hash"; h
    Inc ncase
  Next
Next
Print "sortmix:"; ncase; " cases"
