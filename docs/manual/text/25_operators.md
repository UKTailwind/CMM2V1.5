# Operators

The following operators are listed in order of precedence. Operators that are on the same level (for example + and -) are processed with a left to right precedence as they occur on the program line.

## Numeric Operators (Float or Integer)

{widths: 25,75}
| Operator | Description |
|---|---|
| NOT    INV | NOT will invert the logical value on the right. INV will perform a bitwise inversion of the value on the right. Both of these have the highest precedence so if the value being operated on is an expression it should be surrounded by brackets. For example, IF NOT (A = 3 OR A = 8) THEN ... |
| ^ | Exponentiation (eg, b^n means b to the power of n) |
| - (negation) | A minus sign in front of a value (rather than between two values) negates it. It applies after any exponentiation that follows, as in mathematics: -2^2 is -(2^2), which is -4, while (-2)^2 is 4. A minus after ^ belongs to the exponent: 2^-2 is 0.25. |
| *    /    \    MOD | Multiplication, division, integer division and modulus (remainder) |
| +    - | Addition and subtraction |
| x << y    x >> y    x >>> y | These operate in a special way. << means that the value returned will be the value of x shifted by y bits to the left while >> means the same only right shifted. They are integer functions and any bits shifted off are discarded. For a left shift any bits introduced are set to zero. >> is an unsigned right shift where the top bit is set to 0. >>> is a signed right shift and any bits introduced are set to the value of the top bit (bit 63). |
| <>    <    >    <=    =<    >=    => | Inequality, less than, greater than, less than or equal to, less than or equal to (alternative version), greater than or equal to, greater than or equal to (alternative version) |
| = | Equality (also used in assignment to a variable, eg implied LET). |
| AND    OR    XOR | Conjunction, disjunction, exclusive or. These are bitwise operators and can be used on 64-bit unsigned integers. |

The operators AND, OR and XOR are integer bitwise operators. For example PRINT (3 AND 6) will output 2.

The other logical operations result in the integer 0 (zero) for false and 1 for true. For example the statement PRINT 4 >= 5 will print the number zero on the output and the expression A = 3 > 2 will store +1 in A.

## String Operators

{widths: 25,75}
| Operator | Description |
|---|---|
| + | Join two strings |
| <>    <    >    <=    =<    >=    => | Inequality, less than, greater than, less than or equal to, less than or equal to (alternative version), greater than or equal to, greater than or equal to (alternative version). |
| = | Equality |

String comparisons respect the case of the characters: they compare the characters' codes, so "A" is less than "a".
