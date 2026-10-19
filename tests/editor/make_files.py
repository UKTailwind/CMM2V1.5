"""make_files.py: writes the start and expected files of the editor scripts (tests/editor), with the CRLF endings an
editor save gives. Run it after changing a file here; the files are committed as it writes them."""
import os

HERE = os.path.dirname(os.path.abspath(__file__))

FILES = {
    'start1.bas': ["' editor test file 1", 'Dim a%, b$', 'For a% = 1 To 3', '  Print "line"; a%', 'Next', 'b$ = "done"',
                   'Print b$'],
    'start2.bas': ["' editor test 2", 'Print "one";', 'Print " two"', 'x = 1 +', '2', 'Print "a";', '"b"', 'Print x'],
    'expect2.bas': ["' editor test 2", 'Print "one"', 'Print " two"', 'Print "three"', 'x = 1 +2', 'Print "a";"b"',
                    'Print x'],
    'expect4.bas': ["' editor test file 1", 'Dim a%, b$', 'For a% = 1 To 3', '  Print "line"; a%', 'Next',
                    'b$ = "done!"', 'Print b$'],
    'start6.bas': ["' editor test 6", 'Print "before"', 'a = 1', 'b = a / 0', 'Print "after"'],
    'expect6.bas': ["' editor test 6", 'Print "before"', 'a = 1', "'b = a / 0", 'Print "after"'],
    'expect7.bas': ['Print "new"'],
    'start8.bas': ['line one', 'line two keep', 'line three', '    indented four', '  indented five', 'xyz',
                   '      six'],
    # 08 and 09 with the PicoMite's keys and kilo's Shift keys (Peter, 19 Oct)
    'expect8a.bas': ['line one', 'line two keep', 'line three', '  indented four', 'indented five', 'yz', '  a',
                     '      six'],
    'expect8.bas': ['line one', 'g       h', 'b  c', '   d', 'line two keep', 'line three', '  indented four',
                    'indented five', 'yz', 'e   f', '  a', '  six'],
    'start9.bas': ['aaa', 'bbb', 'ccc', 'ddd', 'eee', 'fff'],
    'expect9.bas': ['bb', 'c', 'ddd', 'eee', 'fff', 'ddd', ''],
    'start10.bas': ['apple one', 'banana two', 'Apple three', 'cherry four', 'APPLE five'],
    'expect10.bas': ['apple one', 'banana two', 'Apple three', 'banana four', 'APPLE five'],
    'expect11.bas': ["X' editor test file 1", 'Dim a%, b$', 'For a% = 1 To 3', '  Print "line"; a%', 'Next',
                     'b$ = "done"', 'Print b$'],
    'expect12.bas': ["' editor test 2", 'Print "one";', 'Print " two"', 'x = 1 +', '2', 'Print "a";', '"b"', 'Print x',
                     "' editor test file 1", 'Dim a%, b$', 'For a% = 1 To 3', '  Print "line"; a%', 'Next',
                     'b$ = "done"', 'Print b$'],
    'expect12buf.txt': ["' editor test 2"],
    'expect13.bas': ["' editor test file 1", 'DIM [type] decl [,decl]...Dim a%, b$', 'For a% = 1 To 3',
                     '  Print "line"; a%', 'Next', 'b$ = "done"', 'Print b$'],
    'start14.bas': ['line %03d' % i for i in range(1, 101)],
    # 09b: about 100 KB in 420 lines of 238 characters (kilo's lines stop at 240)
    'big9.bas': ['L%03d ' % i + ''.join(chr(65 + (i + j) % 26) for j in range(233)) for i in range(420)],
    # the engine's scripts (15-19)
    'expect15.txt': ['0123456789' * 30],
    'start16.bas': ['Sub Show(n)', 'Print n', 'End Sub', 'For i = 1 To 3', 'If i = 2 Then', 'Print "two"', 'Else',
                    'Print i', 'End If', 'Next i', 'Do', 'x = x + 1', 'Loop Until x > 3', 'Select Case x', 'Case 4',
                    'Print "four"', 'Case Else', 'Print "other"', 'End Select'],
    'expect16.bas': ['Sub Show(n)', '  Print n', 'End Sub', '', 'For i = 1 To 3', '  If i = 2 Then',
                     '    Print "two"', '  Else', '    Print i', '  End If', 'Next i', 'Do', '  x = x + 1',
                     'Loop Until x > 3', 'Select Case x', 'Case 4', '  Print "four"', 'Case Else', '  Print "other"',
                     'End Select'],
    'start17.bas': ["' a comment line", 'Dim s$ = Left$("abc", 2)', 'x = 123 + 4.5', 'Rem a rem line', '/* a block',
                    'still comment', '*/', 'Print s$, x'],
    'start18.bas': ['Sub A', '    Print 1', 'End Sub'],
    'expect18.bas': ['Sub A', '    Print 1', '    Print 2', '  Print 3', 'x=1', 'End Sub'],
}
# 09b: all of it cut and pasted back, which leaves the empty line the cut left after it
FILES['expect9b.bas'] = FILES['big9.bas'] + ['']


def main():
    for name, lines in FILES.items():
        with open(os.path.join(HERE, name), 'wb') as f:
            f.write(''.join(l + '\r\n' for l in lines).encode('latin-1'))
    print('written', len(FILES), 'files')


if __name__ == '__main__':
    main()
