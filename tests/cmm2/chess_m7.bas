' chess_m7.bas: made by tools/gen_csubtests.py from the PicoMite's Bas/chess (TSCP 1.81c, MMBasic by Ceptimus)
' TSCP Chess
' Tom Kerrigan's Simple Chess Program
' (TSCP) version 1.81c, 2/3/19
' Copyright 2019 Tom Kerrigan
' MMBasic conversion by Ceptimus

Option explicit:Option default integer:Option base 0
Const autoplay=0

'changes to monitor STACK usage (require picomite 5.07.05RC5 or newer)
'SetTick 200,showstack

'Sub showstack
' Local a%,b%
' a%=MM.Info(stack)
' b%=MM.Info(heap)
' a%=a%-&h2003f800  'stack info from matherp
' Text 240,212,"stack free "+Str$(a%)
' Text 240,220,"heap free  "+Str$(b%)
'End Sub

'end of stack monitor changes ----------------------------------------

' BOARD
Sub init_board
Local i
Restore data_init_hue:For i=0 To 63:Read H(i):Next 'i
Restore data_init_piece:For i=0 To 63:Read P(i):Next 'i
side=LI
xside=DK
castle=15
ep=-1
fifty=0
ply=0
hply=0
set_hash ' init_hash() must be called before this function.
first_move(0)=0
End Sub

Sub init_hash
Local i,j,k
For i=0 To 1
  For j=0 To 5
    For k=0 To 63
      HP(i,j,k)=hash_rand()
    Next k
  Next j
Next i
hash_side=hash_rand()
For i=0 To 63
  hash_ep(i)=hash_rand()
Next i
End Sub

'could this be changed to a simple RND() ?
Function hash_rand()
Local i,r=0
For i=0 To 31:r=r Xor ((2^16)*Rnd())*(2^i):Next 'i
hash_rand=r
End Function

Sub set_hash
Local i
hash=0 'hash is global
For i=0 To 63
  If H(i)<>MT Then hash=hash Xor HP(H(i),P(i),i)
Next i
If side=DK Then hash=hash Xor hash_side
If ep<>-1 Then hash=hash Xor hash_ep(ep)
End Sub

Function in_check(s)
Local i
in_check=1 ' in case king not found (shouldn't happen)
For i=0 To 63
  If (P(i)=KING)And(H(i)=s) Then
    in_check=attack(i, s Xor 1)
    i=63
  EndIf
Next 'i
End Function

Function attack(sq,s)
Local i,j,n
attack=0 ' default to false
For i=0 To 63
  If H(i)=s Then
    If P(i)=PN Then
      If s=LI Then
        If ((i And 7)<>0)And(i-9=sq) Then
          attack=1:Exit For
        EndIf
        If ((i And 7)<>7)And(i-7=sq) Then
          attack=1:Exit For
        EndIf
      Else
        If ((i And 7)<>0)And(i+7=sq) Then
          attack=1:Exit For
        EndIf
        If ((i And 7)<>7)And(i+9=sq) Then
          attack=1:Exit For
        EndIf
      EndIf
    Else
      For j=0 To offsets(P(i))-1
        n=i
        Do
          n=mb(mb64(n)+offset(P(i),j))
          If n=-1 Then Exit
          If n=sq Then
            attack=1:j=offsets(P(i))-1
            i=63:Exit
          EndIf
          If H(n)<>MT Then Exit
          If slide(P(i))=0 Then Exit
        Loop
      Next 'j
    EndIf
  EndIf
Next i
End Function

Sub gen
Local i,j,n,x
first_move(ply+1)=first_move(ply)

For i=0 To 63
  If H(i)=side Then
    If P(i)=PN Then
      If side=LI Then
        If (i And 7) Then
          x=i-9
          If H(x)=DK Then gen_push(i,x,17)
        EndIf
        If (i And 7)<>7 Then
          x=i-7
          If H(x)=DK Then gen_push(i,x,17)
        EndIf
        x=i-8
        If H(x)=MT Then
          gen_push(i,x,16)
          If i>=48 Then
            Inc x,-8
            If H(x)=MT Then gen_push(i,x,24)
          EndIf
        EndIf
      Else
        If (i And 7) Then
          x=i+7
          If H(x)=LI Then gen_push(i,x,17)
        EndIf
        If (i And 7)<>7 Then
          x=i+9
          If H(x)=LI Then gen_push(i,x,17)
        EndIf
        x=i+8
        If H(x)=MT Then
          gen_push(i,x,16)
          If i<=15 Then
            Inc x,8
            If H(x)=MT Then gen_push(i,x,24)
          EndIf
        EndIf
      EndIf
    Else
      For j=0 To offsets(P(i))-1
        n=i
        Do
          n=mb(mb64(n)+offset(P(i),j))
          If n=-1 Then Exit
          If H(n)<>MT Then
            If H(n)=xside Then gen_push(i,n,1)
            Exit
          EndIf
          gen_push(i,n,0)
          If slide(P(i))=0 Then Exit
        Loop
      Next 'j
    EndIf
  EndIf
Next i

If side=LI Then
  If (castle And 1) Then gen_push(E1,G1,2)
  If (castle And 2) Then gen_push(E1,C1,2)
Else
  If (castle And 4) Then gen_push(E8,G8,2)
  If (castle And 8) Then gen_push(E8,C8,2)
EndIf

If ep<>-1 Then
  If side=LI Then
    x=ep+7
    If ((ep And 7)<>0)And(H(x)=LI)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
    x=ep+9
    If ((ep And 7)<>7)And(H(x)=LI)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
  Else
    x=ep-9
    If ((ep And 7)<>0)And(H(x)=DK)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
    x=ep-7
    If ((ep And 7)<>7)And(H(x)=DK)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
  EndIf
EndIf
End Sub

Sub gen_caps
Local i,j,n,x
first_move(ply+1)=first_move(ply)
For i=0 To 63
  If H(i)=side Then
    If P(i)=PN Then
      If side=LI Then
        If (i And 7) Then
          x=i-9
          If H(x)=DK Then gen_push(i,x,17)
        EndIf
        If (i And 7)<>7 Then
          x=i-7
          If H(x)=DK Then gen_push(i,x,17)
        EndIf
        If i<=15 Then
          x=i-8
          If H(x)=MT Then gen_push(i,x,16)
        EndIf
      EndIf
      If side=DK Then
        If (i And 7) Then
          x=i+7
          If H(x)=LI Then gen_push(i,x,17)
        EndIf
        If (i And 7)<>7 Then
          x=i+9
          If H(x)=LI Then gen_push(i,x,17)
        EndIf
        If i>=48 Then
          x=i+8
          If H(x)=MT Then gen_push(i,x,16)
        EndIf
      EndIf
    Else
      For j=0 To offsets(P(i))-1
        n=i
        Do
          n=mb(mb64(n)+offset(P(i),j))
          If n=-1 Then Exit
          If H(n)<>MT Then
            If H(n)=xside Then gen_push(i,n,1)
            Exit
          EndIf
          If slide(P(i))=0 Then Exit
        Loop
      Next 'j
    EndIf
  EndIf
Next i

If ep<>-1 Then
  If side=LI Then
    x=ep+7
    If ((ep And 7)<>0)And(H(x)=LI)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
    x=ep+9
    If ((ep And 7)<>7)And(H(x)=LI)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
  Else
    x=ep-9
    If ((ep And 7)<>0)And(H(x)=DK)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
    x=ep-7
    If ((ep And 7)<>7)And(H(x)=DK)And(P(x)=PN) Then
      gen_push(x,ep,21)
    EndIf
  EndIf
EndIf
End Sub

Sub gen_push(from,too,bits)
Local g,q=0
If (bits And 16) Then
  If side=LI Then
    If too<=H8 Then gen_promote(from,too,bits):Exit Sub
  Else
    If too>=A1 Then gen_promote(from,too,bits):Exit Sub
  EndIf
EndIf
g=first_move(ply+1)
Inc first_move(ply+1),1
'Poke var q,0,from: Poke var q,1,too: Poke var q,2,0: Poke var q,3,bits
q=from Or(too<<8)Or(bits<<24)
If H(too)<>MT Then
  gd(g)=q Or((1000000+P(too)*10-P(from))<<32)
Else
  gd(g)=q Or(hi(from,too)<<32)
EndIf
End Sub

Sub gen_promote(from,too,bits)
Local i,g,p=0
For i=KT To QUEEN
  g=first_move(ply+1)
  Inc first_move(ply+1),1
' Poke var p,0,from: Poke var p,1,too: Poke var p,2,i: Poke var p,3,bits Or 32
  p=from Or(too<<8)Or(i<<16)Or((bits Or 32)<<24)
  gd(g)=p Or((1000000+i*10)<<32)
Next 'i
End Sub

Function makemove(m)
Local from,too
Local m_from=m And &HFF'Peek(var m,0)
Local m_too=m>>8 And &HFF'Peek(var m,1)
Local m_promote=m>>16 And &HFF'Peek(var m,2)
Local m_bits=m>>24 And &HFF'Peek(var m,3)
Local break=0
makemove=0

If (m_bits And 2) Then
  If in_check(side) Then Exit Function
  Select Case m_too
    Case 62
      If (H(F1)<>MT)Or(H(G1)<>MT) Then
        break=1
        ElseIf attack(F1, xside) Or attack(G1, xside) Then
        break=1
      EndIf
      from=H1
      too=F1
    Case 58
      If (H(B1)<>MT)Or(H(C1)<>MT)Or(H(D1)<>MT) Then
        break=1
        ElseIf attack(C1, xside) Or attack(D1, xside) Then
        break=1
      EndIf
      from=A1
      too=D1
    Case 6
      If (H(F8)<>MT)Or(H(G8)<>MT) Then
        break=1
        ElseIf attack(F8, xside) Or attack(G8, xside) Then
        break=1
      EndIf
      from=H8
      too=F8
    Case 2
      If (H(B8)<>MT)Or(H(C8)<>MT)Or(H(D8)<>MT) Then
        break=1
        ElseIf attack(C8, xside) Or attack(D8, xside) Then
        break=1
      EndIf
      from=A8
      too=D8
    Case Else  ' shouldn't get here
      from=-1
      too=-1
  End Select
  If break Then Exit Function
  H(too)=H(from)
  P(too)=P(from)
  H(from)=MT
  P(from)=MT
EndIf

hd(hply,0)=m Or(P(m_too)<<32)Or(castle<<36)Or((ep+1)<<40)Or(fifty<<48)
hd(hply,1)=hash
Inc ply,1
Inc hply,1

castle=castle And (castle_mask(m_from) And castle_mask(m_too))
If (m_bits And 8) Then
  If side=LI Then
    ep=m_too+8
  Else
    ep=m_too-8
  EndIf
Else
  ep=-1
EndIf

If (m_bits And 17) Then
  fifty=0
Else
  Inc fifty,1
EndIf

H(m_too)=side
If (m_bits And 32) Then
  P(m_too)=m_promote
Else
  P(m_too)=P(m_from)
EndIf
H(m_from)=MT
P(m_from)=MT

If (m_bits And 4) Then
  If side=LI Then
    H(m_too+8)=MT
    P(m_too+8)=MT
  Else
    H(m_too-8)=MT
    P(m_too-8)=MT
  EndIf
EndIf

side=side Xor 1
xside=xside Xor 1
If in_check(xside) Then takeback:Exit Function
set_hash
makemove=1
End Function

Sub takeback
Local m,m_from,m_too,m_bits,from,too

side=side Xor 1
xside=xside Xor 1
Inc ply,-1
Inc hply,-1
m=hd(hply,0)
'm_from=Peek(var m,0):m_too=Peek(var m,1):m_bits=Peek(var m,3)
m_from=m And &HFF:m_too=m>>8 And &HFF:m_bits=m>>24 And &HFF
castle=m>>36 And 15
ep=(m>>40 And 255)-1
fifty=m>>48
hash=hd(hply,1)
H(m_from)=side
If (m_bits And 32) Then
  P(m_from)=PN
Else
  P(m_from)=P(m_too)
EndIf
If (m>>32 And 15)=MT Then
  H(m_too)=MT
  P(m_too)=MT
Else
  H(m_too)=xside
  P(m_too)=m>>32 And 15
EndIf
If (m_bits And 2) Then
  Select Case m_too
    Case 62
      from=F1
      too=H1
    Case 58
      from=D1
      too=A1
    Case 6
      from=F8
      too=H8
    Case 2
      from=D8
      too=A8
    Case Else ' shouldn't get here
      from=-1
      too=-1
  End Select
  H(too)=side
  P(too)=ROOK
  H(from)=MT
  P(from)=MT
EndIf

If (m_bits And 4) Then
  If side=LI Then
    H(m_too+8)=xside
    P(m_too+8)=PN
  Else
    H(m_too-8)=xside
    P(m_too-8)=PN
  EndIf
EndIf
End Sub

' BOOK
Dim book_file=1 'set to 1 if opening book is present

Sub open_book
Local d$=MM.DEVICE$

On Error Skip
Open "book.txt" For input As#1
If MM.Errno Then
  oPrintR "Opening book missing."
Else
  Close #1
  book_file=1
EndIf
End Sub

Sub close_book
book_file=0
End Sub

Function book_move()
Local L$,book_line$
Local i,j,m,found
Local move(50)  'the possible book moves
Local count(50) 'the number of occurrences of each move
Local moves,total_count

moves=0
total_count=0
book_move=-1 'default to "no book move"
If book_file=0 Or hply>25 Then Exit Function
L$=""
For i=0 To hply-1
    L$=L$+move_str$(hd(i,0))+" "
Next 'i
Open "book.txt" For input As#1
Do While Not Eof(#1)
  Input #1, book_line$
  If book_match(L$, book_line$) Then
    m=parse_move(Mid$(book_line$, Len(L$)+1))
    If m<>-1 Then
      m=gd(m) And &HFFFFFFFF
      found=0
      For j=0 To moves-1
        If move(j)=m Then Inc count(j),1:found=1:j=moves
      Next 'j
      If found=0 Then
        move(moves)=m
        count(moves)=1
        Inc moves,1
      EndIf
      Inc total_count,1
    EndIf
  EndIf
Loop
Close #1
If moves=0 Then Exit Function

j=Int(Rnd()*total_count)
For i=0 To moves-1
  j=j-count(i)
  If j<0 Then book_move=move(i):i=moves
Next 'i
End Function

Function book_match(s1$, s2$)
book_match=0
If LEFT$(s2$, Len(s1$))=s1$ Then book_match=1
End Function

' DATA
Sub init_arrays
Local i,j


data_mb:
Data -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
Data -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
Data -1,  0,  1,  2,  3,  4,  5,  6,  7, -1
Data -1,  8,  9, 10, 11, 12, 13, 14, 15, -1
Data -1, 16, 17, 18, 19, 20, 21, 22, 23, -1
Data -1, 24, 25, 26, 27, 28, 29, 30, 31, -1
Data -1, 32, 33, 34, 35, 36, 37, 38, 39, -1
Data -1, 40, 41, 42, 43, 44, 45, 46, 47, -1
Data -1, 48, 49, 50, 51, 52, 53, 54, 55, -1
Data -1, 56, 57, 58, 59, 60, 61, 62, 63, -1
Data -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
Data -1, -1, -1, -1, -1, -1, -1, -1, -1, -1

data_mb64:
Data 21, 22, 23, 24, 25, 26, 27, 28
Data 31, 32, 33, 34, 35, 36, 37, 38
Data 41, 42, 43, 44, 45, 46, 47, 48
Data 51, 52, 53, 54, 55, 56, 57, 58
Data 61, 62, 63, 64, 65, 66, 67, 68
Data 71, 72, 73, 74, 75, 76, 77, 78
Data 81, 82, 83, 84, 85, 86, 87, 88
Data 91, 92, 93, 94, 95, 96, 97, 98

data_offset:
Data   0,   0,   0,   0,   0,   0,   0,   0
Data -21, -19, -12,  -8,   8,  12,  19,  21
Data -11,  -9,   9,  11,   0,   0,   0,   0
Data -10,  -1,   1,  10,   0,   0,   0,   0
Data -11, -10,  -9,  -1,   1,   9,  10,  11
Data -11, -10,  -9,  -1,   1,   9,  10,  11

data_castle_mask:
Data  7, 15, 15, 15,  3, 15, 15, 11
Data 15, 15, 15, 15, 15, 15, 15, 15
Data 15, 15, 15, 15, 15, 15, 15, 15
Data 15, 15, 15, 15, 15, 15, 15, 15
Data 15, 15, 15, 15, 15, 15, 15, 15
Data 15, 15, 15, 15, 15, 15, 15, 15
Data 15, 15, 15, 15, 15, 15, 15, 15
Data 13, 15, 15, 15, 12, 15, 15, 14


data_init_hue:
Data 1, 1, 1, 1, 1, 1, 1, 1
Data 1, 1, 1, 1, 1, 1, 1, 1
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 0, 0, 0, 0, 0, 0, 0, 0
Data 0, 0, 0, 0, 0, 0, 0, 0

data_init_piece:
Data 3, 1, 2, 4, 5, 2, 1, 3
Data 0, 0, 0, 0, 0, 0, 0, 0
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 6, 6, 6, 6, 6, 6, 6, 6
Data 0, 0, 0, 0, 0, 0, 0, 0
Data 3, 1, 2, 4, 5, 2, 1, 3


Restore data_mb:For i=0 To 119:Read mb(i):Next 'i
Restore data_mb64:For i=0 To 63:Read mb64(i):Next 'i
Restore data_offset

For i=0 To 5
  For j=0 To 7
    Read offset(i,j)
  Next 'j
Next 'i

Restore data_castle_mask:For i=0 To 63:Read castle_mask(i):Next 'i
Restore data_pawn_pcsq:For i=0 To 63:Read pawn_pcsq(i):Next 'i
Restore data_knight_pcsq:For i=0 To 63:Read knight_pcsq(i):Next 'i
Restore data_bishop_pcsq:For i=0 To 63:Read bishop_pcsq(i):Next 'i
Restore data_king_pcsq:For i=0 To 63:Read king_pcsq(i):Next 'i
Restore data_king_endgame_pcsq
For i=0 To 63:Read king_endgame_pcsq(i):Next 'i
Restore data_flip:For i=0 To 63:Read flip(i):Next 'i
End Sub

' DEFS
Const GEN_STACK  = 1120' 400 '1120 '208
Const MAX_PLY    = 32 ' as TSCP's C
Const HIST_STACK = 400
Const LI  = 0 'Light
Const DK  = 1 'Dark
Const PN  = 0 'Pawn
Const KT  = 1 'Knight
Const BISHOP = 2
Const ROOK   = 3
Const QUEEN  = 4
Const KING   = 5
Const MT = 6 'Empty
Const A1 =56
Const B1 =57
Const C1 =58
Const D1 =59
Const E1 =60
Const F1 =61
Const G1 =62
Const H1 =63
Const A8 = 0
Const B8 = 1
Const C8 = 2
Const D8 = 3
Const E8 = 4
Const F8 = 5
Const G8 = 6
Const H8 = 7

' EVAL
Const DPP  = 10 'Doubled Pawn Penalty
Const IPP  = 20 'Isolated Pawn Penalty
Const BPP  =  8 'Backwards Pawn Penalty
Const PPB  = 20 'Passed Pawn Bonus
Const RSOB = 10 'Rook Semi Open File Bonus
Const ROB  = 15 'Rook Open File Bonus
Const RSB  = 20 'Rook On Seventh Bonus

Dim piece_value(5)=(100,300,300,500,900,0)
Dim pawn_pcsq(63)
data_pawn_pcsq:
Data  0,   0,   0,   0,   0,   0,   0,   0
Data  5,  10,  15,  20,  20,  15,  10,   5
Data  4,   8,  12,  16,  16,  12,   8,   4
Data  3,   6,   9,  12,  12,   9,   6,   3
Data  2,   4,   6,   8,   8,   6,   4,   2
Data  1,   2,   3, -10, -10,   3,   2,   1
Data  0,   0,   0, -40, -40,   0,   0,   0
Data  0,   0,   0,   0,   0,   0,   0,   0

Dim knight_pcsq(63)
data_knight_pcsq:
Data -10, -10, -10, -10, -10, -10, -10, -10
Data -10,   0,   0,   0,   0,   0,   0, -10
Data -10,   0,   5,   5,   5,   5,   0, -10
Data -10,   0,   5,  10,  10,   5,   0, -10
Data -10,   0,   5,  10,  10,   5,   0, -10
Data -10,   0,   5,   5,   5,   5,   0, -10
Data -10,   0,   0,   0,   0,   0,   0, -10
Data -10, -30, -10, -10, -10, -10, -30, -10

Dim bishop_pcsq(63)
data_bishop_pcsq:
Data -10, -10, -10, -10, -10, -10, -10, -10
Data -10,   0,   0,   0,   0,   0,   0, -10
Data -10,   0,   5,   5,   5,   5,   0, -10
Data -10,   0,   5,  10,  10,   5,   0, -10
Data -10,   0,   5,  10,  10,   5,   0, -10
Data -10,   0,   5,   5,   5,   5,   0, -10
Data -10,   0,   0,   0,   0,   0,   0, -10
Data -10, -10, -20, -10, -10, -20, -10, -10

Dim king_pcsq(63)
data_king_pcsq:
Data -40, -40, -40, -40, -40, -40, -40, -40
Data -40, -40, -40, -40, -40, -40, -40, -40
Data -40, -40, -40, -40, -40, -40, -40, -40
Data -40, -40, -40, -40, -40, -40, -40, -40
Data -40, -40, -40, -40, -40, -40, -40, -40
Data -40, -40, -40, -40, -40, -40, -40, -40
Data -20, -20, -20, -20, -20, -20, -20, -20
Data   0,  20,  40, -20,   0, -20,  40,  20

Dim king_endgame_pcsq(63)
data_king_endgame_pcsq:
Data  0,  10,  20,  30,  30,  20,  10,   0
Data 10,  20,  30,  40,  40,  30,  20,  10
Data 20,  30,  40,  50,  50,  40,  30,  20
Data 30,  40,  50,  60,  60,  50,  40,  30
Data 30,  40,  50,  60,  60,  50,  40,  30
Data 20,  30,  40,  50,  50,  40,  30,  20
Data 10,  20,  30,  40,  40,  30,  20,  10
Data  0,  10,  20,  30,  30,  20,  10,   0

Dim flip(63)
data_flip:
Data 56,  57,  58,  59,  60,  61,  62,  63
Data 48,  49,  50,  51,  52,  53,  54,  55
Data 40,  41,  42,  43,  44,  45,  46,  47
Data 32,  33,  34,  35,  36,  37,  38,  39
Data 24,  25,  26,  27,  28,  29,  30,  31
Data 16,  17,  18,  19,  20,  21,  22,  23
Data  8,   9,  10,  11,  12,  13,  14,  15
Data  0,   1,   2,   3,   4,   5,   6,   7

Dim pawn_rank(1,9)
Dim piece_mat(1)
Dim pawn_mat(1)

Function eval_()
Local i,f,score(1)

For i=0 To 9
  pawn_rank(LI,i)=0
  pawn_rank(DK,i)=7
Next 'i
piece_mat(LI)=0
piece_mat(DK)=0
pawn_mat(LI)=0
pawn_mat(DK)=0
For i=0 To 63
  If H(i)<>MT Then
    If P(i)=PN Then
      Inc pawn_mat(H(i)),piece_value(PN)
      f=(i And 7)+1 ' add 1 because of the extra file in the array
      If H(i)=LI Then
        If pawn_rank(LI,f)<i\8 Then pawn_rank(LI,f)=i\8
      Else
        If pawn_rank(DK,f)>i\8 Then pawn_rank(DK,f)=i\8
      EndIf
    Else
      Inc piece_mat(H(i)),piece_value(P(i))
    EndIf
  EndIf
Next 'i

score(LI)=piece_mat(LI)+pawn_mat(LI)
score(DK)=piece_mat(DK)+pawn_mat(DK)
For i=0 To 63
  Do
    If H(i)<>MT Then Exit
    Inc i,1:If i>63 Then Exit
  Loop :If i>63 Then Exit For
  If H(i)=LI Then
    Select Case P(i)
      Case PN
        Inc score(LI),eval_light_pawn(i)
      Case KT
        Inc score(LI),knight_pcsq(i)
      Case BISHOP
        Inc score(LI),bishop_pcsq(i)
      Case ROOK
        If pawn_rank(LI,(i And 7)+1)=0 Then
          If pawn_rank(DK,(i And 7)+1)=7 Then
            Inc score(LI),ROB
          Else
            Inc score(LI),RSOB
          EndIf
        EndIf
        If i\8=1 Then
          Inc score(LI),RSB
        EndIf
      Case KING
        If piece_mat(DK)<=1200 Then
          Inc score(LI),king_endgame_pcsq(i)
        Else
          Inc score(LI),eval_light_king(i)
        EndIf
    End Select
  Else
    Select Case P(i)
      Case PN
        Inc score(DK),eval_dark_pawn(i)
      Case KT
        Inc score(DK),knight_pcsq(flip(i))
      Case BISHOP
        Inc score(DK),bishop_pcsq(flip(i))
      Case ROOK
        If pawn_rank(DK,(i And 7)+1)=7 Then
          If pawn_rank(LI,(i And 7)+1)=0 Then
            Inc score(DK),ROB
          Else
            Inc score(DK),RSOB
          EndIf
        EndIf
        If i\8=6 Then
          Inc score(DK),RSB
        EndIf
      Case KING
        If piece_mat(LI)<=1200 Then
          Inc score(DK),king_endgame_pcsq(flip(i))
        Else
          Inc score(DK),eval_dark_king(i)
        EndIf
    End Select
  EndIf
Next 'i

If side=LI Then
  eval_=score(LI)-score(DK)
Else
  eval_=score(DK)-score(LI)
EndIf
End Function

Function eval_light_pawn(sq)
Local r ' the value to return
Local f ' the pawn's file
Local x

r=0
f=(sq And 7)+1
Inc r,pawn_pcsq(sq)
x=sq\8

If pawn_rank(LI,f)>x Then Inc r,-DPP
If (pawn_rank(LI,f-1)=0)And(pawn_rank(LI,f+1)=0) Then
    Inc r,-IPP
ElseIf (pawn_rank(LI,f-1)<x)And(pawn_rank(LI,f+1)<x) Then
  Inc r,-BPP
EndIf

If (pawn_rank(DK,f-1)>=x)And(pawn_rank(DK,f)>=x) Then
  If pawn_rank(DK,f+1)>=x Then
   Inc r,(7-x)*PPB
  EndIf
EndIf

eval_light_pawn=r
End Function

Function eval_dark_pawn(sq)
Local r
Local f
Local x

f=(sq And 7)+1
r=pawn_pcsq(flip(sq))
x=sq\8

If pawn_rank(DK,f)<x Then Inc r,-DPP
If (pawn_rank(DK,f-1)=7)And(pawn_rank(DK,f+1)=7) Then
  Inc r,-IPP
ElseIf (pawn_rank(DK,f-1)>x)And(pawn_rank(DK,f+1)>x) Then
  Inc r,-BPP
EndIf

If (pawn_rank(LI,f-1)<=x)And(pawn_rank(LI,f)<=x) Then
  If pawn_rank(LI,f+1)<=x Then
    Inc r,x*PPB
  EndIf
EndIf
eval_dark_pawn=r
End Function

Function eval_light_king(sq)
Local r
Local i

r=king_pcsq(sq)
If (sq And 7)<3 Then
  Inc r,eval_lkp(1)
  Inc r,eval_lkp(2)
  Inc r,eval_lkp(3)\2
ElseIf (sq And 7)>4 Then
  Inc r,eval_lkp(8)
  Inc r,eval_lkp(7)
  Inc r,eval_lkp(6)\2
Else
  For i=(sq And 7) To (sq And 7)+2
    If (pawn_rank(LI,i)=0)And(pawn_rank(DK,i)=7) Then
      Inc r,-10
    EndIf
  Next 'i
EndIf

r=r*piece_mat(DK)
r=r\3100
eval_light_king=r
End Function

Function eval_lkp(f)
Local r=0

If pawn_rank(LI,f)=6 Then
ElseIf pawn_rank(LI,f)=5 Then
  Inc r,-10
ElseIf pawn_rank(LI,f) Then
  Inc r,-20
Else
  Inc r,-25
EndIf

If pawn_rank(DK,f)=7 Then
  Inc r,-15
ElseIf pawn_rank(DK,f)=5 Then
  Inc r,-10
ElseIf pawn_rank(DK,f)=4 Then
  Inc r,-5
EndIf

eval_lkp=r
End Function

Function eval_dark_king(sq)
Local r,i

r=king_pcsq(flip(sq))
If (sq And 7)<3 Then
  Inc r,eval_dkp(1)
  Inc r,eval_dkp(2)
  Inc r,eval_dkp(3)\2
ElseIf (sq And 7)>4 Then
  Inc r,eval_dkp(8)
  Inc r,eval_dkp(7)
  Inc r,eval_dkp(6)\2
Else
  For i=(sq And 7) To (sq And 7)+2
    If (pawn_rank(LI,i)=0)And(pawn_rank(DK,i)=7) Then
      Inc r,-10
    EndIf
  Next 'i
EndIf

'r=r*piece_mat(LI)
'r=r\3100
eval_dark_king=(r*piece_mat(LI))\3100
End Function

Function eval_dkp(f)
Local r=0

If pawn_rank(DK,f)=1 Then
ElseIf pawn_rank(DK,f)=2 Then
  Inc r,-10
ElseIf pawn_rank(DK,f)<>7 Then
  Inc r,-20
Else
  Inc r,-25
EndIf

If pawn_rank(LI,f)=0 Then
  Inc r,-15
ElseIf pawn_rank(LI,f)=2 Then
  Inc r,-10
ElseIf pawn_rank(LI,f)=3 Then
  Inc r,-5
EndIf

eval_dkp=r
End Function

' GRAPHICS
Const P_Y=228
Const R_H=7
Const B_W=136
Const B_T=10
Const E_W=320
Const B_T1=184
Const B_W1=224

Sub load_sprites

End Sub

Sub oPrint s$
Print s$;

End Sub

Sub oPrintR s$
Print s$

End Sub

Function oInput$(prompt$)
Local s$
draw_chessboard
Box 0,P_Y,E_W,R_H,0,0,0
oPrint prompt$
'-------------------------------Volhout---------- autoplay itself -----
If autoplay Then
 s$="on" '<--------this is autoplay
Else
 Input "",s$
EndIf
Blit B_W,B_T1+R_H,B_W,B_T1,B_W1,P_Y-B_T1
Blit 0,B_T+R_H,0,B_T,B_W,P_Y-B_T
Box 0,P_Y,E_W,R_H,0,0,0
'  Print
oInput$=s$
End Function

Sub draw_chessboard

End Sub

' MAIN
Sub main
Local computer_side,m,s$,selfplay
CLS
Text 0,0, "'help' lists commands. TSCP Chess By Tom Kerrigan. Converted MMBasic Ceptimus"

init_hash
init_board
open_book
gen
computer_side=MT
max_time=6e5' 10min '18e5 '30min' 4e5 '400sec '2^24
max_depth=5

Do
  ' auto: the computer takes whichever side is to move, until a key
  If selfplay Then
    computer_side=side
    If Inkey$<>"" Then selfplay=0:computer_side=MT
  EndIf
  If side=computer_side Then
    think(1)
    If pv(0,0)=0 Then
      oPrintR ""
      oPrintR "(no legal moves)"
      computer_side=MT
      selfplay=0
      print_result
    Else
      oPrintR""
      oPrintR "Computer's move: "+UCASE$(move_str$(pv(0,0)))
      m=makemove(pv(0,0))
      ply=0
      gen
      print_result
      If selfplay Then
        draw_chessboard ' no tscp> prompt comes to draw it
        ' a draw print_result reported ends auto
        If (reps()=2)Or(fifty>=100) Then selfplay=0:computer_side=MT
      EndIf
    EndIf
  Else
    s$=oInput$("tscp> ")
    If s$="on" Then
      computer_side=side
    ElseIf s$="off" Then
      computer_side=MT
    ElseIf s$="auto" Then
      selfplay=1
    ElseIf (LEFT$(s$,2)="st")And(Len(s$)>2) Then
      max_time=1000*Val(Mid$(s$,3))
      max_depth=7'32
    ElseIf (LEFT$(s$,2)="sd")And(Len(s$)>2) Then
      max_depth=Val(Mid$(s$,3))
      If max_depth<1 Then max_depth=1
      If max_Depth>MAX_PLY Then max_depth=MAX_PLY+1
      max_time=72e5'2 hours '2^24
    ElseIf s$="undo" Then
      If hply Then
        computer_side=MT
        takeback
        ply=0
        gen
      EndIf
    ElseIf s$="new" Then
      computer_side=MT
      init_board
      gen
'      ElseIf s$="d" Then
'        print_board
    ElseIf s$="v" Then
      flipped=Not flipped
    ElseIf s$="bench" Then
      computer_side=MT
      bench
    ElseIf s$="bye" Then
      oPrintR "Share and enjoy!"
      Exit
    ElseIf s$="help" Then
      oPrintR "on - computer plays"
      oPrintR "     for the side to move"
      oPrintR "auto - computer plays both sides"
      oPrintR "       until a key is pressed"
      oPrintR "off - computer stops playing"
      oPrintR "st n - search for n secs per move"
      oPrintR "sd n - search n (1-7) ply per move"
      oPrintR "undo - takes back a move"
      oPrintR "new - starts a new game"
      oPrintR "v - view board from opposite side"
      oPrintR "bench - run the built-in benchmark"
      oPrintR "bye - exit the program"
      oPrintR "Enter moves in coordinate notation"
      oPrintR "               e.g., e2e4, e7e8Q"
    Else
      m=parse_move(s$)
      If m=-1 Then
        oPrintR ""
        oPrintR "Illegal move."
      ElseIf makemove(gd(m) And &HFFFFFFFF)=0 Then
        oPrintR ""
        oPrintR "Illegal move."
      Else
        ply=0
        gen
        print_result
        draw_chessboard
      EndIf
    EndIf
  EndIf
Loop
close_book
End Sub

Function parse_move(vs$)
Local from,too,i,a(4),s$

parse_move=-1 'default to 'illegal move'
s$=LCASE$(vs$)
If Len(s$)<4 Then Exit Function
For i=0 To 3:a(i)=Asc(Mid$(s$,i+1)):Next 'i
If (a(0)<Asc("a"))Or(a(0)>Asc("h"))Then Exit Function
If (a(1)<Asc("1"))Or(a(1)>Asc("8"))Then Exit Function
If (a(2)<Asc("a"))Or(a(2)>Asc("h"))Then Exit Function
If (a(3)<Asc("1"))Or(a(3)>Asc("8"))Then Exit Function
a(4)=Asc("q")
If Len(s$)>4 Then a(4)=Asc(Mid$(s$,5))

from=a(0)-Asc("a")
from=from+8*(8-(a(1)-Asc("0")))
too=a(2)-Asc("a")
too=too+8*(8-(a(3)-Asc("0")))

For i=0 To first_move(1)-1
'  If (Peek(var gd(i,0),0)=from)And(Peek(var gd(i,0),1)=too)Then
   If (gd(i)And &HFF)=from And (gd(i)>>8 And &HFF)=too Then
'    If (Peek(var gd(i,0),3)And 32) Then
    If gd(i)And &H20000000 Then
      Select Case a(4)
        Case Asc("n")
          parse_move=i:Exit For
        Case Asc("b")
          parse_move=i+1:Exit For
        Case Asc("r")
          parse_move=i+2:Exit For
        Case Else 'assume it's a queen
          parse_move=i+3:Exit For
      End Select
    EndIf
    parse_move=i:Exit For
  EndIf
Next i
End Function

Function move_str$(m)
'Local from=Peek(var m,0),too=Peek(var m,1)
Local from=m And &HFF, too=m>>8 And &HFF

move_str$=Chr$((from And 7)+Asc("a"))
move_str$=move_str$+Chr$(8-(from\8)+Asc("0"))
move_str$=move_str$+Chr$((too And 7)+Asc("a"))
move_str$=move_str$+Chr$(8-(too\8)+Asc("0"))

'If (Peek(var m,3)And 32) Then
If (m And &H20000000) Then
  Select Case Peek(var m,2)
    Case KT
      move_str$=move_str$+"N"
    Case BISHOP
      move_str$=move_str$+"B"
    Case ROOK
      move_str$=move_str$+"R"
    Case Else
      move_str$=move_str$+"Q"
  End Select
EndIf
End Function

'Sub print_board
'  Local i
'
'  oPrintR "" :oPrint "8 "
'  For i=0 To 63
'    Select Case H(i)
'      Case MT
'        oPrint " ."
'      Case LI
'        oPrint " "+Mid$(piece_char$,P(i)+1,1)
'      Case DK
'        oPrint " "+Mid$(piece_char$,P(i)+7,1)
'    End Select
'    If ((i+1)Mod 8=0)And(i<>63) Then oPrintR "" :oPrint Str$(7-(i\8))+" "
'  Next 'i
'  oPrintR "" :oPrintR "" :oPrintR "   a b c d e f g h":oPrintR ""
'End Sub

Sub print_result
Local i

For i=0 To first_move(1)-1
  If makemove(gd(i) And &HFFFFFFFF) Then takeback:Exit For
Next 'i
If i=first_move(1)) Then
  If in_check(side) Then
    If side=LI Then
      oPrintR "0-1 {Black mates}"
    Else
      oPrintR "1-0 {White mates}"
    EndIf
  Else
    oPrintR "1/2-1/2 {Stalemate}"
  EndIf
ElseIf reps()=2 Then
  oPrintR "1/2-1/2 {Draw by repetition}"
ElseIf fifty>=100 Then
  oPrintR "1/2-1/2 {Draw by fifty move rule}"
EndIf

End Sub

data_bench_color:
Data 6, 1, 1, 6, 6, 1, 1, 6
Data 1, 6, 6, 6, 6, 1, 1, 1
Data 6, 1, 6, 1, 1, 6, 1, 6
Data 6, 6, 6, 1, 6, 6, 0, 6
Data 6, 6, 1, 0, 6, 6, 6, 6
Data 6, 6, 0, 6, 6, 6, 0, 6
Data 0, 0, 0, 6, 6, 0, 0, 0
Data 0, 6, 0, 6, 0, 6, 0, 6

data_bench_piece:
Data 6, 3, 2, 6, 6, 3, 5, 6
Data 0, 6, 6, 6, 6, 0, 0, 0
Data 6, 0, 6, 4, 0, 6, 1, 6
Data 6, 6, 6, 1, 6, 6, 1, 6
Data 6, 6, 0, 0, 6, 6, 6, 6
Data 6, 6, 0, 6, 6, 6, 0, 6
Data 0, 0, 4, 6, 6, 0, 2, 0
Data 3, 6, 2, 6, 3, 6, 5, 6

Sub bench
Local i,t
Local float nps

close_book

Restore data_bench_color:For i=0 To 63:Read H(i):Next 'i
Restore data_bench_piece:For i=0 To 63:Read P(i):Next 'i

side=LI
xside=DK
castle=0
ep=-1
fifty=0
ply=0
hply=0
set_hash
draw_chessboard
'  print_board
max_time=300e5 '60 min
max_depth=5
think(1)
t=Timer-start_time
oPrintR ""
oPrintR "" :oPrintR "Nodes: "+Str$(nodes)


init_board
max_time=18e5'30min'2^24
max_depth=3
open_book
gen
End Sub

' SEARCH
Sub think(output)
Local i,j,x
Local move$=""

pv(0,0)=book_move():If pv(0,0)<>-1 Then Exit Sub

stop_search=0
start_time=Timer
stop_time=start_time+max_time

ply=0
nodes=0
For i=0 To MAX_PLY
  For j=0 To MAX_PLY
    pv(i,j)=0
  Next 'j
 Next 'i
For i=0 To 63
  For j=0 To 63
    hi(i,j)=0
  Next 'j
Next 'i

If output=1 Then oPrint "ply  nodes  score   PV"
For i=1 To max_depth
  follow_pv=1
  x=search(-10000,10000,i)
  If stop_search Then Exit For
  If output=1 Then
   For j = 0 To pvl(0)-1
    move$=move$+" "+move_str$(pv(0, j))
   Next 'j
   move$=UCASE$(move$)
   oPrintR ""
   oPrint Str$(i, 3)+Str$(nodes, 7)+Str$(x, 7)+"  "+move$
'   oPrintR ""
   move$=""
   EndIf
  If x>9000 Or x<-9000 Then i=max_depth
Next i
If stop_search Then
  Do
    If ply = 0 Then Exit
    takeback()
  Loop
EndIf
End Sub

' --- mmb2csub: search is now the CSUB below (original in p.bas.bak)

Function reps()
Local i
reps=0
For i=hply-fifty To hply-1
  If hd(i,1)=hash Then Inc reps'1
Next 'i
End Function

Sub sort_pv
Local i
follow_pv=0
For i=first_move(ply) To first_move(ply+1)-1
  If (gd(i) And &HFFFFFFFF)=pv(0,ply) Then
    follow_pv=1
    Inc gd(i),10000000<<32
    i=first_move(ply+1)-1
  EndIf
Next 'i
End Sub

Sub tscpSort(from)
Local g
Local i,bs,bi
bs=-1
bi=from

For i=from To first_move(ply+1)-1
  If (gd(i)>>32)>bs Then bs=gd(i)>>32:bi=i
Next 'i
g=gd(from):gd(from)=gd(bi):gd(bi)=g
End Sub

' globals: Dim'd here at program level rather than inside init_arrays,
' so mmb2csub sees them as globals
Dim H(63) 'Hue
Dim P(63) 'Piece
Dim side
Dim xside
Dim castle
Dim ep
Dim fifty
Dim hash
Dim ply
Dim hply
Dim gd(GEN_STACK) 'Gen Dat: the move in the low 32 bits, its score in the high 32
Dim first_move(MAX_PLY)
Dim hi(63,63) 'History
Dim hd(HIST_STACK, 1) 'History Data: (n,0) the move, the captured piece<<32, castle<<36,
'  (ep+1)<<40 and fifty<<48; (n,1) the hash (48 bits)
Dim max_time,max_depth
Dim start_time,stop_time
Dim nodes
Dim pv(MAX_PLY, MAX_PLY)
Dim pvl(MAX_PLY) 'Pv Length
Dim ss_alpha(MAX_PLY),ss_beta(MAX_PLY),ss_depth(MAX_PLY),ss_move(MAX_PLY) 'search stack
Dim follow_pv
Dim HP(1,5,63) 'Hash Piece
Dim hash_side
Dim hash_ep(63)
Dim mb(119)
Dim mb64(63)
Dim slide(5)=(0,0,1,1,1,0)
Dim offsets(5)=(0,8,4,4,8,8)
Dim offset(5,7)
Dim castle_mask(63)
Dim piece_char$="PNBRQKpnbrqk"
Dim stop_search=0
Dim flipped=0
init_arrays 'initialize those arrays that are too big for a one-line statement
load_sprites
'Dim km1, km2, km3
init_hash
init_board
bench
Print "bench done"

' --- mmb2csub: generated code for search
' The wrapper keeps the original name, so call sites are unchanged.
Function search(a%, b%, d%)
  Local __r%
  Local Integer __gt(42)
  __gt(0) = Peek(VARADDR bishop_pcsq%())
  __gt(1) = Peek(VARADDR castle_mask%())
  __gt(2) = Peek(VARADDR first_move%())
  __gt(3) = Peek(VARADDR flip%())
  __gt(4) = Peek(VARADDR gd%())
  __gt(5) = Peek(VARADDR h%())
  __gt(6) = Peek(VARADDR hash_ep%())
  __gt(7) = Peek(VARADDR hd%())
  __gt(8) = Peek(VARADDR hi%())
  __gt(9) = Peek(VARADDR hp%())
  __gt(10) = Peek(VARADDR king_endgame_pcsq%())
  __gt(11) = Peek(VARADDR king_pcsq%())
  __gt(12) = Peek(VARADDR knight_pcsq%())
  __gt(13) = Peek(VARADDR mb%())
  __gt(14) = Peek(VARADDR mb64%())
  __gt(15) = Peek(VARADDR offset%())
  __gt(16) = Peek(VARADDR offsets%())
  __gt(17) = Peek(VARADDR p%())
  __gt(18) = Peek(VARADDR pawn_mat%())
  __gt(19) = Peek(VARADDR pawn_pcsq%())
  __gt(20) = Peek(VARADDR pawn_rank%())
  __gt(21) = Peek(VARADDR piece_mat%())
  __gt(22) = Peek(VARADDR piece_value%())
  __gt(23) = Peek(VARADDR pv%())
  __gt(24) = Peek(VARADDR pvl%())
  __gt(25) = Peek(VARADDR slide%())
  __gt(26) = Peek(VARADDR ss_alpha%())
  __gt(27) = Peek(VARADDR ss_beta%())
  __gt(28) = Peek(VARADDR ss_depth%())
  __gt(29) = Peek(VARADDR ss_move%())
  __gt(30) = Peek(VARADDR castle%)
  __gt(31) = Peek(VARADDR ep%)
  __gt(32) = Peek(VARADDR fifty%)
  __gt(33) = Peek(VARADDR follow_pv%)
  __gt(34) = Peek(VARADDR hash%)
  __gt(35) = Peek(VARADDR hash_side%)
  __gt(36) = Peek(VARADDR hply%)
  __gt(37) = Peek(VARADDR nodes%)
  __gt(38) = Peek(VARADDR ply%)
  __gt(39) = Peek(VARADDR side%)
  __gt(40) = Peek(VARADDR stop_search%)
  __gt(41) = Peek(VARADDR stop_time%)
  __gt(42) = Peek(VARADDR xside%)
  searchK __r%, a%, b%, d%, __gt()
  search = __r%
End Function

CSUB searchK INTEGER, INTEGER, INTEGER, INTEGER, INTEGER   ' Cortex-M7 (CMM2)
	00000484
	'mm_mark
	F04FB570 F8D525E0 6A1B3D08 68346FDE 6B1BB994 5080F44F F8D54798 60303D08 
	6FDB6A1B F8D5605C 6A1B3D08 685A6FDB 5F80F5B2 605CBF88 23E0F04F 3D08F8D3 
	6FDB6A1B BD706858 
	'f_eval_lkp
	4606B570 FFD8F7FF 23E0F04F 68324604 3D08F8D3 6A1B0111 6DEB6FDD 1202EB03 
	6851585B 43081F98 1F58D009 D0124308 D013430B 0013F06F 31FFF04F 2000E001 
	E9D24601 1FDE3202 D10A4316 F161380F 606C0100 F06FBD70 E7ED0009 0018F06F 
	1F5EE7EA D1014316 E7F0380A 43133B04 3805D1EF 0000E7EB 
	'f_eval_dkp
	4606B5F8 FF9CF7FF 23E0F04F 68324604 3D08F8D3 6A1B0117 6DEE6FDD 1202EB06 
	3102E9D2 43081E58 1E98D00A D0134308 430B3B07 F06FD013 F04F0013 E00131FF 
	46012000 685259F3 0602EA53 380FD10A 0100F161 BDF8606C 0009F06F F06FE7ED 
	E7EA0018 43161E9E 380AD101 3B03E7F0 D1EF4313 E7EB3805 
	'f_attack
	4DF0E92D 2400B08A F04F4680 910427E0 FF5AF7FF 90072300 3400E9CD 46262400 
	3D08F8D7 6FDA6A1B 33016893 F3C36093 B9230309 3D08F8D7 6A9B6A1B F8D74798 
	00E23D08 6A1B9904 330C6FDB 5000E9D1 EB016959 F8510EC4 F8DEC034 42811004 
	45ACBF08 6C59D14D F851440A 68521034 D12F430A 0107F014 0C07F004 2101BF18 
	E9D84328 D1172300 0509F1B4 30FFF146 BF084298 D1004295 1FE0B949 31FFF146 
	BF084299 D12C4290 0F07F1BC 2301D029 E9CD2400 E02B3400 F1461DE5 42980000 
	4295BF08 2900D101 F114D1F1 F1460009 E7E50100 25006C1B 02C1EB03 3031F853 
	3B0146AA 93086852 33FFF142 F8D79309 6A1B3D08 9B086FDA 9B0942AB 030AEB73 
	3401DA13 0600F146 F1762C40 DB870300 23E0F04F F8D39A07 6A1B3D08 0100E9DD 
	605A6FDB E8BDB00A 68938DF0 60933301 0309F3C3 F8D7B923 6A1B3D08 47986A9B 
	46A32330 9305436B 930600E3 3D08F8D7 6FDA6A1B 33016893 F3C36093 B9230309 
	3D08F8D7 6A9B6A1B F8D74798 9A063D08 6FD96A1B 310C6D0B E034F8D1 9A05589B 
	E9D19303 4413030E EA4F9A03 F8500CC2 9803203B 3030F853 EB0E4413 F85E02C3 
	6853B033 3FFFF1B3 F1BBBF08 D0153FFF 2000E9D8 BF084298 D113455A 24006C0B 
	26009A03 F853449C F8DC5032 3D013004 3AFFF143 E9CD2301 243F3400 F14A3501 
	E7840A00 EB03694B F85302CB 6852303B 43133B06 6E4BD1F2 449C9A03 3032F853 
	2004F8DC D1A1431A 0000E7E8 
	'f_in_check
	4DF0E92D 46062400 0A01F04F 27E0F04F ED2D46A0 ED9F8B02 B0848B2C FE4EF7FF 
	46054623 2D08F8D7 6A122C40 4300E9CD 0300F173 DB076FD2 46414650 B0046055 
	8B02ECBD 8DF0E8BD 33016893 F3C36093 B9230309 3D08F8D7 6A9B6A1B F8D74798 
	00E13D08 F8D36A1B F8DCC07C F10C2050 EB020E0C F8520BC4 F8DB2034 3A05B004 
	020BEA52 3000E9D6 F8DCD117 44112020 4034F852 4282684A 429CBF08 F083D10D 
	A9020301 5004F8CC 3002E9CD F7FF4668 4682FEA1 ED8D4688 9C008B00 34019B01 
	0300F143 BF00E7AE 0000003F 00000000 
	'f_gen_promote
	4DFFE92D 2AE0F04F 460E4607 F8DA4690 F44F3D08 F04F3480 6A1B0B00 F8D36FDB 
	930230A4 FDE2F7FF 90034B2F F8DA9300 6A1B3D08 68936FDA 60933301 0309F3C3 
	F8DAB923 6A1B3D08 47986A9B 3D08F8DA 6FDB6A1B 000CF103 9B029301 681A6881 
	32016900 5032F851 0CC2EB01 0E01F115 3004F8DC 0300F143 E032F841 EB009900 
	F8CC0EC5 E9D83004 061B2300 2312EA43 6839430B 6C02EA41 EA43687A E9D60102 
	021B2300 2C02EA4C 6312EA43 0C04EA4C 3480F514 0301EA43 5200F04C 030BEA43 
	0B00F14B 2035F840 3004F8CE 330A9B00 F5A49300 EA5323A0 D1A8030B 9A039B01 
	B004605A 8DF0E8BD 000F424A 
	'f_gen_push
	4DF3E92D 26E0F04F 46834692 F8D6460C 6A1B3D08 F8D36FDB F8D320A4 920170A8 
	FD6EF7FF 3000F8DA 9A014605 0F10F013 E9D7D01A E9D40700 43381300 2908D110 
	0300F173 4652DA10 46584621 FF62F7FF 3D08F8D6 6FDB6A1B B002605D 8DF0E8BD 
	F1732938 DAEE0300 23E0F04F F8D36812 32013D08 6FDF6A1B F1076979 F851080C 
	EB016032 F11600C2 68430C01 0300F143 C032F841 E9D46043 E9DA0200 02124300 
	EA42061B EA436210 E9DB2314 EA42CE00 EA4C020E 43132100 2014F8D8 6104EA41 
	4010F8D8 0AC0EB02 2030F852 0EC6EB04 A004F8DA EA523A06 D014020A 8044F8D8 
	2030F858 FBA2200A F8582000 F502003C F5022274 1A127210 F8444313 F8CE1036 
	607D3004 F8D8E7A9 EB022020 F8522240 E7F1203C 
	'f_takeback
	4DFFE92D 2AE0F04F 3D08F8DA 6FDB6A1B 208CF8D3 509CF8D3 F8D39201 F8D32094 
	920260B4 B429E9D3 8721E9D3 FCDEF7FF 90036823 0E00F04F 0301F083 68336023 
	0301F083 E9DB6033 3B013200 32FFF142 3200E9CB 3200E9D5 F1423B01 E9C532FF 
	F8DA3200 6A122D08 6A906FD2 0A0CF102 EB009200 F85002C3 F5001033 68526049 
	2307F3C1 6C11EA4F 01FFF001 1B03F3C2 2C02EA4C BE00E9C8 2807F3C2 38FFF118 
	3EFFF14E 0F20F01C 8E00E9C7 EA4F9F01 EA4F4E22 F00278E2 BF18020F 0B00F04F 
	E800E9C7 EB00682D EA4F00C5 ED1005C1 98027B02 7B00ED80 0014F8DA 7B00ED94 
	01C1EB00 0EC3EB00 7B00ED81 1044F8DA F04FBF15 EB010A00 186D07C3 BF08186D 
	AB00E9D7 EA4F2A06 E9C507C3 BF04AB00 0A06F04F 0B00F04F 05C3EB01 E9D6BF18 
	F01CAB00 E9CE0F02 F841AB00 F04F2033 606A0200 2B3ED022 2B3AD038 2B06D039 
	1E9AD03A 2503D13B 0EC2EB00 02C2EB01 AB00E9D4 AB00E9CE 0A03F04F 0B00F04F 
	AB00E9C2 02C5EB00 0A06F04F 0B00F04F 05C5EB01 AB00E9C2 AB00E9C5 0F04F01C 
	E9D4D00E E9D62400 4322AB00 3740D11B 23002200 440F4438 AB00E9C0 2300E9C7 
	9A039B00 B004605A 8DF0E8BD 253D223F 2238E7CA E7C7253B 25052207 F04FE7C4 
	461532FF 3B08E7C0 EB002200 EB0100C3 230001C3 AB00E9C0 2300E9C1 0000E7E0 
	'f_eval_
	23E0F04F 3D08F8D3 6FDB6A1B 4DF0E92D 30A8F8D3 9306B096 FBF8F7FF AA122300 
	46199007 F8023301 2B101B01 2600D1FA 27E0F04F 25002400 3D08F8D7 6FDA6A1B 
	33016893 F3C36093 B9230309 3D08F8D7 6A9B6A1B F8D74798 20073D08 6A1B2100 
	6DDA6FDB 4432330C 2EA03610 4500E9C2 0102E9C2 6D5AD1E0 26E0F04F E9C26C9B 
	E9C24500 E9C34502 E9C34500 24004502 F8D64625 6A1B3D08 68936FDA 60933301 
	0309F3C3 F8D6B923 6A1B3D08 47986A9B 3D08F8D6 A020F8D3 F8DA00E3 9300207C 
	E020F8D2 EB0E320C F85E0BC4 F8DB1034 1F883004 D0474318 00E36C50 C058F8D2 
	F85018C3 685F0034 430700CB 80CEF040 8048F8D2 F8586D12 44437031 0C00E9DC 
	685F1838 0031F848 070CEB47 F8DA00E1 605F8144 0307F004 1001F85E 0004F8DB 
	43013301 1703EA4F 1303EB02 8092F040 B007F852 F8D34620 2208A004 46292300 
	458347C0 0101EB7A F8D6DA0E 46203D08 6A1B4629 F8D36FDA 23008144 44176DD2 
	47C02208 0100E9C7 F1453401 F1A40500 432B0340 F04FD18D F8D323E0 6A1B3D08 
	6D5A6FDB E9D26E1D E9D54000 191B3100 0100EB41 0202E9D2 3112E9CD 3102E9D5 
	EB41181B E9CD0202 23003214 3300E9CD 21E0F04F 3D08F8D1 6FDA6A1B 33016893 
	F3C36093 B9230309 3D08F8D1 6A9B6A1B F04F4798 F8D121E0 6A1B3D08 68936FDA 
	60933301 0309F3C3 F8D1B923 6A1B3D08 47986A9B 23E0F04F 9E009A00 3D08F8D3 
	990000D4 6FC56A18 F1056A2A EB02030C F85201C1 684E7036 43321FBA 9B00D149 
	93003301 F1439B01 93010300 9A019B00 43133B40 9C06D1CD 68249D06 432C686D 
	0114E9DD 3212E9DD 83CEF000 EB611AC0 E3CC0102 BA02E9D3 23002208 46294620 
	455847C0 010AEB71 AF7EF6BF 3D08F8D6 46294620 6FDA6A1B 8144F8D3 6DD22300 
	22084417 E9C747C0 E76D0102 E054F8D2 08C0EB0C 0030F85C 2031F85E F8D84473 
	18127004 EB406858 F84E0707 605F2031 9A12E75A 9900433E 9A139202 9A149203 
	9A159204 6C5A9205 0C04EB02 2031F852 1004F8DC 81BAF040 F1712A06 D27B0100 
	D8792A05 F002E8DF 9E9C9203 F04FCA78 F7FF26E0 9005FA8B 3D08F8D6 6FDA6A1B 
	7144F8D3 6D922300 59141911 684D2208 0100E9DD F8D647B8 6A1B3D08 B07CF8D3 
	F8DB9B00 F003E05C 33010307 EB0E011A F85E1C03 F8DC6002 42B02004 0202EB71 
	3C0ADA02 0500F165 33011E5E EA4F0132 EB0E1803 F85E1606 F85E7002 EB0E8008 
	68721303 E004F8D3 EA479204 92080208 EA429A04 9A080A0E 0A0AEA52 3C14D13A 
	0500F165 7202E9D6 EC02E9DC EB7C45BE BFBC0602 46624677 418A4287 E9D3DB12 
	42822302 DB0D418B 0007F1D0 0214F04F 0141EB61 0302FBA0 FB021900 46043301 
	0505EB43 F8CB9B05 9B023004 9B0318E4 0503EB45 4512E9CD 33019B00 9B019300 
	0300F143 9B009301 3B409A01 F47F4313 E720AEDF 45479A04 0A0EEB72 4647BFBC 
	42874672 DABD418A E7B93C08 191A6B1B 591B9902 185B6852 EB419903 E0280202 
	E7F468EB 6D199A00 0207F002 EB013201 01141302 6859590A D10B430A 2102E9D3 
	9A021FD3 D117430B 9B03320F 0300F143 2312E9CD 4144F8D0 23002208 0100E9DD 
	380147A0 E9DD4308 D1B53212 F1423314 E9CD0200 E7AF3212 E7E6320A F2406D5A 
	E9D241B1 42880202 0200F172 6A9BDA01 F04FE7BD F7FF28E0 F04FF9B9 46070A00 
	3D08F8D8 6FDB6A1B 19116B9A 9A005914 F002684E 2D020507 2201D858 23006058 
	2500A810 2310E9CD F9C4F7FF 3D08F8D8 0B00EB14 0402F04F A80E6A1B 0A01EB46 
	605F6FDB 450EE9CD F9B4F7FF 3D08F8D8 0B00EB1B 0203F04F A80C6A1B 0A01EB4A 
	605F6FDB 3D08F8D8 F8D36A1B 23006144 230CE9CD F99EF7FF 462B4622 EB1B47B0 
	EB4A0400 F04F0601 F8D525E0 6A1B3D08 6E126FDA 0202E9D2 FC02FB04 421CF640 
	CC00FB06 6144F8D3 0104FBA0 44612300 F8D547B0 6A1B3D08 605F6FDB 18C09B02 
	EB439B03 E9CD0101 E7350112 D3362D05 60582208 A8102300 2310E9CD F96AF7FF 
	3D08F8D8 0A00EB14 0207F04F A80E6A1B 0501EB46 605F6FDB E9CD2300 F7FF230E 
	F8D8F959 EB1A3D08 F04F0A00 6A1B0206 EB45A80C 6FDB0501 F8D8605F 6A1B3D08 
	4144F8D3 E9CD2300 F7FF230C 2202F943 47A02300 0400EB1A 0601EB45 2D03E7A3 
	0300F04F 28E0F04F 2205BF0C 469B2206 F8D84610 6A1B3D08 68936FDA 60933301 
	0309F3C3 F8D8B933 90043D08 6A9B6A1B 98044798 3D08F8D8 6FDA6A1B 6DD1012B 
	1205EB01 685358C9 D1074319 3202E9D2 43133B07 3C0AD102 0600F166 F14A3501 
	42A80A00 030AEB7B E76CDAD3 F1712A06 F4BF0100 2A05AEC1 AEBEF63F F851A102 
	44112022 BF004708 00000019 00000127 00000143 0000014B FFFFFD71 000001A5 
	26E0F04F F8C2F7FF F8D69008 6A1A3D08 F8D26FD3 22087144 330C6999 6CDB5909 
	00C1EB03 4031F853 68452300 0100E9DD F8D647B8 6A1B3D08 6DDE6FDB 9B009302 
	0307F003 011A3301 1E03EB06 E9DE9209 42877202 DA02418A F1653C0A 1E5F0500 
	013A3301 1707EB06 E9D7920A 9203C202 F1AC011A EB060B07 920B1303 EA5B9A03 
	E9D30B02 D135A802 0B07F1AA 0B08EA5B 3C14D130 0500F165 F8DE9A09 F856E004 
	9A0AC002 8002F856 45E0687A 070EEB72 46E0BFBC 45404672 0202EB71 9A0BDB0F 
	58B2685B EB714290 DB080303 FBA02214 19000302 3301FB02 EB434604 9B020505 
	605A9A08 18E49B04 EB459B05 E9CD0503 E6314514 45E29A03 0B02EB78 46D4BFBC 
	45604642 0202EB71 3C08DAC6 68DAE7C2 59126B1B 01C2EB03 3032F853 9904684A 
	9905185B 0202EB41 68DBE02B 68EB591A 9A00E7F0 F0026D1D 32010207 1302EB05 
	E9D30114 3A072102 D10A430A 685B592A 9A04431A 320FD118 F1439B05 E9CD0300 
	F8D02314 22084144 E9DD2300 47A00100 43083806 3214E9DD ADEEF47F F1423314 
	E9CD0200 E5E73214 E7E5320A F2406D5A E9D241B1 42880200 0200F172 68DADA03 
	59126A9B F04FE7B6 F7FE28E0 4607FFEF 0A00F04F 3D08F8D8 6FDB6A1B 5911699A 
	EB026B9A F85200C1 9A004031 F0026846 2D020507 2201D858 2300605F 2500A810 
	2310E9CD F832F7FF 3D08F8D8 0B00EB14 0402F04F A80E6A1B 0A01EB46 605F6FDB 
	450EE9CD F822F7FF 3D08F8D8 0B00EB1B 0203F04F A80C6A1B 0A01EB4A 605F6FDB 
	3D08F8D8 F8D36A1B 23006144 230CE9CD F80CF7FF 462B4622 EB1B47B0 EB4A0400 
	F04F0601 F8D525E0 6A1B3D08 6E126FDA 0200E9D2 FC02FB04 421CF640 CC00FB06 
	6144F8D3 0104FBA0 44612300 F8D547B0 6A1B3D08 605F6FDB 18C09B04 EB439B05 
	E9CD0101 E5670114 D3362D05 605F2208 A8102300 2310E9CD FFD8F7FE 3D08F8D8 
	0A00EB14 0207F04F A80E6A1B 0501EB46 605F6FDB E9CD2300 F7FE230E F8D8FFC7 
	EB1A3D08 F04F0A00 6A1B0206 EB45A80C 6FDB0501 F8D8605F 6A1B3D08 4144F8D3 
	E9CD2300 F7FE230C 2202FFB1 47A02300 0400EB1A 0601EB45 2D03E7A3 0300F04F 
	28E0F04F 2205BF0C 469B2206 F8D84610 6A1B3D08 68936FDA 60933301 0309F3C3 
	F8D8B933 90023D08 6A9B6A1B 98024798 3D08F8D8 6FDA6A1B 6DD1012B 1205EB01 
	685358C9 D1074319 3202E9D2 43133B07 3C0AD102 0600F166 F14A3501 42A80A00 
	030AEB7B E76CDAD3 EB621A18 F04F0101 9A0723E0 3D08F8D3 6FDB6A1B B016605A 
	8DF0E8BD 
	'__aeabi_l2d
	F04FB508 F8D323E0 6A1B3D08 3084F8D3 EC514798 BD080B10 
	'searchK
	4DF0E92D 26E0F04F 460F4615 F8D62100 461C2D08 6FD26A12 8B06ED2D E9C2B0B6 
	F8D61100 9B442D08 68196A12 90206FD2 F10260D1 6899000C 69196041 69996081 
	6A1960C1 6A996101 6B196141 6B996181 6C1961C1 6C996201 6D196241 6D996281 
	6E1962C1 6E996301 6F196341 6F996381 F8D363C1 64011080 1088F8D3 F8D36441 
	64811090 1098F8D3 F8D364C1 650110A0 10A8F8D3 F8D36541 658110B0 10B8F8D3 
	F8D365C1 660110C0 10C8F8D3 B130F8D3 F8D36641 668110D0 10D8F8D3 F8D366C1 
	670110E0 10E8F8D3 F8D36741 F8C210F0 F8D31084 F8C210F8 F8D31088 F8C21100 
	911B108C 1108F8D3 1090F8C2 F8D3911C F8C21110 F8D31094 F8C21118 F8D31098 
	F8C21120 911D109C 1128F8D3 B0A4F8C2 10A0F8C2 F8D39110 F8C21138 912110A8 
	1140F8D3 10ACF8C2 F8D39115 F8D31148 F8C23150 F8C210B0 912230B4 FE58F7FE 
	9013683B 687B9323 682B9324 686B930A 6823930B 68639304 F8DB9305 93283000 
	3004F8DB 9B159329 9306681B 685B9B15 E9DD9307 43133206 F8D6D015 46023D08 
	6FDB6A1B 9B23605A 9B249301 9B209302 9A012000 601A2100 605A9A02 ECBDB036 
	E8BD8B06 9B068DF0 9B079303 9B069308 9B079311 9B069312 9B07931E 9B06931F 
	9B079316 9B239317 9B249301 9B52ED9F 8B53ED9F F0019302 F04FBABB F8D828E0 
	6A1B3D08 E9DB6FDB 6ED82100 04C2EB00 2032F840 E9DD6061 43200404 E9DBD136 
	2A1F2300 0300F173 9B1DDA0C 118FF240 2300E9D3 F173428A BFAC0300 24002401 
	8086F2C0 F9DEF7FF 0111E9CD 23002201 230CE9CD 22E0F04F 3D08F8D2 6FD96A1B 
	3301688B F3C3608B B9230309 3D08F8D2 6A9B6A1B E9DD4798 4313320C 87B4F000 
	93019B11 93029B12 BAC2F001 D0C64311 609CF8D3 508CF8D3 A094F8D3 FDB8F7FE 
	4600E9D6 F1461E61 930D33FF 3200E9D5 EB661AE4 93090302 5306E9DD F8D8930C 
	42A13D08 6A1B9E09 9B0D6FDA DA0841B3 60509B0C D0A2432B 93119B06 93129B07 
	6893E7B4 60933301 0309F3C3 F8D8B943 6A1B3D08 E9CD6A9B 4798010E 010EE9DD 
	3D08F8D8 6FDB6A1B F5036A9B EB036349 E9DA03C4 E9536700 42BB2302 42B2BF08 
	9B0CD104 F1433501 930C0300 34019B09 0300F143 E7C29309 00000000 00000000 
	00000006 00000000 00000001 00000000 3204E9DD F0004313 982183A3 FEFAF7FE 
	430B4603 0116E9CD 9B04D006 93043301 F1439B05 93050300 28E0F04F 2D08F8D8 
	6FD26A12 3084F8D2 A088F8D2 E9D29309 93034329 30B4F8D2 F7FE930D F8D8FD39 
	46052D08 6FD26A12 68226951 02C2EB01 0100E9D2 9B2AED8D 9B2CED8D 9B2EED8D 
	0102E9C2 462AE9DD 2D08F8D8 6A122C40 F1766FD7 F2C00200 9B0980FD 9B03681A 
	0100E9D3 4301607D 82C8F040 D51507D6 3D08F8D8 A932223C 6A1BA834 605D6FDB 
	E9CD2300 223E2334 E9CD2300 22022332 E9CD2300 AA302330 FF7AF7FE 22E0F04F 
	3D08F8D2 6FDB6A1B 9B09605D 079C681B F8D2D515 223C3D08 6FDB6A1B 2300605D 
	2334E9CD 2300223A 2332E9CD 23002202 A834A932 2330E9CD F7FEAA30 E9DAFF59 
	F1B13100 BF083FFF 3FFFF1B3 9803D05B 0207F013 4000E9D0 2201BF18 F0404320 
	330782AF 0100F141 312EE9CD 21E0F04F 1D08F8D1 6FC96A09 6A0C6D08 06C3EB00 
	07C3EB04 4033F854 3033F850 6870687F 43384323 D10A4318 2215B14A 604D2300 
	4651A82E 2334E9CD F7FEAA34 E9DAFF21 F1133100 F1410209 E9CD0100 F04F212E 
	F8D121E0 6A091D08 6D086FC9 EB006A0C EB0406C2 F85407C2 F8504032 687F2032 
	43226870 43104338 F003D10D 2B070307 604DD009 23002215 A82E4651 2334E9CD 
	F7FEAA34 F04FFEF5 F8D323E0 6A1B3D08 605D6FDB E9D39B1C 43132300 F04FD031 
	F8D424E0 6A1B3D08 F8D36FDB F8D37090 F7FEA0A4 2200FC5D 90032300 2300E9C7 
	3D08F8D4 6FDB6A1B F8DA6959 33013000 02C3EB01 6033F851 3E016853 AB8EED1F 
	38FFF143 4502E952 21E0F04F F8D142A6 6A1B3D08 EB786FDA F2800305 9B038542 
	F04F6053 F8DB23E0 F8D32000 6A1B3D08 695B6FDB 01C2EB03 3032F853 684B9303 
	9B069308 9B07930C 9B06930D 9B07931E E63F931F 320168BA F3C260BA B9220209 
	2D08F8D8 6A926A12 F8D84790 00E32D08 930E6A12 9B036FD0 F1006A02 EB02010C 
	F85207C4 E9D32034 687FC300 BF08429F F0404562 6C4A809C 19D700E7 2034F852 
	4317687F 8105F040 0C03EA5C 0707F004 8093F040 1FE7BB1F 21E0F04F 32FFF146 
	722EE9CD 2D08F8D1 6FD26A12 F8D16055 6A122D08 6A116FD2 00C7EB01 1037F851 
	39016840 D12A4301 21002011 AA346055 0134E9CD A82AA92E FE4AF7FE F1B4E01F 
	60450309 32FFF146 322EE9CD 2D08F8D8 6FD26A12 EB016A11 F85100C3 68401033 
	43013901 2011D109 60552100 E9CDAA34 A92E0134 F7FEA82A 2F07FE2B F1B4D1BB 
	F1460708 E9CD32FF F8D8722E 6A122D08 6A116FD2 00C7EB01 1037F851 39066840 
	D1344301 2010AB2E AF2A2100 AA346055 E9CD9308 46190134 F7FE4638 2C30FE09 
	0200F176 F1B4DB23 F1460110 9B0832FF 122EE9CD 2D08F8D8 6FD26A12 F8D86055 
	6A122D08 6A106FD2 0CC1EB00 1031F850 0004F8DC 43013906 2018D109 60552100 
	E9CDAA32 46190132 F7FE4638 3401FDE1 0600F146 462AE9CD BB1FE634 0709F114 
	21E0F04F 0200F146 722EE9CD 2D08F8D1 6FD26A12 F8D16055 6A122D08 6A116FD2 
	00C7EB01 1037F851 43086840 2011D128 60552100 E9CDAA34 A92E0134 F7FEA82A 
	E01DFDB7 60451DE3 0200F146 322EE9CD 2D08F8D8 6FD26A12 EB016A11 F85100C3 
	68401033 D1094308 21002011 AA346055 0134E9CD A82AA92E FD9AF7FE D1BD2F07 
	0708F114 0200F146 722EE9CD 2D08F8D8 6FD26A12 EB016A11 F85100C7 68401037 
	43013906 AB2ED1A3 21002010 6055AF2A 9308AA34 0134E9CD 46384619 FD78F7FE 
	F1762C10 DA920200 0110F114 0200F146 6C08E76D 01C2EB00 2032F850 1E536849 
	F1419314 931833FF ED9F9B06 9308AB93 930C9B07 9A089B14 9B184293 EB739A0C 
	F6FF0202 F8D8AF74 6A122D08 688A6FD1 608A3201 0209F3C2 F8D8B922 6A122D08 
	47906A92 9A082330 E9CD4353 930F462C 2D08F8D8 6FD16A12 3201688A F3C2608A 
	B9220209 2D08F8D8 6A926A12 F8D84790 9F0E2D08 6FD36A12 6C989A0F E040F8D3 
	6D1A4410 6C5F59D2 0032F850 F8579A2C 44022032 00C2EB0E 2032F85E F1B06840 
	BF083FFF 3FFFF1B2 202CE9CD 6A19D027 EB0100D0 F8510EC2 F8DE1032 3906E004 
	EA51605D D022010E 1D08F8D8 6A099F0D 6FCB687F 44086A19 2032F851 6840990D 
	42B86809 428ABF08 2001D109 AA342100 E9CD605D A92C0134 F7FEA82A 9B08FCE9 
	9B0C1C5F 0300F143 930C9708 AA34E782 A82AA92C AB34ED8D FCDAF7FE 2D08F8D8 
	6A129B0E 6D0A6FD1 6F0A58D0 01C0EB02 0030F852 4302684A E7E0D18A D5150750 
	3D08F8D8 A9322204 6A1BA834 605D6FDB E9CD2300 22062334 E9CD2300 22022332 
	E9CD2300 AA302330 FCB2F7FE 22E0F04F 3D08F8D2 6FDB6A1B 9B09605D 0719681B 
	AD4DF57F 3D08F8D2 6A1B2204 605D6FDB E9CD2300 22022334 E9CD2300 E5372332 
	F1413B09 E9CD31FF F04F312E F8D121E0 6A091D08 6A016FC8 04C3EB01 1033F851 
	39016864 6D014321 06C3EB01 1033F851 2401BF0C 68732400 0401F004 BF18430B 
	B1542400 2215B14A 60452300 A82E4651 2334E9CD F7FEAA34 E9DAFC6B 1FD93200 
	32FFF142 122EE9CD 22E0F04F 2D08F8D2 6FD06A12 EB026A02 F85204C1 68642031 
	43223A01 EB026D02 F85206C1 BF0C1031 24002401 F0046872 430A0401 2400BF18 
	F43F2C00 F003AD50 2B070307 AD4BF43F E53F6045 8000F3AF 00000000 00000000 
	FDB0F7FE 42989B0A EB719B0B F2800303 9B0182F9 2AE0F04F 9B024283 F8DA418B 
	6A1B3D08 E9D36FDB F8D34229 F8D38088 920830B4 BFB8930D 0101E9CD F998F7FE 
	23002200 E9CD4605 E9CD232E E9CD2330 F8DA2332 68223D08 6FDB6A1B EB03695B 
	E9D303C2 E9C30100 E9DD0102 2C40432E F1739303 DB650300 3100E9D8 3FFFF1B1 
	F1B3BF08 F43F3FFF 9808ACFE 0207F013 4000E9D0 2201BF18 F0404320 33078218 
	0100F141 3132E9CD 21E0F04F 1D08F8D1 6FC96A09 6A0C6D08 06C3EB00 07C3EB04 
	4033F854 3033F850 6870687F 43384323 D10A4318 2215B14A 604D2300 4641A832 
	2334E9CD F7FEAA34 E9D8FBC3 F1133100 F1410209 E9CD0100 F04F2132 F8D121E0 
	6A091D08 6D086FC9 EB006A0C EB0406C2 F85407C2 F8504032 687F2032 43226870 
	43104338 ACAFF47F 0307F003 F43F2B07 604DACAA 23002215 A8324641 2334E9CD 
	E49EAA34 3D08F8DA 6FDA6A1B 33016893 F3C36093 B9230309 3D08F8DA 6A9B6A1B 
	F8DA4798 6A1B3D08 00E36FDF 020CF107 9B08930E 0100E9D3 EB036953 F85306C4 
	68763034 BF08428E F0404283 6C5380FC 199E00E6 3034F853 431E6876 80FAF040 
	D1724301 0607F014 1FE3D124 F04F9A03 F14221E0 E9CD32FF F8D13232 6A122D08 
	60556FD2 2D08F8D1 6FD26A12 EB006A10 F85001C3 68493033 430B3B01 2300D12B 
	22116055 A82EA932 2334E9CD F7FEAA34 E020FB3F 0009F1B4 607D9B03 33FFF143 
	0332E9CD 3D08F8DA 6FDB6A1B EB026A1A F85201C0 68492030 430A3A01 2211D109 
	2300605D A82EA932 2334E9CD F7FEAA34 2E07FB1F 2C10D1B9 F1739B03 DA240300 
	0308F1B4 F04F9A03 F14221E0 E9CD32FF F8D13232 6A122D08 60556FD2 2D08F8D1 
	6FD26A12 EB006A10 F85001C3 68493033 430B3B06 2300D109 22106055 A82EA932 
	2334E9CD F7FEAA34 9B08FAF3 681B9A08 3B016852 D1764313 23E0F04F 0607F014 
	2D08F8D3 6FD26A12 F114D124 9A030309 21E0F04F 0200F142 3232E9CD 2D08F8D1 
	6FD26A12 F8D16055 6A122D08 6A116FD2 00C3EB01 1033F851 430B6843 2300D129 
	22116055 A82EA932 2334E9CD F7FEAA34 E01EFABF 1DE16055 3D08F8D3 6A1B9803 
	0000F140 6A1A6FDB 1032E9CD 00C1EB02 2031F852 43116841 2211D109 2300605D 
	A82EA932 2334E9CD F7FEAA34 2E07FAA1 2C30D1BB F1739B03 DB240300 0308F114 
	F04F9A03 F14221E0 E9CD0200 F8D13232 6A122D08 60556FD2 2D08F8D1 6FD26A12 
	EB006A10 F85001C3 68493033 430B3B06 2300D109 22106055 A82EA932 2334E9CD 
	F7FEAA34 3401FA75 F1439B03 E9CD0300 E66A432E F04F6C12 EB0227E0 F85201C3 
	684A3033 93143B01 33FFF142 9B049318 9B059309 9B14930C 42939A09 9A0C9B18 
	DBE04193 3D08F8D7 6FDA6A1B 33016893 F3C36093 B9230309 3D08F8D7 6A9B6A1B 
	9B034798 93319A09 94302330 930F4353 3D08F8D7 6FDA6A1B 33016893 F3C36093 
	B9230309 3D08F8D7 6A9B6A1B F8D74798 990E3D08 F8D36A1B F8DEE07C F10E2050 
	6B9E030C C001F852 990F6BDA 00CCEA4F 9A301851 103CF851 2032F856 440A6B5E 
	EB016B59 F85601C2 68492032 3FFFF1B1 F1B2BF08 E9CD3FFF D0282130 911900D1 
	EB016959 F85106C2 68761032 43313906 F8CED025 00D05004 3D08F8D7 6A1B9E0D 
	6FDB6876 44086A19 2032F851 6840990D 42B06809 428ABF08 2201D109 2300605D 
	A82EA930 2334E9CD F7FEAA34 9B09F9E1 9B0C1C5E 0300F143 930C9609 6E5BE77B 
	F8534418 6842303C D191431A 3B09E7EF 31FFF141 3132E9CD 21E0F04F 1D08F8D1 
	6FC86A09 EB016A01 F85104C3 68641033 43213901 EB016D01 F85106C3 BF0C1033 
	24002401 F0046873 430B0401 2400BF18 B14AB154 23002215 46416045 E9CDA832 
	AA342334 F9A4F7FE 3200E9D8 F1421FD9 E9CD32FF F04F1232 F8D222E0 6A122D08 
	6A026FD0 04C1EB02 2031F852 3A016864 6D024322 06C1EB02 1031F852 2401BF0C 
	68722400 0401F004 BF18430A 2C002400 AA89F43F 0307F003 F43F2B07 6045AA84 
	6893E5D8 60933301 0309F3C3 F8D1B923 6A1B3D08 47986A9B 23E0F04F 1000F8DA 
	3D08F8D3 6FDB6A1B 330C69DA C05CF8D3 E034F852 00C4EB02 7284F44F EB0C434A 
	F85C0102 68492002 BF082900 D1144596 ED87689B 6842AB00 0218F502 4216F502 
	60423280 2000F8DA EB033201 F85301C2 684D4032 F1453C01 340135FF 0500F145 
	BA72F7FF 93119B0A 93129B0B E9CD2301 F7FF340C 9B0AB8CF 9B0B9301 23019302 
	2300930C 9B01930D 9B029311 23019312 0501E9CD 2300931E BA7AF7FF 33016893 
	F3C36093 B9230309 3D08F8D4 6A9B6A1B F04F4798 F04F24E0 F8D436FF 46B23D08 
	6FDB6A1B 50A4F8D3 FE82F7FD 3D08F8D4 6A1B682A F8DD3201 6FDB800C 9F084645 
	EB03695B F85301C2 68492032 F1413A01 930933FF 3D08F8D4 6A1B42AA 9B096FD9 
	DA7D41BB 9A0369CB 02C2EB03 03C8EB03 7B00ED93 6700E9D2 7B00ED82 6700E9C3 
	F8D46048 9A133D08 99036A1B 605A6FDB 3D08F8D4 6FDB6A1B F8D369DA F852809C 
	F8D32031 920E70A8 60B4F8D3 A221E9D3 F8D3921A 920F208C 2094F8D3 F8D39225 
	921420A4 FE34F7FD 90099B0E 03FFF003 9B0E9318 2507F3C3 93190E1B 019B9B0E 
	80B0F140 F7FD4638 F8D4FFC5 43013D08 D05A6A1B 9A096FDB 9B03605A 93033301 
	F1439B08 93080300 24E0F04F 3D08F8D4 6FDA6A1B 3500E9DB 1C586951 06C0EB01 
	0030F851 9E036871 98084286 0101EB70 AF64F6FF 2104E9DD F000430A E9DD82F9 
	430A211E 82EDF040 2116E9DD F000430A 4AB98301 9301189B 0300F165 B823F7FF 
	3301688B F3C3608B B9430309 3D08F8D4 6A9B6A1B 020EE9CD E9DD4798 F8D4020E 
	6A1B3D08 69DB6FDB 03C5EB03 17D9685B EB7A429E BFA60C01 46514633 350146A8 
	468A461E 0700F147 6FD8E754 6A012D3E 810DF040 327AE9D1 43133B06 E9D1D104 
	3B06327C D0074313 60439B09 23E0F04F 3D08F8D3 E78D6A1B 223D2300 A8344631 
	2334E9CD FE40F7FD 9026460C 203E2100 0132E9CD A8324631 FE36F7FD 43219B26 
	43014318 233DD1E2 F04F203F F8D222E0 6A122D08 6A116FD2 EB01320C 6C5204C0 
	01C3EB01 00C0EB02 02C3EB02 7B00ED94 7B00ED81 7B00ED90 7B00ED82 8B00ED84 
	8B00ED80 23E0F04F F8DA00EA F8D30000 92263D08 F8D86A1B 6FDCC000 340C6D23 
	EB0369E1 EB0102C5 92270ECC 68129A0F EA420412 9A1A1000 32016812 2202EA40 
	0035F853 980E4302 003CF841 6149F501 2004F8CE 98149A25 7B00ED92 2000F8D8 
	01C2EB01 ED019A14 99147B02 68496812 F1413201 60020100 E9D86041 32012100 
	0100F141 2100E9C8 E004F8D4 99189A18 08C2EA4F 1031F85E 02C2EB0E 68514608 
	2000F8DA F8DA4002 EA010004 EB0E0C00 F85E01C5 6849E035 020EEA02 0101EA0C 
	2100E9CA 07109A19 80FDF140 1200E9D7 F040430A F10580F3 21000208 6002981A 
	60419A19 0F11F012 80F1F040 990F9A0F 68496812 F1413201 980F0100 60416964 
	60029919 068900EA 0004EB08 BF454422 F04F990E 99180C00 4107F3C1 7B00ED97 
	7B00ED82 0208EB03 F853BF5C F8D21031 F843C004 99271035 C004F8C1 8B00ED80 
	8B00ED82 07529A19 E9D7D50C 430A1200 80C0F040 32409A26 441A4414 8B00ED84 
	8B00ED82 4630683B 0301F083 6833603B 0301F083 F04F6033 F7FD26E0 4301FE5B 
	80B2F000 FFBEF7FD 3D08F8D6 2D3AE702 E9D1D134 3B063272 E9D14313 BF143274 
	24002401 43133B06 F044BF18 2C000401 AEEAF47F 3276E9D1 43133B06 AEE4F47F 
	223A2300 A8344631 2334E9CD FD2CF7FD 9026460C 203B2100 0132E9CD A8324631 
	FD22F7FD 43219B26 43014318 AECEF47F 2038233B BF00E6E9 FFFFD8F0 D1262D06 
	320AE9D1 43133B06 AEBEF47F 320CE9D1 43133B06 AEB8F47F 22052300 A8344631 
	2334E9CD FD00F7FD 9026460C 20062100 0132E9CD A8324631 FCF6F7FD 43219B26 
	43014318 AEA2F47F 20072305 2D02E6BD E9D1D130 3B063202 E9D14313 BF143204 
	24002401 43133B06 F044BF18 2C000401 AE8AF47F 3206E9D1 43133B06 AE84F47F 
	22022300 A8344631 2334E9CD FCCCF7FD 9026460C 20032100 0132E9CD A8324631 
	FCC2F7FD 43219B26 43014318 AE6EF47F E68A2303 33FFF04F E6864618 0208F1B5 
	0101EB61 F04FE70A 461132FF E9DDE706 E712210C EB043D08 EB0304C5 ED8403C5 
	ED838B00 E73D8B00 3D08F8D6 6A1B2500 E9D36FDB F8D34725 F8D3A088 F7FD80A8 
	900CFBF7 9B00ED84 3D08F8D6 6FDA6A1B 33016893 F3C36093 B9230309 3D08F8D6 
	6A9B6A1B F8D64798 6A1B3D08 C07CF8D3 3020F8DC 000CF10C 595B195A 1F996852 
	D012430A 210C6A42 2E05FB01 594A6C41 EB036821 EB0E0342 F85E02C3 68523033 
	68634059 E9C4405A 35081200 7F00F5B5 E9D8D1CA 3B013200 D1074313 3200E9D4 
	1500E9D7 4055404B 3500E9C4 1300E9DA 3FFFF1B3 F1B1BF08 D00B3FFF EB006980 
	F85005C1 E9D41031 68682300 40584051 1000E9C4 22E0F04F 99099B0C 3004F8CC 
	3D08F8D2 6A1B9C01 60596FDB 3D08F8D2 1000F8DB 39016A1B 6F5A6FDB F842330C 
	EB024031 9A0200C1 60429C0A 1000F8DB 39016EDA 4031F842 00C1EB02 9C049A0B 
	F8DB6042 6F1A1000 EB023901 F84200C1 9A054031 60429905 430A4622 F8DBD159 
	6F5B2000 98033A01 01C2EB03 0032F843 604B9B08 99019B0A 9B0B425A 0343EB63 
	910A4249 EB619902 E9CD0141 910B2301 22E0F04F 3D08F8D2 6FD96A1B 3301688B 
	F3C3608B B9230309 3D08F8D2 6A9B6A1B 9B104798 681B9A10 33016852 F1429910 
	065F0200 3200E9C1 AD27F47E 24E0F04F 3D08F8D4 F8D36A1B 47983198 EEB09B22 
	E9D3AB40 F7FE0100 EC41FC1F EEB40B17 EEF1ABC7 F77EFA10 2201AD10 99152300 
	2300E9C1 3D08F8D4 6A1B9A13 F7FE6FDB 9A04BCD9 92043A01 F1629A05 92050200 
	9B1BE79D 2300E9D3 F1732A64 DA160300 24E0F04F 2300E9DB 1028E9DD BF084298 
	D1114291 3D08F8D4 6A1B9A13 605A6FDB BCBDF7FE 93019B16 F7FE9B17 9B0CBD24 
	9B0D9301 BD1FF7FE FDECF7FD F8DB9B01 425F1000 EB639B02 F8D40243 6A1B3D08 
	6F586FDB 6EDE330C 04C1EB00 0031F850 EB066865 F85604C1 42B86031 960A6864 
	940B6F1E 04C1EB06 6031F856 96046864 94056F5E 04C1EB06 1031F856 68619103 
	EB759108 DA5E0102 1404E9DD D01C4321 9C036919 6034F851 F3C66A19 F0062C07 
	EB0106FF EB0C2C4C F85C0EC6 F8DE1036 94094004 EB119C04 9C050804 F84C9909 
	EB448036 F8CE0401 990A4004 990B428F 0101EB72 ABDFF6BF 4000F8DB 7884F44F 
	F04F6DD9 69182AE0 FB089E03 F8501104 EB010036 F84105C4 21000034 60696E18 
	4500E9DB EB001C61 F85003C1 685B6031 F1433E01 340133FF 0500F145 F8DA9309 
	42A63D08 6FD96A1B 41AB9B09 F8DBDA17 46153000 33016EC9 03C3EB01 0100E9D3 
	0102E943 9B034638 93033301 F1439B08 93080300 930C9B06 F7FF9B07 688BBBA3 
	608B3301 0309F3C3 F8DAB933 920C3D08 6A9B6A1B 9A0C4798 3D08F8DA 1000F8DB 
	6FDB6A1B FB086E9B 34013304 00C1EB03 0500F145 7B02ED90 7B00ED80 BF00E7BF 
End CSUB
