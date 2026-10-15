# MATH update: bringing Src/MATHS.c up to the PicoMite, with structure-member strides

Step 8a of the P1 plan (`PicoMite_Parity_Audit_and_Plan.md`). This is a separate exercise from the structures work (Peter, 15 Oct 2026).

The sources:
- a full read of both trees' MATH: the PicoMite `core/MATHS.c` at 066f80b (4641 lines) and `core/Commands.c` `array_*`, against the CMM2's `Src/MATHS.c` (3283 lines);
- a review of every array-argument helper in both trees.

Line numbers are the CMM2's as of 49c373f unless marked P (PicoMite).

## Goal and gates

MATH and its array helpers should match the PicoMite's, which means:

- its bug fixes;
- MATH SINC and MATH CLAMP;
- string arrays in SET/ADD/INSERT/SLICE;
- the CROSSING and SENSORFUSION reworks;
- structure-member arrays (`pts().x`) wherever the PicoMite takes them.

The bugs found in both trees are fixed here too, rather than copied.

Gates:

1. **A new `tests/cmm2/math.bas`.** It has one check per fix below, built from the examples in this plan.
2. **The PicoMite's `Testfiles/`:**
   - `MathStructTest.bas` passes, after its ARRAY SET/ADD use is checked against the CMM2's syntax;
   - `MathClampTest.bas` passes;
   - the existing results of `single_element_matrix_demo`, `ArrayFuncTest` and `StructTest` are unchanged.
3. **The goldens corpus.** Its MATH programs match: mathm, mathm1, matha, mathb64, mathcrc, mathq, mathr, mathw, cmath and arrslice. The deliberate behaviour changes (listed under "Behaviour changes") are reviewed one by one.
4. **SP Tools and the crasher suite**, with no regression.

## Where the CMM2 stands

- **Subcommands.** Every CMM2 MATH subcommand and MATH( subfunction exists in the PicoMite, so there is nothing CMM2-only to keep. The PicoMite adds two: SINC and CLAMP.
- **Already done:**
  - the dimension accessors, so single-element dimensions work through every helper;
  - the CHI fixes (a169409, edf62f7);
  - ARRAY SET/ADD/INSERT/SLICE, which pass to MATH SET/ADD/INSERT/SLICE (`Src/Struct.c` `ArrayAsMath`) until phase 3 replaces them;
  - "Struct member arrays not supported for this command" in the non-MATH commands (`CHECK_STRUCT_MEMBER_ARRAY`).
- **Set aside for this update:** the M_INVERSE `DimUpper` read at line 1332, and its `dealloc2df` row count.
- **CMM2 strengths to keep:**
  - HAL RNG seeding for MATH(RAND) and RANDOMIZE;
  - `GetMemory(0)` succeeding, so FFT of one element works.
- **Flash.** MATHS.c sits in `.extra` (FLASH7, 128 KB), which has about 66 KB free; main flash has about 29 KB. All new MATH and `array_*` code goes in `.extra`, `__attribute__((section(".extra")))`. Expect 8-12 KB of growth.

## Phase 1: the bug fixes (about 300 lines, one commit per group)

### 1a. Memory and crashes

1. **M_INVERSE (1322).**
   - Read the output's bound through `DimUpper` (line 1332). This is the set-aside patch.
   - Free the work matrices with `dealloc2df(m, n, n)` ×3 (1344, 1354, 1355); a169409.
2. **`determinant()` (3242).** Return 1 at size 0 and `matrix[0][0]` at size 1, before allocating the minor (c994e92).
   - Without this, every size-1 leaf leaks two pages: about 15.5 KB per 3x3 inverse, and 3 KB per `MATH(M_DETERMINANT)` of a 3x3.
   - A 1x1 inverse returns 0 instead of 0.25.
   - Test: `DIM a(0,0), b(0,0) : a(0,0)=4 : MATH M_INVERSE a(), b()` gives 0.25, and free memory is unchanged after 300 3x3 inverses.
3. **`g_myrand = NULL` in ClearRuntime after InitHeap.** The Mersenne Twister state is in the heap. After RUN, the next MATH(RAND) writes 2.5 KB into memory that may belong to new arrays.
4. **PID channels cleared in ClearExternalIO** (`memset(PIDchannels, 0, …)`). Otherwise a new program's first interrupt jumps to the old program's PID callback.
5. **V_PRINT (1295).** Test `argc == 3` before reading `argv[2]`, which is uninitialised for `MATH V_PRINT a%()`. Make a third argument other than HEX a syntax error.
6. **Integer C_DIV by zero:** "Divide by zero". It currently goes into `__aeabi_ldivmod` (undefined).
7. **SLICE with no index omitted (1053):** "One index must be omitted", as in the PicoMite's ce4a6d6. INSERT returns silently in that case; make it the same error.

### 1b. Wrong results

1. **MATH WINDOW's output variables (1804, 1808).**
   - They are compared `== T_INT`, so a `DIM INTEGER lo` (T_INT|T_IMPLIED) gets the double's bit pattern. Use `FoundType() & T_INT`, which also accepts a structure member.
   - A constant input divides 0 by 0, giving NaN. Give `outmin` instead.
   - An integer output is a C cast (truncating, undefined for NaN). Use `FloatToInt64`.
2. **CRC12** is not masked to 12 bits (`MATH(CRC12 "123456789")` gives &HFEFB, not &HEFB). **CRC reverseOut** reads `argv[10]` where it should read `argv[12]`, at 2114/2149/2184/2219. Both are 38925c4.
3. **Q_CREATE (1570).** `cos(theta/2)` ignores OPTION ANGLE, while the sine term divides by `optionangle` (38925c4). Also fix the argument numbers: "Argument 4" should be 5, and Q_MULT/Q_ROTATE report argument 31 for 3.
4. **PID START.**
   - `T*1000` must be `T*1000000`: `Getuptime()` is in microseconds, so the first callback comes 1000 times early (9130631).
   - PID INIT keeps a pointer to a LOCAL array that its SUB frees on return. Port `IsGlobalData`/`RequireGlobal` (P `MMBasic.c:7558-7579`, about 25 lines) for the CMM2's chunked global records (57b74f8).
   - Check the array's size before storing `PIDparams`.
5. **MATH RANDOMIZE with no argument** fails about half the time: the HAL RNG value goes into an `int` and is "Number out of bounds" when negative. Seed with `(uint32_t)`.
6. **SCALE with an integer output** rounds the scale before multiplying (`a!(0)=2.5 : MATH SCALE a!(), 0.5, b%()` gives 3).
   - Float in: `FloatToInt64(f * a)`.
   - Integer in: `i64 * a` for an integer scale, which stays exact above 2^53 (the PicoMite's isn't), and `FloatToInt64(f * a)` for a float scale.
7. **ADD and POWER with an integer output** round the value or exponent before the arithmetic (`MATH ADD a!(), 0.4, b%()` with 0.3 gives 0; `MATH POWER a!(), 0.5, b%()` with 16 gives 16). Round once, after the arithmetic.
8. **SLICE and INSERT between float and integer arrays** copy raw bit patterns. Require the same type ("Arrays must be the same type"), or convert.
9. **INTERPOLATE** compares NULL with NULL, so `MATH INTERPOLATE a!(), b!(), 0.5, c%()` is always "Arrays must be different". Compare the pointers in use.
10. **MAX/MIN** start from ±3e38, so an array of values beyond that returns the sentinel. Start from element 0.
11. **FFT** checks that the complex array's first dimension has 2 elements.

### 1c. Reworked by the PicoMite (port whole; behaviour changes)

1. **CROSSING (2241), about 60 lines.** The PicoMite version:
   - finds the first sample past the level that holds for `confirm` samples (a new optional argument);
   - returns `i + OptionBase` (ec27921);
   - looks at the last sample.

   The CMM2 heuristic returns a 0-based index whatever the OPTION BASE, and can report a sample still below the level. `DIM a!(4)=-1,-1,1,1,1 : PRINT MATH(CROSSING a!())` gives 1 now and 2 after.
2. **SENSORFUSION (2883/2952/3062), about 150 lines.** The PicoMite version:
   - takes the gyro rates and gives the angles in the OPTION ANGLE units;
   - makes the magnetometer optional (a 6-axis update when mx is empty);
   - guards a zero gradient (the CMM2 latches NaN into `q[]` for good);
   - normalises the quaternion (`StoreQuaternion`) before working out the angles;
   - reads and clears `AHRSTimer` with interrupts off (the CMM2 can lose a tick).
3. **Optional:** `getrnd()` (935) from `HAL_RNG_GenerateRandomNumber` instead of `rand()`, for the AES CBC/CTR initialisation vectors.

## Phase 2: the helpers, with the stride parameter (about 150 lines, plus a scripted signature change)

The PicoMite's helpers:

```c
int parseintegerarray(char *tp, int64_t **a1int, int argno, int dimensions, int *dims, bool ConstantNotAllowed, int *stride);
int parsenumberarray(char *tp, MMFLOAT **a1float, int64_t **a1int, int argno, int dimensions, int *dims, bool ConstantNotAllowed, int *stride);
int parsefloatarray(char *tp, MMFLOAT **a1float, int argno, int dimensions, int *dims, bool ConstantNotAllowed, int *stride);
int parsestringarray(...);                      // new: phase 3
int parsearrays_stride(char *tp, ..., int *s1, int *s2, int *s3);     // parsearrays() calls it with NULLs
#define STRIDE_FLOAT(ptr, idx, stride) (*(MMFLOAT *)((char *)(ptr) + (idx) * (stride)))
#define STRIDE_INT(ptr, idx, stride) (*(long long int *)((char *)(ptr) + (idx) * (stride)))
```

1. **Take the PicoMite's signatures.** This means the extra `int *stride`, and renaming `parsefloatrarray` (a CMM2 typo) to `parsefloatarray`. Keeping them the same as the PicoMite's makes later syncing easier than wrappers would. A script adds `, NULL` to every existing call:

   | File | Calls |
   |---|---|
   | MATHS.c | 79 |
   | MM_Misc.c | 34 |
   | Memory.c | 6 |
   | MM_Custom.c | 2 |
   | External.c | 1 |

   The image should then be byte-identical apart from the new parameter.
2. **The structure branch, as the PicoMite's, with its faults fixed.** When `g_StructMemberType` is set and the base is T_STRUCT:
   - `stride == NULL` gives "Struct member arrays not supported for this command". SORT, LONGSTRING and every other helper user then refuse a member array clearly; today they give "Argument n must be a numerical array".
   - A non-numeric member gives "Argument % must be a numerical array".
   - `*stride` is the structure's `total_size`, and the pointer is the first element's member.
   - **Not in the PicoMite: require the whole array (`pts().x`).** Its branch never checks this. `pts(2).x` passes, starts at element 2, and still takes the whole array's element count, so a MATH command runs past the end. Use findvar's `emptyarray` flag, or the `ptr1 == val.s + offset` test the plain branch uses.
   - **Not in the PicoMite: apply the `dimensions` shape check (1D/2D) to the structure array too.** It also fills `dims` for callers that read it (SCALE does).
   - **Count elements with `DimIsEnd`**, not `j > 0`, which relies on OPTION BASE 1 to stop.
   - A member array inside one structure (`s.xs()`) is contiguous and could be taken with `stride` = the element size. findvar does not resolve `s.xs()` today ("Dimensions"). Leave it as a later extension.
3. **The other array-review fixes in the helpers:**
   - `parseany` tests `dimtbl[0]` before `dimtbl[1]`, so a short scalar string reports "must be a string array";
   - align the error wording with the PicoMite's where the goldens compare it ("Invalid syntax").

## Phase 3: string arrays in SET/ADD/INSERT/SLICE (about 420 lines)

1. **Port `array_set`, `array_add`, `array_insert` and `array_slice`** (P `Commands.c:1266-1640`) into MATHS.c, in `.extra`. Each handles numbers (strided) and strings:
   - `ARRAY SET "x", s$()`;
   - `ARRAY ADD s$(), "x", t$()` appends;
   - INSERT and SLICE copy strings, with "String arrays differ in string length".
2. **MATH SET/ADD/INSERT/SLICE call them with `tp`, and `cmd_arrayset` etc. with `cmdline`.** Delete `ArrayAsMath` from Struct.c.
3. **Port `parsestringarray` (P `MATHS.c:719-759`), plus `if(!DimIsRealArray(dims[0])) error("Argument % must be a string array")`.** Without it a scalar string passes. A short one, held inside its record (`DIM s$ LENGTH 8`), turns its characters into a huge element count. This is in PicoMite issue #11.
4. The SLICE and INSERT fixes of 1a.7 and 1b.8 go into the ported code.

## Phase 4: stride in the MATH loops (about 200 lines)

1. **Pass `&stride` and index with `STRIDE_FLOAT`/`STRIDE_INT`** in the subcommands the PicoMite strides:
   - SCALE, CLAMP, C_ADD, C_MUL(T), C_SUB, C_DIV, POWER, V_ROTATE, WINDOW;
   - MATH(MAX, MIN, MEAN, SUM);
   - SET and ADD, through phase 3.

   Use `parsearrays_stride` in the C_ ones.
2. **Allow a structure member as a single-variable output, through `FoundType()`:** WINDOW's min/max outputs and MAX/MIN's index variable. SENSORFUSION's pitch/roll/yaw would be a CMM2 extension (the PicoMite tests `& T_NBR` on the base).
3. **Every other subcommand passes NULL** and refuses a member array.
4. **Gate:** `MathStructTest.bas`. It exercises SCALE, C_ADD/SUB/MUL/DIV, POWER, V_ROTATE, WINDOW, MAX/MIN/MEAN/SUM and ARRAY SET/ADD on members.

## Phase 5: new subcommands (about 250 lines)

1. **MATH CLAMP** (P 1689, 2310038; 55 lines, strided): in the 'C' block before C_ADD. Gate: `MathClampTest.bas`.
2. **MATH SINC** (P 1520, with `sinc_filter` 929 and `sinc_filter_interpolate` 1022): about 200 lines. MMFLOAT is double on the CMM2, so the kernel is as written.
3. New MATH( subfunctions would only be text after the MATH( token, so the nearly full one-byte function token table is not affected. None is new in this update.

## Behaviour changes to note in the release notes and the dialect document

Each changes what an existing CMM2 program sees.

- **CROSSING:** a new algorithm, an OPTION BASE index, and a `confirm` argument.
- **SENSORFUSION:** under OPTION ANGLE DEGREES, gyro rates and angles are in degrees.
- **SCALE, ADD and POWER into an integer array:** rounded once, after the arithmetic.
- **WINDOW:** a constant input gives `outmin`, not NaN.
- **MAX/MIN:** values beyond ±3e38.
- **Q_CREATE:** under OPTION ANGLE DEGREES.
- **Mixed-type SLICE/INSERT and SLICE with every index given:** now errors.
- **PID INIT with a LOCAL array:** now an error.
- **New:** string arrays, structure-member arrays, CLAMP, SINC.

## Upstream

PicoMite issue #10 lists the shared bugs; ce4a6d6 has fixed SLICE. Report back to it as each is fixed here:

- INSERT with every index given;
- the mixed types;
- INTERPOLATE;
- ADD/POWER rounding;
- WINDOW's 0/0;
- MAX/MIN;
- the Q_ argument numbers;
- FFT's dimension check;
- C_DIV by zero;
- SCALE's 2^53;
- the RANDOMIZE `time_us_32` seed;
- the stride branch's whole-array check, shape check and count loop (from phase 2).

## Order and size

| Phase | Lines | Depends on | Gate |
|---|---|---|---|
| 1a memory and crashes | about 60 | none | math.bas, heap unchanged after a loop of inverses |
| 1b wrong results | about 120 | none | math.bas |
| 1c CROSSING, SENSORFUSION | about 210 | none | math.bas, goldens mathr/mathw |
| 2 helpers and the stride parameter | about 150 + script | none | image the same apart from the parameter; all goldens |
| 3 string arrays | about 420 | 2 | math.bas, arrays.bas |
| 4 stride loops | about 200 | 2, 3 | MathStructTest |
| 5 CLAMP, SINC | about 250 | 2 | MathClampTest |

Total: about 1,400 lines touched, almost all in MATHS.c, plus about 30 in MMBasic.c/External.c/Struct.c. That is 8-12 KB of `.extra` flash.

Each phase is a commit, or a few, built and run on the board. The full regression runs after phases 1, 3 and 5.
