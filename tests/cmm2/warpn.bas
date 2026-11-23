' warpn.bas - FRAMEBUFFER and PAGE WARPH and WARPV in the CMM2's own modes, checked with PIXEL() against the pixel
' model of the PicoMite's warp_test.bas (FRAMEBUFFER_WARP_Design.md sections 4-6), with the CMM2's additions: the 8,
' 12, 16 and 32-bit depths, pages whose rows are stored twice, and a source and destination of different sizes (the
' world framebuffer F). The source holds (x - y) MOD 8 as diagonals of eight colours, the destination (x + y + 3) MOD 8.
' Each configuration: a mode, and P (PAGE form, page 1 TO 0), F (FRAMEBUFFER F, N, F the screen's size), W (F, N with
' F a 1.25x world) or R (N, F: the world the destination). Half the commands end in empty arguments. A mode the board
' does not have (32 bits on a G1) is passed over; a configuration is printed only when something is wrong, so that
' every board prints the same.
OPTION EXPLICIT
OPTION DEFAULT INTEGER
CONST NCASES = 50, NSAMP = 50
DIM cf$(13) = ("1,8 P", "1,16 P", "1,12 P", "1,32 P", "7,8 P", "7,16 P", "7,12 P", "3,8 P", "3,16 P", "7,4 P", "7,8 F", "7,16 W", "7,8 R", "7,12 W")
DIM pal(7), rb(7)
pal(0) = RGB(0,0,0) : pal(1) = RGB(255,0,0) : pal(2) = RGB(0,255,0) : pal(3) = RGB(0,0,255)
pal(4) = RGB(255,255,0) : pal(5) = RGB(0,255,255) : pal(6) = RGB(255,0,255) : pal(7) = RGB(255,255,255)
DIM ci, md$, fm$, depth, SW, SH, DW, DH, k, c, cs, s, x, y, ex, exc, got, fails, shown, nsam, allfails, d$, ok
DIM vert, bands, sz, lim, stp, fst, maxn, n, ph, edg, trn, userows, ev, tv
DIM ofs(0), rws(0)
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
      vert = (RND < 0.5)
      bands = DH : sz = SW : lim = SH
      IF vert THEN bands = DW : sz = SH : lim = SW
      stp = 1 : IF RND < 0.6 THEN stp = ri(1, MIN(bands, 40))
      userows = 0
      IF vert = 0 AND RND < 0.5 THEN userows = 1
      IF userows THEN lim = bands
      lim = MIN(lim, bands)                       ' (the rows or columns the source has: R's limit)
      fst = 0 : IF RND < 0.6 THEN fst = ri(0, lim - 1)
      maxn = (lim - fst + stp - 1) \ stp
      IF fst + maxn * stp > lim AND lim < bands THEN maxn = (lim - fst) \ stp   ' (all of the last band in the source)
      IF maxn < 1 THEN
        fst = 0 : stp = 1 : maxn = lim
      ENDIF
      n = ri(1, maxn)
      ERASE ofs : DIM ofs(n - 1)
      FOR k = 0 TO n - 1 : ofs(k) = roff(sz) : NEXT
      ph = 0 : IF RND < 0.5 THEN ph = ri(-50, 50)
      IF RND < 0.15 THEN ph = ri(-(2^40), 2^40)
      edg = -2 : IF RND < 0.5 THEN edg = ri(-1, 7)
      trn = -1 : IF RND < 0.3 THEN trn = ri(0, 7)
      IF userows THEN
        ERASE rws : DIM rws(n - 1)
        FOR k = 0 TO n - 1 : rws(k) = ri(-1, SH - 1) : NEXT
      ENDIF
      ev = -1 : IF edg >= 0 THEN ev = pal(edg)
      IF trn >= 0 THEN tv = tval(pal(trn))
      IF trn >= 0 AND depth = 12 THEN trn = 0 : tv = RGB(255, 0, 0)   ' (12-bit: alpha 0, whatever is given)
      restorebg
      dowarp
      destpage
      FOR s = 1 TO NSAMP
        x = ri(0, DW - 1) : y = ri(0, DH - 1)
        IF RND < 0.6 THEN
          IF vert THEN
            x = ri(fst, MIN(fst + n * stp, DW) - 1)
          ELSE
            y = ri(fst, MIN(fst + n * stp, DH) - 1)
          ENDIF
        ENDIF
        ex = model(x, y)
        IF ex < 0 THEN exc = rb(pm(x + y + 3, 8)) ELSE exc = rb(ex)
        got = PIXEL(x, y)
        nsam = nsam + 1
        IF got <> exc THEN
          fails = fails + 1
          IF shown < 6 THEN
            shown = shown + 1
            d$ = "H" : IF vert THEN d$ = "V"
            PRINT "  case"; cs; " "; d$; " n"; n; " step"; stp; " first"; fst; " phase"; ph;
            PRINT " edge"; edg; " trans"; trn; " rows"; userows; " at"; x; ","; y; " want"; ex; " got &H"; HEX$(got)
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

' the errors (section 6), in MODE 7,8 (320x240)
MODE 7, 8
DIM e1(9), f1!(9), big(240), bigv(320), r1(8), w300(299), w400(399)
DIM sfit = (240 - 10 + 3) \ 4
DIM e4(sfit)
errtest 1, "Framebuffer not created"
FRAMEBUFFER CREATE 400, 300
errtest 2, "Same framebuffer"
errtest 3, "Same page"
errtest 4, "*"
errtest 5, "Array has 241 elements, step 1 from row 0 fits 240"
errtest 6, "Array has 321 elements, step 1 from column 0 fits 320"
errtest 7, "Array has " + STR$(sfit + 1) + " elements, step 4 from row 10 fits " + STR$(sfit)
errtest 8, "Row array has 9 elements, offset array has 10"
errtest 9, "Row array element 5 is 300: rows are -1 to 299"
errtest 10, "*"
errtest 11, "*"
errtest 12, "*"
errtest 13, "*"
errtest 14, "Rows 0 to 299 are not all in the source (240 high)"
errtest 15, "Columns 0 to 399 are not all in the source (320 wide)"
errtest 16, ""
errtest 17, "*"
FRAMEBUFFER CLOSE
MODE 1, 1
errtest 18, "Invalid for this screen mode"
MODE 7, 8
DIM t0!, t1!
t0! = TIMER : FOR k = 1 TO 10 : FRAMEBUFFER WAIT : NEXT : t1! = TIMER
IF (t1! - t0!) / 10 > 5 AND (t1! - t0!) / 10 < 40 THEN PRINT "FRAMEBUFFER WAIT: a frame" ELSE PRINT "FRAMEBUFFER WAIT:"; (t1! - t0!) / 10; " ms"
MODE 1, 8
IF allfails = 0 THEN PRINT "WARP TEST PASSED" ELSE PRINT "WARP TEST FAILED:"; allfails
END

SUB errtest t, want$
  LOCAL r(9)
  SELECT CASE t
    CASE 1
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, e1()
    CASE 2
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, F, e1()
    CASE 3
      ON ERROR SKIP 1 : PAGE WARPV 1 TO 1, e1()
    CASE 4
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH L, N, e1()
    CASE 5
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, big()
    CASE 6
      ON ERROR SKIP 1 : FRAMEBUFFER WARPV F, N, bigv()
    CASE 7
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, e4(), 4, 10
    CASE 8
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, e1(), , , , , , r1()
    CASE 9
      r(5) = 300 : ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, e1(), , , , , , r()
    CASE 10
      ON ERROR SKIP 1 : FRAMEBUFFER WARPV F, N, e1(), , , , , , r()
    CASE 11
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, f1!()
    CASE 12
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, e1(), 0
    CASE 13
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH F, N, e1(), 1, 1, 0, -1, 256
    CASE 14
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH N, F, w300()
    CASE 15
      ON ERROR SKIP 1 : FRAMEBUFFER WARPV N, F, w400()
    CASE 16
      ON ERROR SKIP 1 : PAGE WARPH 1, 0, e1()
    CASE 17
      ON ERROR SKIP 1 : PAGE WARPH 1 TO 0 TO e1()
    CASE 18
      ON ERROR SKIP 1 : FRAMEBUFFER WARPH N, F, e1()
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

' the destination's background put back before a warp
SUB restorebg
  IF fm$ = "P" THEN PAGE COPY 2 TO 0
  IF fm$ = "F" OR fm$ = "W" THEN PAGE COPY 1 TO 0
  IF fm$ = "R" THEN
    PAGE WRITE 0 : FRAMEBUFFER RESTORE
  ENDIF
END SUB

SUB dowarp
  LOCAL a$, c$
  IF fm$ = "P" THEN
    c$ = "PAGE WARP" + CHOICE(vert, "V", "H") + " 1 TO 0, ofs()"
  ELSEIF fm$ = "R" THEN
    c$ = "FRAMEBUFFER WARP" + CHOICE(vert, "V", "H") + " N, F, ofs()"
  ELSE
    c$ = "FRAMEBUFFER WARP" + CHOICE(vert, "V", "H") + " F, N, ofs()"
  ENDIF
  a$ = ", " + STR$(stp) + ", " + STR$(fst) + ", " + STR$(ph) + ", "
  IF edg >= -1 THEN a$ = a$ + STR$(ev)
  a$ = a$ + ", "
  IF trn >= 0 THEN a$ = a$ + STR$(tv)
  IF userows THEN a$ = a$ + ", rws()"
  IF edg = -2 AND trn < 0 AND NOT userows AND stp = 1 AND fst = 0 AND ph = 0 AND RND < 0.5 THEN a$ = ""
  IF RND < 0.5 THEN
    DO WHILE RIGHT$(a$, 1) = "," OR RIGHT$(a$, 1) = " " : a$ = LEFT$(a$, LEN(a$) - 1) : LOOP
  ENDIF
  EXECUTE c$ + a$
END SUB

' what PIXEL should read at (px, py) of the destination: a palette index, or -1 for its own (x + y + 3) MOD 8
FUNCTION model(px, py)
  LOCAL kb, b, e, o, sx, sy
  IF vert THEN kb = px ELSE kb = py
  IF kb < fst OR kb >= fst + n * stp THEN model = -1 : EXIT FUNCTION
  b = (kb - fst) \ stp
  e = pm(b + pm(ph, n), n)
  o = ofs(e)
  IF vert THEN
    sx = px : sy = py + o
  ELSE
    sy = py
    IF userows THEN
      IF rws(e) = -1 THEN model = -1 : EXIT FUNCTION
      sy = pm(rws(e) + py - (fst + b * stp), SH)
    ENDIF
    sx = px - o
  ENDIF
  IF edg = -2 THEN
    sx = pm(sx, SW) : sy = pm(sy, SH)
  ELSEIF sx < 0 OR sx >= SW OR sy < 0 OR sy >= SH THEN
    IF edg = -1 THEN model = -1 ELSE model = edg
    EXIT FUNCTION
  ENDIF
  model = pm(sx - sy, 8)
  IF model = trn THEN model = -1
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

' a random offset: none, small, the size exactly, past the size, or huge
FUNCTION roff(size)
  LOCAL r
  r = ri(0, 5)
  IF r = 0 THEN roff = 0
  IF r = 1 THEN roff = ri(-3 * size, 3 * size)
  IF r = 2 THEN roff = ri(-size - 1, size + 1)
  IF r = 3 THEN roff = size * (2 * ri(0, 1) - 1)
  IF r = 4 THEN roff = ri(-(2^40), 2^40)
  IF r = 5 THEN roff = ri(-2, 2)
END FUNCTION

' the transparent value as WARPH and WARPV take it in this depth: a 4 or 8-bit pixel value, else the colour
FUNCTION tval(cv)
  IF depth = 4 THEN
    tval = ((cv >> 20) AND 8) OR ((cv >> 13) AND 6) OR ((cv >> 7) AND 1)
  ELSEIF depth = 8 THEN
    tval = ((cv >> 16) AND &HE0) OR ((cv >> 11) AND &H1C) OR ((cv >> 6) AND 3)
  ELSE
    tval = cv
  ENDIF
END FUNCTION
