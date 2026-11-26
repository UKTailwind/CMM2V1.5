' affinen.bas - FRAMEBUFFER and PAGE AFFINE and SPAN in the CMM2's own modes, checked with PIXEL() against the pixel
' model of the PicoMite's affine_test.bas (FRAMEBUFFER_AFFINE_SPAN_Design.md sections 4 and 8), with the CMM2's
' additions: the 8, 12, 16 and 32-bit depths, pages whose rows are stored twice, and a source and destination of
' different sizes (the world framebuffer F). The source holds (x - y) MOD 8 as diagonals of eight colours, the
' destination (x + y + 3) MOD 8. Each configuration: a mode, and P (PAGE form, page 1 TO 0), F (FRAMEBUFFER F, N, F the
' screen's size), W (F, N with F a 1.25x world) or R (N, F: the world the destination). Half the commands end in empty
' arguments. A mode the board does not have (32 bits on a G1) is passed over; a configuration is printed only when
' something is wrong, so that every board prints the same.
OPTION EXPLICIT
OPTION DEFAULT INTEGER
CONST NCASES = 50, NSAMP = 50, FP = 65536
DIM cf$(13) = ("1,8 P", "1,16 P", "1,12 P", "1,32 P", "7,8 P", "7,16 P", "7,12 P", "3,8 P", "3,16 P", "7,4 P", "7,8 F", "7,16 W", "7,8 R", "7,12 W")
DIM pal(7), rb(7)
pal(0) = RGB(0,0,0) : pal(1) = RGB(255,0,0) : pal(2) = RGB(0,255,0) : pal(3) = RGB(0,0,255)
pal(4) = RGB(255,255,0) : pal(5) = RGB(0,255,255) : pal(6) = RGB(255,0,255) : pal(7) = RGB(255,255,255)
DIM ci, md$, fm$, depth, SW, SH, DW, DH, k, c, cs, s, x, y, ex, exc, got, fails, shown, nsam, allfails, ok
DIM aff, stp, fst, n, edg, trn, ev, tv, lft, wid, outs, same, kind
DIM au(0), av(0), adu(0), adv(0), sl(0), sr(0)
RANDOMIZE 2026
FOR ci = 0 TO 13
  md$ = FIELD$(cf$(ci), 1, " ") : fm$ = FIELD$(cf$(ci), 2, " ")
  depth = VAL(FIELD$(md$, 2, ","))
  ON ERROR SKIP 1
  EXECUTE "MODE " + md$
  ok = (MM.ERRNO = 0)
  IF ok THEN
    DW = MM.HRES : DH = MM.VRES : SW = DW : SH = DH
    IF depth = 12 THEN pal(0) = RGB(0, 0, 0, 0) ELSE pal(0) = RGB(0, 0, 0)   ' (12-bit: colour 0 of alpha 0)
    ' the source and the destination's background
    IF fm$ = "P" THEN
      PAGE WRITE 1 : source DW, DH : PAGE WRITE 2 : backdrop DW, DH
    ELSEIF fm$ = "F" OR fm$ = "W" THEN
      IF fm$ = "W" THEN SW = DW * 5 \ 4 : SH = DH * 5 \ 4
      FRAMEBUFFER CREATE SW, SH : FRAMEBUFFER WRITE F : source SW, SH
      PAGE WRITE 1 : backdrop DW, DH
    ELSE
      DW = SW * 5 \ 4 : DH = SH * 5 \ 4
      FRAMEBUFFER CREATE DW, DH : FRAMEBUFFER WRITE F : backdrop DW, DH : FRAMEBUFFER BACKUP
      PAGE WRITE 0 : source SW, SH
    ENDIF
    destpage
    FOR k = 0 TO 7 : PIXEL 0, 0, pal(k) : rb(k) = PIXEL(0, 0) : NEXT
    fails = 0 : shown = 0 : nsam = 0
    FOR cs = 1 TO NCASES
      aff = (RND < 0.6)
      stp = 1 : IF RND < 0.6 THEN stp = ri(1, MIN(DH, 40))
      n = ri(1, MIN(DH \ stp + 2, 60))
      fst = 0 : IF RND < 0.6 THEN fst = ri(-(n * stp) \ 2, DH - 1)
      edg = -2 : IF RND < 0.5 THEN edg = ri(-1, 7)
      trn = -1 : IF RND < 0.3 THEN trn = ri(0, 7)
      ev = -1 : IF edg >= 0 THEN ev = pal(edg)
      IF trn >= 0 THEN tv = tval(pal(trn))
      IF trn >= 0 AND depth = 12 THEN trn = 0 : tv = RGB(255, 0, 0)   ' (12-bit: alpha 0, whatever is given)
      same = 0
      IF aff THEN
        ERASE au, av, adu, adv : DIM au(n - 1), av(n - 1), adu(n - 1), adv(n - 1)
        kind = ri(0, 5)
        FOR k = 0 TO n - 1 : setval k : NEXT
        lft = 0 : IF RND < 0.5 THEN lft = ri(-DW \ 2, DW - 1)
        wid = DW : IF RND < 0.5 THEN wid = ri(1, DW + 20)
      ELSE
        ERASE sl, sr : DIM sl(n - 1), sr(n - 1)
        FOR k = 0 TO n - 1
          sl(k) = ri(-10, DW + 10) : sr(k) = ri(-10, DW + 10)
          IF RND < 0.1 THEN sl(k) = ri(-(2^40), 2^40)
          IF RND < 0.1 THEN sr(k) = ri(-(2^40), 2^40)
        NEXT
        outs = (RND < 0.5)
        same = (RND < 0.2)
      ENDIF
      restorebg
      IF aff THEN doaffine ELSE dospan
      destpage
      FOR s = 1 TO NSAMP
        x = ri(0, DW - 1) : y = ri(0, DH - 1)
        IF RND < 0.6 AND fst + n * stp > 0 THEN y = ri(MAX(fst, 0), MIN(fst + n * stp, DH) - 1)
        IF aff THEN ex = amodel(x, y) ELSE ex = smodel(x, y)
        IF ex < 0 THEN exc = rb(pm(x + y + 3, 8)) ELSE exc = rb(ex)
        got = PIXEL(x, y)
        nsam = nsam + 1
        IF got <> exc THEN
          fails = fails + 1
          IF shown < 6 THEN
            shown = shown + 1
            IF aff THEN
              PRINT "  case"; cs; " AFFINE kind"; kind; " n"; n; " step"; stp; " first"; fst; " left"; lft; " width"; wid;
            ELSE
              PRINT "  case"; cs; " SPAN n"; n; " step"; stp; " first"; fst; " outside"; outs; " same"; same;
            ENDIF
            PRINT " edge"; edg; " trans"; trn; " at"; x; ","; y; " want"; ex; " got &H"; HEX$(got)
          ENDIF
        ENDIF
      NEXT
    NEXT
    IF fails THEN PRINT cf$(ci); " (src "; STR$(SW); "x"; STR$(SH); ", dst "; STR$(DW); "x"; STR$(DH); "):"; NCASES; " cases,"; nsam; " samples,"; fails; " wrong"
    allfails = allfails + fails
    PAGE WRITE 0
    IF fm$ <> "P" THEN FRAMEBUFFER CLOSE
  ENDIF
NEXT

' a quarter turn must be exact, and so must a window on a larger world: MODE 7,8 with a 400x300 framebuffer
MODE 7, 8
DW = MM.HRES : DH = MM.VRES : SW = 400 : SH = 300
FRAMEBUFFER CREATE SW, SH : FRAMEBUFFER WRITE F : source SW, SH
PAGE WRITE 0 : CLS
FOR k = 0 TO 7 : PIXEL 0, 0, pal(k) : rb(k) = PIXEL(0, 0) : NEXT
DIM qu(0), qv(0), qdu(0), qdv(0)
' N's (x, y) shows F's (y, DH - 1 - x): a quarter turn of the left DH x DH of F
qdu(0) = 0 : qdv(0) = -FP : qu(0) = 0 : qv(0) = (DH - 1) * FP
FRAMEBUFFER AFFINE F, N, qu(), qv(), qdu(), qdv(), DH, 0, 0, DH
fails = 0
FOR y = 0 TO DH - 1 STEP 3
  FOR x = 0 TO DH - 1 STEP 3
    IF PIXEL(x, y) <> rb(pm(y - (DH - 1 - x), 8)) THEN fails = fails + 1
  NEXT
NEXT
PRINT "quarter turn:"; fails; " wrong"
allfails = allfails + fails
' N shows the world from (61, 47), as FRAMEBUFFER WINDOW 61, 47, 0 would: the last column and row are the world's own
qdu(0) = FP : qdv(0) = 0 : qu(0) = 61 * FP : qv(0) = 47 * FP
FRAMEBUFFER AFFINE F, N, qu(), qv(), qdu(), qdv(), DH
fails = 0
FOR y = 0 TO DH - 1 STEP 3
  FOR x = 0 TO DW - 1 STEP 3
    IF PIXEL(x, y) <> rb(pm(x + 61 - y - 47, 8)) THEN fails = fails + 1
  NEXT
NEXT
IF PIXEL(DW - 1, DH - 1) <> rb(pm(DW + 60 - DH - 46, 8)) THEN fails = fails + 1
PRINT "window on the world:"; fails; " wrong"
allfails = allfails + fails
FRAMEBUFFER CLOSE

' the errors (section 10), in MODE 7,8 (320x240)
DIM e1(9), e9(8), f1!(9)
errtest 1, "Framebuffer not created"
FRAMEBUFFER CREATE 400, 300
errtest 2, "Same framebuffer"
errtest 3, "Same page"
errtest 4, "*"
errtest 5, "Array 2 has 9 elements, array 1 has 10"
errtest 6, "Array 4 has 9 elements, array 1 has 10"
errtest 7, "*"
errtest 8, "*"
errtest 9, "*"
errtest 10, "*"
errtest 11, "Right array has 9 elements, left array has 10"
errtest 12, "*"
errtest 13, "*"
errtest 14, "*"
errtest 15, ""
errtest 16, "*"
errtest 17, ""
errtest 18, ""
errtest 19, "*"
errtest 20, "*"
FRAMEBUFFER CLOSE
MODE 1, 1
errtest 21, "Invalid for this screen mode"
errtest 22, "Invalid for this screen mode"
MODE 1, 8
IF allfails = 0 THEN PRINT "AFFINE AND SPAN TEST PASSED" ELSE PRINT "AFFINE AND SPAN TEST FAILED:"; allfails
END

SUB errtest t, want$
  SELECT CASE t
    CASE 1
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e1(), e1(), e1()
    CASE 2
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, F, e1(), e1(), e1(), e1()
    CASE 3
      ON ERROR SKIP 1 : PAGE AFFINE 1 TO 1, e1(), e1(), e1(), e1()
    CASE 4
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE L, N, e1(), e1(), e1(), e1()
    CASE 5
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e9(), e1(), e1()
    CASE 6
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e1(), e1(), e9()
    CASE 7
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e1(), f1!(), e1()
    CASE 8
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e1(), e1(), e1(), 0
    CASE 9
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e1(), e1(), e1(), 1, 0, 0, 0
    CASE 10
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e1(), e1()
    CASE 11
      ON ERROR SKIP 1 : FRAMEBUFFER SPAN F, N, e1(), e9()
    CASE 12
      ON ERROR SKIP 1 : FRAMEBUFFER SPAN F, N, e1(), e1(), 1, 0, -1, , 2
    CASE 13
      ON ERROR SKIP 1 : FRAMEBUFFER SPAN F, N, e1(), e1(), 0
    CASE 14
      ON ERROR SKIP 1 : FRAMEBUFFER SPAN F, N, e1(), e1(), 1, 0, -1, 256
    CASE 15
      ON ERROR SKIP 1 : PAGE SPAN 1, 0, e1(), e1()
    CASE 16
      ON ERROR SKIP 1 : PAGE AFFINE 1 TO 0 TO e1(), e1(), e1(), e1()
    CASE 17
      ON ERROR SKIP 1 : FRAMEBUFFER SPAN F, F, e1(), e1(), 1, 0, 0
    CASE 18
      ON ERROR SKIP 1 : PAGE SPAN 1 TO 1, e1(), e1()
    CASE 19
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE F, N, e1(), e1(), e1(), e1(), 1, 16777217
    CASE 20
      ON ERROR SKIP 1 : FRAMEBUFFER SPAN F, N, e1(), f1!()
    CASE 21
      ON ERROR SKIP 1 : FRAMEBUFFER AFFINE N, F, e1(), e1(), e1(), e1()
    CASE 22
      ON ERROR SKIP 1 : PAGE SPAN 1 TO 0, e1(), e1()
  END SELECT
  IF MM.ERRNO = 0 AND want$ = "" THEN
    PRINT "error test"; t; ": no error, as wanted"
  ELSEIF MM.ERRNO = 0 THEN
    PRINT "error test"; t; ": no error, wanted "; want$ : allfails = allfails + 1
  ELSEIF want$ = "*" OR MM.ERRMSG$ = want$ THEN
    PRINT "error test"; t; ": "; MM.ERRMSG$
  ELSE
    PRINT "error test"; t; ": got "; MM.ERRMSG$; " wanted "; want$ : allfails = allfails + 1
  ENDIF
END SUB

' the source's diagonals over w x h of the page being written
SUB source w, h
  LOCAL c
  CLS
  FOR c = -(h - 1) TO w - 1 : LINE c, 0, c + h - 1, h - 1, 1, pal(pm(c, 8)) : NEXT
END SUB

' the destination's background, the other way
SUB backdrop w, h
  LOCAL c
  CLS
  FOR c = 0 TO w + h - 2 : LINE c, 0, c - h + 1, h - 1, 1, pal(pm(c + 3, 8)) : NEXT
END SUB

' the page that is the destination, written (so PIXEL reads it)
SUB destpage
  IF fm$ = "R" THEN
    FRAMEBUFFER WRITE F
  ELSE
    PAGE WRITE 0
  ENDIF
END SUB

' the destination's background put back before a command
SUB restorebg
  IF fm$ = "P" THEN PAGE COPY 2 TO 0
  IF fm$ = "F" OR fm$ = "W" THEN PAGE COPY 1 TO 0
  IF fm$ = "R" THEN
    PAGE WRITE 0 : FRAMEBUFFER RESTORE
  ENDIF
END SUB

' element k of the four AFFINE arrays, by the case's kind (the source is SW x SH)
SUB setval k
  SELECT CASE kind
    CASE 0  ' the picture as it is, moved
      adu(k) = FP : adv(k) = 0 : au(k) = ri(-SW, SW) * FP : av(k) = ri(-SH, SH) * FP
    CASE 1  ' any small step
      adu(k) = ri(-3 * FP, 3 * FP) : adv(k) = ri(-3 * FP, 3 * FP)
      au(k) = ri(-4 * SW * FP, 4 * SW * FP) : av(k) = ri(-4 * SH * FP, 4 * SH * FP)
    CASE 2  ' a quarter turn
      adu(k) = 0 : adv(k) = FP * (2 * ri(0, 1) - 1) : au(k) = ri(-SW, SW) * FP : av(k) = ri(-SH, SH) * FP
    CASE 3  ' far away, with long steps
      adu(k) = ri(-(2^30), 2^30) : adv(k) = ri(-(2^30), 2^30) : au(k) = ri(-(2^44), 2^44) : av(k) = ri(-(2^44), 2^44)
    CASE 4  ' steps of a few thousandths of a pixel
      adu(k) = ri(-300, 300) : adv(k) = ri(-300, 300) : au(k) = ri(0, SW * FP) : av(k) = ri(0, SH * FP)
    CASE 5  ' a mirrored row
      adu(k) = -FP : adv(k) = 0 : au(k) = ri(-SW, 2 * SW) * FP + FP \ 2 : av(k) = ri(-SH, 2 * SH) * FP
  END SELECT
END SUB

' the buffers of a command, as its form writes them (@ for the command's word): one for a SPAN on the destination alone
FUNCTION bufs$(one)
  IF fm$ = "P" THEN
    bufs$ = CHOICE(one, " 0 TO 0", " 1 TO 0")
  ELSEIF fm$ = "R" THEN
    bufs$ = CHOICE(one, " F, F", " N, F")
  ELSE
    bufs$ = CHOICE(one, " N, N", " F, N")
  ENDIF
  IF fm$ = "P" THEN bufs$ = "PAGE @" + bufs$ ELSE bufs$ = "FRAMEBUFFER @" + bufs$
END FUNCTION

' the command run: its optional arguments a$, with the empty ones at the end left off half the time
SUB runcmd word$, one, arrays$, a$
  LOCAL c$, i
  c$ = bufs$(one)
  i = INSTR(c$, "@")
  c$ = LEFT$(c$, i - 1) + word$ + MID$(c$, i + 1) + arrays$
  IF RND < 0.5 THEN
    DO WHILE RIGHT$(a$, 1) = "," OR RIGHT$(a$, 1) = " " : a$ = LEFT$(a$, LEN(a$) - 1) : LOOP
  ENDIF
  EXECUTE c$ + a$
END SUB

SUB doaffine
  LOCAL a$
  a$ = ", " + STR$(stp) + ", " + STR$(fst) + ", " + STR$(lft) + ", " + STR$(wid) + ", "
  IF edg >= -1 THEN a$ = a$ + STR$(ev)
  a$ = a$ + ", "
  IF trn >= 0 THEN a$ = a$ + STR$(tv)
  IF edg = -2 AND trn < 0 AND lft = 0 AND wid = DW AND stp = 1 AND fst = 0 AND RND < 0.5 THEN a$ = ""
  runcmd "AFFINE", 0, ", au(), av(), adu(), adv()", a$
END SUB

' SPAN's fill is the case's edge: none for -2 (the argument left out) and for -1
SUB dospan
  LOCAL a$
  a$ = ", " + STR$(stp) + ", " + STR$(fst) + ", "
  IF edg >= -1 THEN a$ = a$ + STR$(ev)
  a$ = a$ + ", "
  IF trn >= 0 THEN a$ = a$ + STR$(tv)
  a$ = a$ + ", "
  IF outs OR RND < 0.5 THEN a$ = a$ + STR$(outs)
  runcmd "SPAN", same, ", sl(), sr()", a$
END SUB

' what PIXEL should read at (px, py) of the destination after AFFINE: a palette index, or -1 for its own
' (x + y + 3) MOD 8
FUNCTION amodel(px, py)
  LOCAL b, j, i, uu, vv, sx, sy
  amodel = -1
  IF py < fst OR py >= fst + n * stp THEN EXIT FUNCTION
  i = px - lft
  IF i < 0 OR i >= wid THEN EXIT FUNCTION
  b = (py - fst) \ stp : j = py - fst - b * stp
  uu = au(b) - j * adv(b) + i * adu(b) : vv = av(b) + j * adu(b) + i * adv(b)
  sx = fl(uu) : sy = fl(vv)
  IF edg = -2 THEN
    sx = pm(sx, SW) : sy = pm(sy, SH)
  ELSEIF sx < 0 OR sx >= SW OR sy < 0 OR sy >= SH THEN
    IF edg >= 0 THEN amodel = edg
    EXIT FUNCTION
  ENDIF
  amodel = pm(sx - sy, 8)
  IF amodel = trn THEN amodel = -1
END FUNCTION

' and after SPAN: a place that the source does not have is left as it is
FUNCTION smodel(px, py)
  LOCAL b, inside
  smodel = -1
  IF py < fst OR py >= fst + n * stp THEN EXIT FUNCTION
  b = (py - fst) \ stp
  inside = (sl(b) <= px AND px <= sr(b))
  IF inside <> outs THEN
    IF same = 0 AND px < SW AND py < SH THEN
      smodel = pm(px - py, 8)
      IF smodel = trn THEN smodel = -1
    ENDIF
  ELSEIF edg >= 0 THEN
    smodel = edg
  ENDIF
END FUNCTION

' a / 65536 rounded down
FUNCTION fl(a)
  IF a >= 0 THEN fl = a \ FP ELSE fl = -((FP - 1 - a) \ FP)
END FUNCTION

FUNCTION pm(a, m)
  LOCAL r
  r = a MOD m
  IF r < 0 THEN r = r + m
  pm = r
END FUNCTION

FUNCTION ri(lo, hi)
  ri = lo + INT(RND * (hi - lo + 1))
END FUNCTION

' the transparent value as the commands take it in this depth: a 4 or 8-bit pixel value, else the colour
FUNCTION tval(cv)
  IF depth = 4 THEN
    tval = ((cv >> 20) AND 8) OR ((cv >> 13) AND 6) OR ((cv >> 7) AND 1)
  ELSEIF depth = 8 THEN
    tval = ((cv >> 16) AND &HE0) OR ((cv >> 11) AND &H1C) OR ((cv >> 6) AND 3)
  ELSE
    tval = cv
  ENDIF
END FUNCTION
