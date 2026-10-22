"""fm_mouse_test.py [PORT [BAUD]]: FM's mouse, through a serial terminal's clicks (Tera Term's VT200 reports, ESC [ M
b x y, sent over the console: COM4 at 460800). The physical mouse goes through the same fm_mouse_key(), so this checks
what a click does; the pointer itself is checked by hand on the display. Its files go in A:/m and are deleted after.
  - a left click selects an entry, and makes the other panel active;
  - a right click marks it; the wheel moves the selection;
  - a double click opens a directory, and shows an image, which a click leaves;
  - a click on the top line: S sorts, F1 shows the help, which a click leaves;
  - a click in a prompt neither cancels it nor reaches FM as keys (its M would be Move);
  - on leaving, the terminal's mouse reports are turned off."""
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vt100drive, edit_test

t = vt100drive.Term(sys.argv[1] if len(sys.argv) > 1 else 'COM4', int(sys.argv[2]) if len(sys.argv) > 2 else 460800)
r = edit_test.Run(t, False)
results = []


def check(ok, what):
    results.append((ok, what))
    print('%-5s %s' % ('ok' if ok else 'FAIL', what))
    if not ok:
        t.screen.show()


def click(row, col, button=' ', release=True, settle=0.8):
    """a press at the cell (row, col), as Tera Term reports it (each 1-based plus 32), and its release"""
    t.send(b'\x1b[M' + bytes([ord(button), col + 33, row + 33]), settle=False)
    if release:
        time.sleep(0.1)
        t.send(b'\x1b[M' + bytes([ord('#'), col + 33, row + 33]), settle=False)
    t.settle(settle)


def wheel(row, col, down):
    t.send(b'\x1b[M' + (b'`' if down else b'a') + bytes([col + 33, row + 33]), settle=False)
    t.settle(0.8)


def find_in(name, left):
    """(row, col) of a name in the left or the right panel"""
    half = t.screen.cols // 2
    for row, line in enumerate(t.screen.lines()):
        i = line.find(name)
        if i >= 0 and row >= 2 and ((i < half) == left):
            return row, i
    return None


r.recover()
r.at_prompt('On Error Skip : MkDir "A:/m"')
r.at_prompt('chdir "A:/m"')
r.at_prompt('On Error Skip : MkDir "sub"')
for n in ('a.bas', 'b.bas', 'c.bas'):
    r.at_prompt('Open "%s" For Output As #1 : Print #1, "Print 1" : Close #1' % n)
r.at_prompt('On Error Skip : Kill "pic.bmp"')
r.at_prompt('Copy "A:/c/bd_24.bmp" To "A:/m/pic.bmp"')
t.mark()
t.cmd('FM', settle=True)
t.settle(1.5)
check('\x1b[?1000h' in t.log,'FM turns on the terminal\'s mouse reports')
t.key('left')                                           # the left panel, at A:/m
t.type('G')
t.settle(0.8)
t.type('A:/m')
t.key('enter')
t.settle(1.5)
check(find_in('b.bas', True) is not None, 'the left panel lists A:/m')

pos = find_in('b.bas', True)
click(*pos)
check(t.screen.message().strip() == 'b.bas', 'a left click selects b.bas: %r' % t.screen.message().strip())

pos = find_in('a.bas', True)
click(*pos, button='"')
row = t.screen.line(pos[0])
check(t.screen.message().strip().startswith('1 marked') and row.lstrip().startswith('*'),
      'a right click marks a.bas: %r / %r' % (t.screen.message().strip(), row.strip()))

wheel(pos[0], pos[1], down=True)
check(t.screen.message().strip() == 'b.bas', 'the wheel moves the selection down to b.bas: %r' % t.screen.message().strip())

pos = find_in('sub', True)
click(*pos, settle=0.15)
click(*pos, settle=1.5)
check(find_in('a.bas', True) is None and find_in('..', True) is not None, 'a double click opens sub')
pos = find_in('..', True)
click(*pos, settle=0.15)
click(*pos, settle=1.5)
check(find_in('a.bas', True) is not None, 'a double click on .. goes back to A:/m')

pos = find_in('pic.bmp', True)
click(*pos, settle=0.15)
t.mark()
click(*pos, settle=2.5)                                 # the image is on the display
click(10, 10, settle=2.0)                               # a click goes back to FM
check(t.screen.message().strip() == 'pic.bmp' and t.screen.find('F1:Help') is not None,
      'a double click shows pic.bmp, and a click returns to FM: %r' % t.screen.message().strip())

i = t.screen.line(0).find('S:Sort')
click(0, i + 2)
check(t.screen.message().strip().startswith('Sort:'), 'a click on S:Sort sorts: %r' % t.screen.message().strip())
click(0, i + 2)
click(0, i + 2)                                         # (back to name order)
i = t.screen.line(0).find('F1:Help')
click(0, i + 1, settle=1.5)
check(t.screen.find('return to FM') is not None, 'a click on F1:Help shows the help')
click(5, 5, settle=1.5)
check(t.screen.find('F1:Help') is not None and t.screen.find('return to FM') is None, 'a click leaves the help')

half = t.screen.cols // 2
for row in range(2, 6):
    if t.screen.line(row)[half + 2:].strip():
        break
click(row, half + 4)
right = t.screen.message().strip()
click(*find_in('c.bas', True))
check(t.screen.message().strip() == 'c.bas' and right != 'c.bas',
      'a click in the right panel makes it active (%r), one in the left makes it active again' % right)

t.type('G')
t.settle(0.8)
check(t.screen.message().find('Go to') >= 0 or t.screen.message().find('path') >= 0, 'G asks for a path: %r' % t.screen.message())
click(*find_in('a.bas', True), settle=1.0)
check(t.screen.message().find('path') >= 0, 'a click leaves the prompt as it is: %r' % t.screen.message())
t.key('esc')
t.settle(1.0)
check(t.screen.find('Move') is None and find_in('a.bas', True) is not None, 'the click was not keys (no Move)')

t.mark()
t.key('esc')
time.sleep(0.5)
t.prompt(10)
check('\x1b[?1000l' in t.log,'leaving FM turns the terminal\'s mouse reports off')

r.at_prompt('chdir "A:/"')
for n in ('a.bas', 'b.bas', 'c.bas', 'pic.bmp'):
    r.at_prompt('On Error Skip : Kill "A:/m/%s"' % n)
r.at_prompt('On Error Skip : RmDir "A:/m/sub"')
r.at_prompt('On Error Skip : RmDir "A:/m"')
print('%d of %d checks passed' % (sum(1 for ok, _ in results if ok), len(results)))
