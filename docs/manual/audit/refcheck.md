# The reference entries against the build

Made by `tools/manual/refcheck.py` from `docs/manual/ref/` and `firmware.json` (7.00.00b3). Suggestions for the reference pass: the pass decides.

Entries: NO KEYWORD 9, NO SUB-KEYWORD 1, OK 676, PART 7.

## Predefined Read Only Variables (`variables.txt`)

53 entries; not OK: 0

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|

## Options (`options.txt`)

53 entries; not OK: 0

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|

## Commands (`commands.txt`)

409 entries; not OK: 11

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|
| 51 | NO KEYWORD | A: , B: or C: | A: , B: or C: (NO KEYWORD) |
| 380 | PART | CONTINUE DO | CONTINUE FOR (no CONTINUE FOR) |
| 450 | PART | CSUB name [type  [, type]  …] | hex [[ hex[…] (NO KEYWORD); hex [[ hex[…] (NO KEYWORD) |
| 505 | PART | DEFINEFONT #Nbr | hex [[ hex[…] (NO KEYWORD); hex [[ hex[…] (NO KEYWORD) |
| 517 | PART | DIM [type] decl [,decl].. | where 'decl' is: (NO KEYWORD); Examples: (NO KEYWORD) |
| 1344 | PART | LOCAL variable [, variables] | See DIM for the full syntax. (NO KEYWORD) |
| 1474 | NO KEYWORD | Simple array arithmetic | Simple array arithmetic (NO KEYWORD) |
| 1545 | NO KEYWORD | Matrix arithmetic | Matrix arithmetic (NO KEYWORD) |
| 1572 | NO KEYWORD | Vector arithmetic | Vector arithmetic (NO KEYWORD) |
| 1604 | NO KEYWORD | Quaternion arithmetic | Quaternion arithmetic (NO KEYWORD) |
| 2769 | PART | STATIC variable [, variables] | See DIM for the full syntax. (NO KEYWORD) |

## Functions (`functions.txt`)

172 entries; not OK: 6

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|
| 299 | NO SUB-KEYWORD | EPOCH(DATETIME$) | EPOCH(DATETIME$) (no EPOCH( DATETIME) |
| 547 | NO KEYWORD | Simple functions | Simple functions (NO KEYWORD) |
| 591 | NO KEYWORD | Simple Statistics | Simple Statistics (NO KEYWORD) |
| 646 | NO KEYWORD | Vector Arithmetic | Vector Arithmetic (NO KEYWORD) |
| 662 | NO KEYWORD | Matrix Arithmetic | Matrix Arithmetic (NO KEYWORD) |
| 671 | PART | Creation | Creation (NO KEYWORD); complex% = MATH(C_CPLX r!, i!) (NO KEYWORD); complex% = MATH(C_POLAR radius!, angle!) (NO KEYWORD); Floating returns (NO KEYWORD); real! = MATH(C_REAL complex%) imag! = MATH(C_IMAG complex%)  (NO KEYWORD); Unary functions (NO KEYWORD); complex1% = MATH(C_CONJ complex2%) complex1% = MATH(C_SIN co (NO KEYWORD); Basic Arithmetic (NO KEYWORD); complex1% = MATH(C_ADD complex2%,complex3%) complex1% = MATH (NO KEYWORD) |

## Obsolete Commands and Functions (`obsolete.txt`)

8 entries; not OK: 0

| Line | Verdict | First syntax line | What is missing |
|---|---|---|---|
