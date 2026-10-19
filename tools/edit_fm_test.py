"""edit_fm_test.py: FM driving EDIT (Editor_Port_Plan.md, E1's FM gate), on the board on COM4: E then Esc, E then F2
(the program runs, FM comes back), N (a new file, typed and saved with F1), and E after a program stopped with an
error (the editor opens at its line). Prints each check; its files go in A:/e and are deleted after."""
import os, sys, tempfile, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vt100drive, edit_test

d = tempfile.mkdtemp()
FILES = {'fmrun.bas': ['Print "fm run ok"'],
         'fmerr.bas': ['Print "before"', 'x = 1', 'y = x / 0', 'Print "after"']}
for n, lines in FILES.items():
    open(os.path.join(d, n), 'wb').write(''.join(l + '\r\n' for l in lines).encode())
edit_test.HERE = d

t = vt100drive.Term()
r = edit_test.Run(t, False)
results = []


def check(ok, what):
    results.append((ok, what))
    print('%-5s %s' % ('ok' if ok else 'FAIL', what))
    if not ok:
        t.screen.show()


def pick(prefix):
    t.type('/')
    t.type(prefix)
    t.settle(1.0)


r.recover()
r.at_prompt('chdir "A:/e"')
for n in FILES:
    r.put(n, n)
r.kill(['fmnew.bas', 'fmnew.bas.bak'])
t.cmd('FM', settle=True)
t.settle(1.0)
check(t.screen.find('fmrun.bas') is not None, 'FM lists fmrun.bas')

# E (F4) then Esc
pick('fmrun')
t.key('f4')
t.settle(1.0)
check('fmrun.bas' in t.screen.status(), 'F4 opens the editor on fmrun.bas: %r' % t.screen.status())
t.key('esc')
t.settle(1.5)
check(t.screen.find('A:/E') is not None, 'Esc goes back to FM')

# E then F2: saved, run, FM back
pick('fmrun')
t.key('f4')
t.settle(1.0)
check('fmrun.bas' in t.screen.status(), 'F4 opens the editor again')
t.mark()
t.key('f2')
t.settle(3.0)
check('fm run ok' in t.text(), 'F2 runs the program (its output)')
t.screen.show()
# FM comes back after the program (it may wait for a key)
if t.screen.find('A:/E') is None:
    t.key('enter')
    t.settle(2.0)
check(t.screen.find('A:/E') is not None, 'FM comes back after the program')

# N: a new file, through the editor
t.type('N')
t.settle(1.0)
check(t.screen.find('New file name') is not None, 'N asks for the name')
t.type('fmnew.bas')
t.key('enter')
t.settle(1.5)
check('fmnew.bas' in t.screen.status(), 'N opens the editor on the new file: %r' % t.screen.status())
t.type('Print "new"')
t.key('f1')
t.settle(2.0)
check(t.screen.find('A:/E') is not None, 'F1 goes back to FM')
check(t.screen.find('fmnew.bas') is not None, 'FM lists fmnew.bas')

# a program that stops with an error, then E: the editor at its line
pick('fmerr')
t.mark()
t.key('enter')
t.settle(3.0)
check('before' in t.text(), 'Enter runs fmerr.bas')
t.screen.show()
if t.screen.find('A:/E') is None:
    t.key('enter')
    t.settle(2.0)
check(t.screen.find('A:/E') is not None, 'FM comes back after the error')
t.screen.show()
pick('fmerr')
t.key('f4')
t.settle(1.0)
check('fmerr.bas' in t.screen.status(), 'F4 opens the editor on fmerr.bas')
check(t.screen.r == 2, 'the cursor is on the error line, row 2 (0-based): at %d,%d' % (t.screen.r, t.screen.c))
t.screen.show()
t.key('esc')
t.settle(1.5)
t.key('esc')
t.settle(1.5)
check(t.at_prompt(), 'Esc leaves FM')
got = r.get('fmnew.bas')
check(got == b'Print "new"\r\n', 'fmnew.bas holds what was typed: %r' % got)
r.kill(['fmnew.bas', 'fmnew.bas.bak', 'fmrun.bas', 'fmrun.bas.bak', 'fmerr.bas'])
t.close()
print('%d of %d checks passed' % (sum(1 for ok, _ in results if ok), len(results)))
sys.exit(0 if all(ok for ok, _ in results) else 1)
