# The reference entries against the build

Made by `tools/manual/refcheck.py` from `docs/manual/ref/` and `firmware.json` (7.00.00b1). Suggestions for the reference pass: the pass decides.

Entries: NO KEYWORD 9, NO SUB-KEYWORD 1, OK 627, PART 7.

## Predefined Read Only Variables (`variables.txt`)

46 entries; not OK: 0

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|

## Options (`options.txt`)

50 entries; not OK: 0

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|

## Commands (`commands.txt`)

379 entries; not OK: 11

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|
| 50 | NO KEYWORD | A: , B: or C: | A: , B: or C: (NO KEYWORD) |
| 368 | PART | CONTINUE DO | CONTINUE FOR (no CONTINUE FOR) |
| 435 | PART | CSUB name [type  [, type]  …] | hex [[ hex[…] (NO KEYWORD); hex [[ hex[…] (NO KEYWORD) |
| 482 | PART | DEFINEFONT #Nbr | hex [[ hex[…] (NO KEYWORD); hex [[ hex[…] (NO KEYWORD) |
| 494 | PART | DIM [type] decl [,decl].. | where 'decl' is: (NO KEYWORD); Examples: (NO KEYWORD) |
| 1259 | PART | LOCAL variable [, variables] | See DIM for the full syntax. (NO KEYWORD) |
| 1385 | NO KEYWORD | Simple array arithmetic | Simple array arithmetic (NO KEYWORD) |
| 1456 | NO KEYWORD | Matrix arithmetic | Matrix arithmetic (NO KEYWORD) |
| 1483 | NO KEYWORD | Vector arithmetic | Vector arithmetic (NO KEYWORD) |
| 1515 | NO KEYWORD | Quaternion arithmetic | Quaternion arithmetic (NO KEYWORD) |
| 2576 | PART | STATIC variable [, variables] | See DIM for the full syntax. (NO KEYWORD) |

## Functions (`functions.txt`)

163 entries; not OK: 6

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|
| 295 | NO SUB-KEYWORD | EPOCH(DATETIME$) | EPOCH(DATETIME$) (no EPOCH( DATETIME) |
| 535 | NO KEYWORD | Simple functions | Simple functions (NO KEYWORD) |
| 579 | NO KEYWORD | Simple Statistics | Simple Statistics (NO KEYWORD) |
| 634 | NO KEYWORD | Vector Arithmetic | Vector Arithmetic (NO KEYWORD) |
| 650 | NO KEYWORD | Matrix Arithmetic | Matrix Arithmetic (NO KEYWORD) |
| 659 | PART | Creation | Creation (NO KEYWORD); complex% = MATH(C_CPLX r!, i!) (NO KEYWORD); complex% = MATH(C_POLAR radius!, angle!) (NO KEYWORD); Floating returns (NO KEYWORD); real! = MATH(C_REAL complex%) imag! = MATH(C_IMAG complex%)  (NO KEYWORD); Unary functions (NO KEYWORD); complex1% = MATH(C_CONJ complex2%) complex1% = MATH(C_SIN co (NO KEYWORD); Basic Arithmetic (NO KEYWORD); complex1% = MATH(C_ADD complex2%,complex3%) complex1% = MATH (NO KEYWORD) |

## Obsolete Commands and Functions (`obsolete.txt`)

8 entries; not OK: 0

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|
