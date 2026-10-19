"""edit_heap_test.py: MM.INFO(HEAP) and MM.INFO(IHEAP) the same before and after EDIT (Editor_Port_Plan.md, E1's gate),
on the board on COM4: Esc on an unchanged file, Esc and Y after a change, F1 twice (the same each time, and NEW after it),
and Esc twice with a module playing (A:/snd/tiny.mod, which must play on). Uses tests/editor/start1.bas as A:/e/t.bas."""
import os, re, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vt100drive, edit_test

t = vt100drive.Term()
r = edit_test.Run(t, False)
results = []


def check(ok, what):
    results.append((ok, what))
    print('%-5s %s' % ('ok' if ok else 'FAIL', what))


def heap():
    out = r.at_prompt('? mm.info(heap); mm.info(iheap)')
    nums = re.findall(r'-?\d+', out.split('\n', 1)[1])
    return tuple(int(n) for n in nums[:2])


def edit_and(keys):
    t.cmd('EDIT "A:/e/t.bas"', settle=True)
    t.settle(1.0)
    if 't.bas' not in t.screen.status():
        check(False, 'the editor opened: %r' % t.screen.status())
        return
    for k in keys:
        if k.startswith('type:'):
            t.type(k[5:])
        else:
            t.key(k)
        t.settle(0.3)
    t.prompt(20)


r.recover()
r.put('t.bas', 'start1.bas')
r.at_prompt('new')
h0 = heap()
edit_and(['esc'])
h1 = heap()
check(h1 == h0, 'Esc, unchanged file: %s then %s' % (h0, h1))
edit_and(['type:x', 'esc', 'type:Y', 'enter'])
h2 = heap()
check(h2 == h0, 'Esc and Y after a change: %s then %s' % (h0, h2))
edit_and(['f1'])                                    # F1 loads the program: its heap is the program's, the same each time
hl = heap()
r.at_prompt('new')
edit_and(['f1'])
h3 = heap()
check(h3 == hl, 'F1 twice (the program loaded) gives the same: %s then %s' % (hl, h3))
r.at_prompt('new')
h3n = heap()
check(h3n == h0, 'F1 then NEW, as at the start: %s then %s' % (h0, h3n))

r.at_prompt('new')
r.at_prompt('play modfile "A:/snd/tiny.mod"')
time.sleep(1)
h4 = heap()
edit_and(['esc'])
h5 = heap()
check(h5 == h4, 'with a module playing, Esc: %s then %s' % (h4, h5))
edit_and(['type:y', 'esc', 'type:Y', 'enter'])
h6 = heap()
check(h6 == h4, 'with a module playing, Esc and Y: %s then %s' % (h4, h6))
out = r.at_prompt('? mm.info(sound)')
check('MOD' in out.upper(), 'the module still plays: %r' % out.split('\n', 1)[1].strip())
r.at_prompt('play stop')
r.at_prompt('new')
h7 = heap()
check(h7 == h0, 'after PLAY STOP and NEW, as at the start: %s then %s' % (h0, h7))
t.close()
print('%d of %d checks passed' % (sum(1 for ok, _ in results if ok), len(results)))
sys.exit(0 if all(ok for ok, _ in results) else 1)
