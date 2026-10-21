' A minus sign in front of a value applies after the powers that follow it, as in maths and most languages (MATLAB,
' Python, Visual Basic, QuickBASIC): -2^2 is -(2^2), -4. It was (-2)^2, 4, as in Excel and BBC BASIC.
Option Explicit
Dim integer x = 2, k
Dim float y = 2.5
Print -2^2; -x^2; -y^2; 0-2^2; -(2)^2; (-2)^2; -(2^2)
Print 2^-2; -2^-2; -2^3; -2^3^2; 2^3^2; 2^-2^2; --2^2; - -x^2
Print 3*-2^2; 1-2^2; -2^2+1; -x^2/2; Exp(-y^2/2); -2*3; -2+3; -x; -(-x)
k = -x^2 : Print k; -x^0; -1^0; (-1)^0
