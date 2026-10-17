' fbbackup.bas - FRAMEBUFFER BACKUP and RESTORE (the whole of it, and a region) put the framebuffer's pixels back,
' its first and last pixels included (the copies by mycopy since copy_words went, 17 Oct)
Dim integer bad, pg, pr, pb, pe, xe, ye
Framebuffer Create
Framebuffer Write
xe = MM.HRes - 1 : ye = MM.VRes - 1
CLS RGB(blue)
Box 10, 10, 100, 50, 1, RGB(red), RGB(red)
Pixel 0, 0, RGB(green)
Pixel xe, ye, RGB(yellow)
pg = Pixel(0, 0) : pr = Pixel(20, 20) : pb = Pixel(200, 200) : pe = Pixel(xe, ye)
Framebuffer Backup
CLS RGB(black)
Framebuffer Restore
If Pixel(0, 0) <> pg Then bad = bad + 1
If Pixel(20, 20) <> pr Then bad = bad + 1
If Pixel(200, 200) <> pb Then bad = bad + 1
If Pixel(xe, ye) <> pe Then bad = bad + 1
Print "whole restore wrong:"; bad
bad = 0
CLS RGB(black)
Framebuffer Restore 0, 0, 50, 50
If Pixel(0, 0) <> pg Then bad = bad + 1
If Pixel(20, 20) <> pr Then bad = bad + 1
If Pixel(60, 20) <> Pixel(200, 200) Then bad = bad + 1             ' outside the region: still black
If Pixel(xe, ye) = pe Then bad = bad + 1
Print "region restore wrong:"; bad
Page Write 0
Framebuffer Close
