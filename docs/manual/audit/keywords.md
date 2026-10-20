# The manual audit: every keyword of the build against the manuals

Made by `tools/manual/crossref.py` from the firmware 7.00.00b1 (`firmware.json`) and the harvests of the PicoMite manual 7.0 (`pico.json`), the CMM2 manual 6.00.00 b14 (`cmm2_b14.json`, pages) and the CMM2 manual 5.05.04 (`cmm2_2020.json`, tables). Run it again after any of them changes; the Audit column is kept by hand (docs/Manual_Plan.md, M-A3).

Keywords by where they are documented (a suggestion, to be confirmed): C 40, P 16, P+C 253, U 8.

## 1. The build's keywords

| Keyword | Kind | Handler | PicoMite 7.0 | CMM2 b14 | CMM2 2020 | Suggested | Audit |
|---|---|---|---|---|---|---|---|
| **PLAY** | command | Audio.c:1522 | t17 | p116, p117, p118 | t11 | P+C |  |
| &nbsp;&nbsp;STOP | sub-keyword | Audio.c:1525 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;NEXT | sub-keyword | Audio.c:1529 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PREVIOUS | sub-keyword | Audio.c:1540 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PAUSE | sub-keyword | Audio.c:1551 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RESUME | sub-keyword | Audio.c:1569 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | Audio.c:1584 | - | - | - | U |  |
| &nbsp;&nbsp;VOLUME | sub-keyword | Audio.c:1589 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;TONE | sub-keyword | Audio.c:1599 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;BBC SOUND | sub-keyword | Audio.c:1642 | yes | - | - | P |  |
| &nbsp;&nbsp;BBC ENVELOPE | sub-keyword | Audio.c:1701 | yes | - | - | P |  |
| &nbsp;&nbsp;LOAD SOUND | sub-keyword | Audio.c:1712 | yes | - | - | P |  |
| &nbsp;&nbsp;ARRAY | sub-keyword | Audio.c:1720 | yes | - | - | P |  |
| &nbsp;&nbsp;SAMPLE | sub-keyword | Audio.c:1776 | yes | - | - | P |  |
| &nbsp;&nbsp;RELEASE | sub-keyword | Audio.c:1858 | yes | - | - | P |  |
| &nbsp;&nbsp;SOUND | sub-keyword | Audio.c:1863 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;O | sub-keyword | Audio.c:1870 | - | - | - | U |  |
| &nbsp;&nbsp;L | sub-keyword | Audio.c:1875 | yes | - | - | P |  |
| &nbsp;&nbsp;R | sub-keyword | Audio.c:1876 | yes | - | - | P |  |
| &nbsp;&nbsp;B | sub-keyword | Audio.c:1877 | - | - | - | U |  |
| &nbsp;&nbsp;Q | sub-keyword | Audio.c:1890 | - | - | - | U |  |
| &nbsp;&nbsp;T | sub-keyword | Audio.c:1891 | yes | - | - | P |  |
| &nbsp;&nbsp;W | sub-keyword | Audio.c:1892 | - | - | - | U |  |
| &nbsp;&nbsp;S | sub-keyword | Audio.c:1893 | - | - | - | U |  |
| &nbsp;&nbsp;P | sub-keyword | Audio.c:1894 | - | - | - | U |  |
| &nbsp;&nbsp;N | sub-keyword | Audio.c:1895 | yes | - | - | P |  |
| &nbsp;&nbsp;U | sub-keyword | Audio.c:1896 | - | - | - | U |  |
| &nbsp;&nbsp;WAV | sub-keyword | Audio.c:1935 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;TTS | sub-keyword | Audio.c:1994 | - | yes | yes | C |  |
| &nbsp;&nbsp;MODSAMPLE | sub-keyword | Audio.c:1999 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;EFFECT | sub-keyword | Audio.c:2026 | - | yes | yes | C |  |
| &nbsp;&nbsp;FLAC | sub-keyword | Audio.c:2084 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MODFILE | sub-keyword | Audio.c:2143 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MP3 | sub-keyword | Audio.c:2206 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PHONETIC | sub-keyword | Audio.c:1221 (cmd_tts) | - | yes | yes | C |  |
| **CALL** | command | Commands.c:1406 | t17, t18 | p136, p82 | - | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | Commands.c:3308 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;STRING | sub-keyword | Commands.c:3310 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | Commands.c:3312 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;BYVAL | sub-keyword | MMBasic.c:866 (SubLayoutBuild) | - | - | - | U |  |
| &nbsp;&nbsp;BY VAL | sub-keyword | MMBasic.c:866 (SubLayoutBuild) | - | - | - | U |  |
| &nbsp;&nbsp;BYREF | sub-keyword | MMBasic.c:867 (SubLayoutBuild) | - | - | - | U |  |
| &nbsp;&nbsp;BY REF | sub-keyword | MMBasic.c:867 (SubLayoutBuild) | - | - | - | U |  |
| **CLEAR** | command | Commands.c:3679 | t17 | p83 | t11 | P+C |  |
| &nbsp;&nbsp;VARS | sub-keyword | Commands.c:3681 | - | - | - | U |  |
| **ERASE** | command | Commands.c:3675 | t17 | p90 | - | P+C |  |
| **CONTINUE** | command | Commands.c:1379 | t17 | p83 | t11 | P+C |  |
| &nbsp;&nbsp;DO | sub-keyword | Commands.c:1385 | yes | yes | yes | P+C |  |
| **DATA** | command | Commands.c:93 | t17 | p87 | t11 | P+C |  |
| **MMDEBUG** | command | Commands.c:421 | - | p109 | - | C |  |
| &nbsp;&nbsp;BREAK | sub-keyword | Commands.c:423 | - | yes | - | C |  |
| &nbsp;&nbsp;COLOR | sub-keyword | MMBasic.c:2009 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;SPRITE | sub-keyword | MMBasic.c:2012 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;WII | sub-keyword | MMBasic.c:2015 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;PIXEL FAST | sub-keyword | MMBasic.c:2018 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;ELSE IF | sub-keyword | MMBasic.c:2021 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;END IF | sub-keyword | MMBasic.c:2024 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;EXIT DO | sub-keyword | MMBasic.c:2027 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;CAT | sub-keyword | MMBasic.c:2030 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;AS | sub-keyword | MMBasic.c:2074 (tokenise) | - | - | - | U |  |
| **DIM** | command | Commands.c:3385 | t17 | p87, p88 | t11 | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | Commands.c:3308 (CheckIfTypeSpecified) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;STRING | sub-keyword | Commands.c:3310 (CheckIfTypeSpecified) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | Commands.c:3312 (CheckIfTypeSpecified) | yes | - | yes | P+C |  |
| **DO** | command | Commands.c:2391 | t17 | p89 | t11 | P+C |  |
| **ELSEIF** | command | Commands.c:1747 | t17 | p90 | t11 | P+C |  |
| **CASE ELSE** | command | Commands.c:1938 | - | p123 | - | C |  |
| **ELSE** | command | Commands.c:1747 | t17 | p89, p90 | t11 | P+C |  |
| **SELECT CASE** | command | Commands.c:1808 | t17 | p123 | t11 | P+C |  |
| &nbsp;&nbsp;IS | sub-keyword | Commands.c:1855 | - | - | - | U |  |
| **END SELECT** | command | Commands.c:93 | - | p123 | - | C |  |
| **CASE** | command | Commands.c:1938 | t17 | p123 | t11 | P+C |  |
| **END FUNCTION** | command | Commands.c:2746 | t17 | p90, p92 | t11 | P+C |  |
| **END SUB** | command | Commands.c:2737 | t17 | p129, p90 | t11 | P+C |  |
| **ENDIF** | command | Commands.c:93 | t17 | p90, p96 | t11 | P+C |  |
| **END** | command | Commands.c:1792 | t17 | p123, p129, p132, p85, p86, p87, p90, p9 | t11 | P+C |  |
| &nbsp;&nbsp;NOEND | sub-keyword | Commands.c:1794 | yes | - | - | P |  |
| &nbsp;&nbsp;ON | sub-keyword | Draw.c:540 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OFF | sub-keyword | Draw.c:625 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:630 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:642 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | - | - | - | U |  |
| **EXIT FOR** | command | Commands.c:2529 | t17 | p90 | t11 | P+C |  |
| **EXIT SUB** | command | Commands.c:2737 | t17 | p90 | t11 | P+C |  |
| **EXIT FUNCTION** | command | Commands.c:2746 | t17 | p90 | t11 | P+C |  |
| **EXIT** | command | Commands.c:2538 | t17 | p90 | t11 | P+C |  |
| **ERROR** | command | Commands.c:2547 | t17 | p90 | t11 | P+C |  |
| **FOR** | command | Commands.c:2057 | t17 | p70, p88, p91 | t11 | P+C |  |
| **FUNCTION** | command | Commands.c:2564 | t17 | p92 | t11 | P+C |  |
| **GOSUB** | command | Commands.c:2592 | t17, t19 | p154 | t15 | P+C |  |
| **GOTO** | command | Commands.c:1398 | t17 | p93 | t11 | P+C |  |
| **HELP** | command | Commands.c:273 | t17 | p94 | - | P+C |  |
| **IF** | command | Commands.c:1572 | t17, t19 | p154, p70, p96 | t11, t15 | P+C |  |
| **INC** | command | Commands.c:360 | t17 | p97 | - | P+C |  |
| **LINE INPUT** | command | Commands.c:3175 | t17 | p99 | t11 | P+C |  |
| **INPUT** | command | Commands.c:1963 | t17 | p97 | t11 | P+C |  |
| **LET** | command | Commands.c:637 | t17 | p98 | t11 | P+C |  |
| **LIST** | command | Commands.c:969 | t17 | p100, p88, p97, p99 | t11 | P+C |  |
| &nbsp;&nbsp;ALL | sub-keyword | Commands.c:976 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FLASH ALL | sub-keyword | Commands.c:990 | - | - | - | U |  |
| &nbsp;&nbsp;PROFILE CSV | sub-keyword | Commands.c:993 | - | yes | - | C |  |
| &nbsp;&nbsp;PROFILE | sub-keyword | Commands.c:1050 | - | yes | - | C |  |
| &nbsp;&nbsp;FILES | sub-keyword | Commands.c:1099 | - | yes | yes | C |  |
| &nbsp;&nbsp;PAGES | sub-keyword | Commands.c:1101 | - | yes | - | C |  |
| &nbsp;&nbsp;TYPE | sub-keyword | Commands.c:1127 | - | yes | - | C |  |
| &nbsp;&nbsp;FLASH | sub-keyword | Commands.c:1129 | - | - | - | U |  |
| &nbsp;&nbsp;COMMANDS | sub-keyword | Commands.c:1132 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;COMMANDS V | sub-keyword | Commands.c:1156 | - | - | - | U |  |
| &nbsp;&nbsp;FUNCTIONS | sub-keyword | Commands.c:1160 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FUNCTIONS V | sub-keyword | Commands.c:1187 | - | - | - | U |  |
| &nbsp;&nbsp;ON | sub-keyword | Draw.c:540 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OFF | sub-keyword | Draw.c:625 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:630 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:642 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;NAME | sub-keyword | Commands.c:780 (do_listfiles) | - | yes | - | C |  |
| &nbsp;&nbsp;TIME | sub-keyword | Commands.c:781 (do_listfiles) | - | - | - | U |  |
| &nbsp;&nbsp;SIZE | sub-keyword | Commands.c:782 (do_listfiles) | - | yes | - | C |  |
| **LOCAL** | command | Commands.c:3385 | t17 | p101 | t11 | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | Commands.c:3308 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;STRING | sub-keyword | Commands.c:3310 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | Commands.c:3312 (CheckIfTypeSpecified) | - | - | - | U |  |
| **LOOP** | command | Commands.c:2475 | t17 | p102, p89 | t11 | P+C |  |
| **NEW** | command | Commands.c:2169 | t17 | p112 | t11 | P+C |  |
| **NEXT** | command | Commands.c:2193 | t17 | p112 | t11 | P+C |  |
| **ON** | command | Commands.c:3209 | t17, t19 | p112, p113, p154, p75 | t11, t13, t15 | P+C |  |
| &nbsp;&nbsp;KEY | sub-keyword | Commands.c:3215 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;ERROR | sub-keyword | Commands.c:3240 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;ABORT | sub-keyword | Commands.c:3242 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CLEAR | sub-keyword | Commands.c:3249 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;IGNORE | sub-keyword | Commands.c:3250 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RESTART | sub-keyword | Commands.c:3254 | yes | - | - | P |  |
| &nbsp;&nbsp;SKIP | sub-keyword | Commands.c:3258 | yes | yes | yes | P+C |  |
| **PRINT** | command | Commands.c:440 | t17 | p119, p120, p70 | t11 | P+C |  |
| **READ** | command | Commands.c:2753 | t17 | p121, p122 | t11 | P+C |  |
| &nbsp;&nbsp;SAVE | sub-keyword | Commands.c:2759 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;RESTORE | sub-keyword | Commands.c:2766 | yes | yes | - | P+C |  |
| **REM** | command | Commands.c:93 | t17 | - | t11 | P+C |  |
| **RESTORE** | command | Commands.c:3131 | t17 | p122 | t11 | P+C |  |
| **RETURN** | command | Commands.c:2737 | t17, t19 | p154 | t15 | P+C |  |
| **RUN** | command | Commands.c:1290 | t17 | p122 | t11 | P+C |  |
| &nbsp;&nbsp;DEFINE | sub-keyword | FileIO.c:586 (FileLoadProgram) | - | - | - | U |  |
| **STATIC** | command | Commands.c:3385 | t17 | p129 | t13 | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | Commands.c:3308 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;STRING | sub-keyword | Commands.c:3310 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | Commands.c:3312 (CheckIfTypeSpecified) | - | - | - | U |  |
| **SUB** | command | Commands.c:2564 | t17 | p129 | t13 | P+C |  |
| **TRACE** | command | Commands.c:2030 | t17 | p131 | t13 | P+C |  |
| &nbsp;&nbsp;ON | sub-keyword | Commands.c:2031 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;OFF | sub-keyword | Commands.c:2033 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;LIST | sub-keyword | Commands.c:2035 | yes | yes | yes | P+C |  |
| **WHILE** | command | Commands.c:2391 | - | - | - | U |  |
| **CONST** | command | Commands.c:3496 | t17 | p83 | t11 | P+C |  |
| **MID$(** | command | Commands.c:2679 | t17, t18 | p109, p146 | t11, t14 | P+C |  |
| **LMID(** | command | Commands.c:2648 | t17 | p98 | - | P+C |  |
| **EXECUTE** | command | Commands.c:288 | t17 | p90 | t11 | P+C |  |
| &nbsp;&nbsp;COLOR | sub-keyword | MMBasic.c:2009 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;SPRITE | sub-keyword | MMBasic.c:2012 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;WII | sub-keyword | MMBasic.c:2015 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;PIXEL FAST | sub-keyword | MMBasic.c:2018 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;ELSE IF | sub-keyword | MMBasic.c:2021 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;END IF | sub-keyword | MMBasic.c:2024 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;EXIT DO | sub-keyword | MMBasic.c:2027 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;CAT | sub-keyword | MMBasic.c:2030 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;AS | sub-keyword | MMBasic.c:2074 (tokenise) | - | - | - | U |  |
| **TRIANGLE** | command | Draw.c:4826 | t17 | p131 | t13 | P+C |  |
| **TEXT** | command | Draw.c:3122 | t17 | p131 | t13 | P+C |  |
| **PIXEL** | command | Draw.c:3405 | t17, t18 | p116, p149 | t11, t14 | P+C |  |
| &nbsp;&nbsp;FILL | sub-keyword | Draw.c:3406 | - | yes | - | C |  |
| **CIRCLE** | command | Draw.c:3962 | t17 | p83 | t11 | P+C |  |
| **LINE** | command | Draw.c:4204 | t17 | p98, p99 | t11 | P+C |  |
| &nbsp;&nbsp;PLOT | sub-keyword | Draw.c:4241 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;GRAPH | sub-keyword | Draw.c:4275 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;AA | sub-keyword | Draw.c:4283 | yes | yes | - | P+C |  |
| **BOX** | command | Draw.c:4364 | t17 | p81 | t11 | P+C |  |
| &nbsp;&nbsp;AND_PIXELS | sub-keyword | Draw.c:4369 | - | yes | - | C |  |
| &nbsp;&nbsp;FRAMEBUFFER | sub-keyword | Draw.c:4403 | - | - | - | U |  |
| &nbsp;&nbsp;OR_PIXELS | sub-keyword | Draw.c:4446 | - | yes | - | C |  |
| &nbsp;&nbsp;XOR_PIXELS | sub-keyword | Draw.c:4523 | - | yes | - | C |  |
| **RBOX** | command | Draw.c:4701 | t17 | p121 | t11 | P+C |  |
| **CLS** | command | Draw.c:4918 | t17 | p83 | t11 | P+C |  |
| **FONT** | command | Draw.c:5024 | t17 | p91 | t11 | P+C |  |
| **COLOUR** | command | Draw.c:5079 | t17 | p78, p81, p83 | t11 | P+C |  |
| &nbsp;&nbsp;MAP | sub-keyword | Draw.c:5081 | yes | - | - | P |  |
| **PAGE** | command | Draw.c:2592 | - | p115, p116, p81 | t11 | C |  |
| &nbsp;&nbsp;WRITE | sub-keyword | Draw.c:2598 | - | yes | yes | C |  |
| &nbsp;&nbsp;COPY | sub-keyword | Draw.c:2598 | - | yes | yes | C |  |
| &nbsp;&nbsp;SCROLL | sub-keyword | Draw.c:2598 | - | yes | yes | C |  |
| &nbsp;&nbsp;DISPLAY | sub-keyword | Draw.c:2600 | - | yes | - | C |  |
| &nbsp;&nbsp;FRAMEBUFFER | sub-keyword | Draw.c:2654 | - | - | - | U |  |
| &nbsp;&nbsp;RESIZE | sub-keyword | Draw.c:2666 | - | yes | - | C |  |
| &nbsp;&nbsp;MERGE | sub-keyword | Draw.c:2678 | - | - | - | U |  |
| &nbsp;&nbsp;STITCH | sub-keyword | Draw.c:2843 | - | yes | yes | C |  |
| &nbsp;&nbsp;AND_PIXELS | sub-keyword | Draw.c:2905 | - | yes | yes | C |  |
| &nbsp;&nbsp;OR_PIXELS | sub-keyword | Draw.c:2965 | - | yes | yes | C |  |
| &nbsp;&nbsp;XOR_PIXELS | sub-keyword | Draw.c:3025 | - | yes | yes | C |  |
| **ARC** | command | Draw.c:3597 | t17 | p78 | t11 | P+C |  |
| **POLYGON** | command | Draw.c:3958 | t17 | p119 | t11 | P+C |  |
| **MAP(** | command | Draw.c:2524 | t17, t18 | p102, p103, p143 | t11 | P+C |  |
| &nbsp;&nbsp;SET | sub-keyword | Draw.c:2526 | - | - | - | U |  |
| &nbsp;&nbsp;RESET | sub-keyword | Draw.c:2530 | - | - | - | U |  |
| &nbsp;&nbsp;GREYSCALE | sub-keyword | Draw.c:2532 | - | - | - | U |  |
| &nbsp;&nbsp;GRAYSCALE | sub-keyword | Draw.c:2532 | - | - | - | U |  |
| &nbsp;&nbsp;MAXIMITE | sub-keyword | Draw.c:2557 | - | - | - | U |  |
| **MAP** | command | Draw.c:2524 | t17, t18 | p102, p103, p143 | t11 | P+C |  |
| &nbsp;&nbsp;SET | sub-keyword | Draw.c:2526 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RESET | sub-keyword | Draw.c:2530 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;GREYSCALE | sub-keyword | Draw.c:2532 | yes | - | - | P |  |
| &nbsp;&nbsp;GRAYSCALE | sub-keyword | Draw.c:2532 | - | - | - | U |  |
| &nbsp;&nbsp;MAXIMITE | sub-keyword | Draw.c:2557 | yes | yes | yes | P+C |  |
| **IMAGE** | command | Draw.c:1798 | - | p96, p97 | t11 | C |  |
| &nbsp;&nbsp;ROTATE_FAST | sub-keyword | Draw.c:1801 | - | yes | yes | C |  |
| &nbsp;&nbsp;FRAMEBUFFER | sub-keyword | Draw.c:1811 | - | - | - | U |  |
| &nbsp;&nbsp;ROTATE | sub-keyword | Draw.c:1910 | - | yes | yes | C |  |
| &nbsp;&nbsp;WARP_H | sub-keyword | Draw.c:1949 | - | yes | - | C |  |
| &nbsp;&nbsp;WARP_V | sub-keyword | Draw.c:2089 | - | yes | - | C |  |
| &nbsp;&nbsp;RESIZE_FAST | sub-keyword | Draw.c:2228 | - | yes | yes | C |  |
| &nbsp;&nbsp;RESIZE | sub-keyword | Draw.c:2440 | - | yes | yes | C |  |
| **FRAMEBUFFER** | command | Draw.c:898 | t17 | p91, p92 | t11 | P+C |  |
| &nbsp;&nbsp;WINDOW | sub-keyword | Draw.c:904 | - | yes | yes | C |  |
| &nbsp;&nbsp;WRITE | sub-keyword | Draw.c:931 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | Draw.c:940 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;BACKUP | sub-keyword | Draw.c:954 | - | yes | yes | C |  |
| &nbsp;&nbsp;RESTORE | sub-keyword | Draw.c:971 | - | yes | yes | C |  |
| &nbsp;&nbsp;CREATE | sub-keyword | Draw.c:994 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CREATE 2 | sub-keyword | Rgb121.c:456 (Rgb121Framebuffer) | yes | - | - | P |  |
| &nbsp;&nbsp;LAYER TOP | sub-keyword | Rgb121.c:460 (Rgb121Framebuffer) | yes | - | - | P |  |
| &nbsp;&nbsp;LAYER | sub-keyword | Rgb121.c:467 (Rgb121Framebuffer) | yes | - | - | P |  |
| &nbsp;&nbsp;F | sub-keyword | Rgb121.c:476 (Rgb121Framebuffer) | - | - | - | U |  |
| &nbsp;&nbsp;L | sub-keyword | Rgb121.c:477 (Rgb121Framebuffer) | - | - | - | U |  |
| &nbsp;&nbsp;T | sub-keyword | Rgb121.c:478 (Rgb121Framebuffer) | - | - | - | U |  |
| &nbsp;&nbsp;2 | sub-keyword | Rgb121.c:479 (Rgb121Framebuffer) | yes | - | - | P |  |
| &nbsp;&nbsp;N | sub-keyword | Rgb121.c:485 (Rgb121Framebuffer) | - | - | - | U |  |
| &nbsp;&nbsp;MERGE | sub-keyword | Rgb121.c:508 (Rgb121Framebuffer) | yes | - | - | P |  |
| &nbsp;&nbsp;B | sub-keyword | Rgb121.c:517 (Rgb121Framebuffer) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;WAIT | sub-keyword | Rgb121.c:524 (Rgb121Framebuffer) | yes | - | - | P |  |
| &nbsp;&nbsp;COPY | sub-keyword | Rgb121.c:526 (Rgb121Framebuffer) | yes | - | - | P |  |
| **DRAW3D** | command | Draw.c:1334 | t17, t18 | p138, p89 | - | P+C |  |
| &nbsp;&nbsp;CREATE | sub-keyword | Draw.c:1336 | - | - | - | U |  |
| &nbsp;&nbsp;DIAGNOSE | sub-keyword | Draw.c:1559 | - | - | - | U |  |
| &nbsp;&nbsp;LIGHT | sub-keyword | Draw.c:1572 | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:1581 | - | yes | - | C |  |
| &nbsp;&nbsp;SET FLAGS | sub-keyword | Draw.c:1598 | - | - | - | U |  |
| &nbsp;&nbsp;ROTATE | sub-keyword | Draw.c:1615 | - | - | - | U |  |
| &nbsp;&nbsp;HIDE ALL | sub-keyword | Draw.c:1649 | - | - | - | U |  |
| &nbsp;&nbsp;RESET | sub-keyword | Draw.c:1660 | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:1675 | - | - | - | U |  |
| &nbsp;&nbsp;RESTORE | sub-keyword | Draw.c:1690 | - | - | - | U |  |
| &nbsp;&nbsp;WRITE | sub-keyword | Draw.c:1701 | - | yes | - | C |  |
| &nbsp;&nbsp;CLOSE ALL | sub-keyword | Draw.c:1718 | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | Draw.c:1721 | - | - | - | U |  |
| &nbsp;&nbsp;CAMERA | sub-keyword | Draw.c:1732 | - | - | - | U |  |
| **PIN(** | command | External.c:487 | t17, t18 | p116, p148 | t11, t14 | P+C |  |
| **SETPIN** | command | External.c:1006 | t17 | p124, p125 | t11 | P+C |  |
| &nbsp;&nbsp;OFF | sub-keyword | External.c:1011 | - | - | - | U |  |
| &nbsp;&nbsp;0 | sub-keyword | External.c:1011 | - | - | - | U |  |
| &nbsp;&nbsp;AIN | sub-keyword | External.c:1013 | - | - | - | U |  |
| &nbsp;&nbsp;DIN | sub-keyword | External.c:1015 | - | - | - | U |  |
| &nbsp;&nbsp;FIN | sub-keyword | External.c:1017 | - | yes | - | C |  |
| &nbsp;&nbsp;PIN | sub-keyword | External.c:1019 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CIN | sub-keyword | External.c:1021 | - | yes | - | C |  |
| &nbsp;&nbsp;INTH | sub-keyword | External.c:1023 | - | - | - | U |  |
| &nbsp;&nbsp;INTL | sub-keyword | External.c:1025 | - | - | - | U |  |
| &nbsp;&nbsp;DOUT | sub-keyword | External.c:1027 | yes | - | - | P |  |
| &nbsp;&nbsp;OOUT | sub-keyword | External.c:1029 | - | - | - | U |  |
| &nbsp;&nbsp;INTB | sub-keyword | External.c:1031 | - | - | - | U |  |
| &nbsp;&nbsp;PULLUP | sub-keyword | External.c:1046 | - | - | - | U |  |
| &nbsp;&nbsp;PULLDOWN | sub-keyword | External.c:1047 | - | - | - | U |  |
| &nbsp;&nbsp;OC | sub-keyword | External.c:1078 | - | - | - | U |  |
| **PULSE** | command | External.c:1112 | t17 | p120 | t11 | P+C |  |
| **PORT(** | command | External.c:945 | t17, t18 | p119, p149 | t11, t14 | P+C |  |
| **IR** | command | External.c:1195 | t17 | p97, p98 | t11 | P+C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | External.c:1198 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SEND | sub-keyword | External.c:1202 | yes | yes | yes | P+C |  |
| **SYNC** | command | External.c:1523 | t17 | - | - | P |  |
| &nbsp;&nbsp;U | sub-keyword | External.c:1537 | - | - | - | U |  |
| &nbsp;&nbsp;M | sub-keyword | External.c:1538 | - | - | - | U |  |
| &nbsp;&nbsp;S | sub-keyword | External.c:1539 | - | - | - | U |  |
| **BITBANG** | command | External.c:1548 | t17, t19 | p80 | t11 | P+C |  |
| &nbsp;&nbsp;WS2812 | sub-keyword | External.c:1550 | - | yes | yes | C |  |
| &nbsp;&nbsp;DHT22 | sub-keyword | External.c:1555 | - | - | - | U |  |
| &nbsp;&nbsp;BITSTREAM | sub-keyword | External.c:1560 | - | yes | yes | C |  |
| **HUMID** | command | External.c:1568 | t17 | p95 | - | P+C |  |
| **DHT22** | command | External.c:1568 | - | p154 | t11 | C |  |
| **WS2812** | command | External.c:1569 | t17 | p133 | - | P+C |  |
| **BITSTREAM** | command | External.c:1570 | t17 | p80 | - | P+C |  |
| **CTRLVAL(** | command | GUI.c:2338 | - | - | - | U |  |
| &nbsp;&nbsp;ON | sub-keyword | Draw.c:540 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OFF | sub-keyword | Draw.c:625 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:630 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:642 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OPEN | sub-keyword | MiscSTM32.c:882 (cmd_mouse) | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | MiscSTM32.c:1156 (cmd_mouse) | - | - | - | U |  |
| **I2C** | command | I2C.c:111 | t17 | p95, p96 | t11 | P+C |  |
| &nbsp;&nbsp;OPEN | sub-keyword | I2C.c:117 (cmd_i2c1) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | I2C.c:119 (cmd_i2c1) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;WRITE | sub-keyword | I2C.c:121 (cmd_i2c1) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;READ | sub-keyword | I2C.c:123 (cmd_i2c1) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CHECK | sub-keyword | I2C.c:125 (cmd_i2c1) | yes | - | - | P |  |
| **I2C2** | command | I2C.c:112 | t17 | - | - | P |  |
| &nbsp;&nbsp;OPEN | sub-keyword | I2C.c:133 (cmd_i2c2) | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | I2C.c:135 (cmd_i2c2) | - | - | - | U |  |
| &nbsp;&nbsp;WRITE | sub-keyword | I2C.c:137 (cmd_i2c2) | - | - | - | U |  |
| &nbsp;&nbsp;READ | sub-keyword | I2C.c:139 (cmd_i2c2) | - | - | - | U |  |
| &nbsp;&nbsp;CHECK | sub-keyword | I2C.c:141 (cmd_i2c2) | - | - | - | U |  |
| **I2C3** | command | I2C.c:113 | - | - | - | U |  |
| &nbsp;&nbsp;OPEN | sub-keyword | I2C.c:150 (cmd_i2c3) | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | I2C.c:152 (cmd_i2c3) | - | - | - | U |  |
| &nbsp;&nbsp;WRITE | sub-keyword | I2C.c:154 (cmd_i2c3) | - | - | - | U |  |
| &nbsp;&nbsp;READ | sub-keyword | I2C.c:156 (cmd_i2c3) | - | - | - | U |  |
| &nbsp;&nbsp;CHECK | sub-keyword | I2C.c:158 (cmd_i2c3) | - | - | - | U |  |
| **MATH** | command | MATHS.c:1458 | t17, t18 | p103, p104, p105, p106, p107, p108, p143 | t11, t14 | P+C |  |
| &nbsp;&nbsp;SET | sub-keyword | MATHS.c:1470 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SINC | sub-keyword | MATHS.c:1476 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;SCALE | sub-keyword | MATHS.c:1528 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SHIFT | sub-keyword | MATHS.c:1596 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;U | sub-keyword | MATHS.c:1615 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;SLICE | sub-keyword | MATHS.c:1629 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;SENSORFUSION | sub-keyword | MATHS.c:1635 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;CLAMP | sub-keyword | MATHS.c:1645 | yes | - | - | P |  |
| &nbsp;&nbsp;C_ADD | sub-keyword | MATHS.c:1699 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;C_MUL | sub-keyword | MATHS.c:1722 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;C_MULT | sub-keyword | MATHS.c:1723 | - | - | - | U |  |
| &nbsp;&nbsp;C_AND | sub-keyword | MATHS.c:1748 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;C_XOR | sub-keyword | MATHS.c:1770 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;C_OR | sub-keyword | MATHS.c:1792 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;C_SUB | sub-keyword | MATHS.c:1814 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;C_DIV | sub-keyword | MATHS.c:1837 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;V_MULT | sub-keyword | MATHS.c:1867 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;V_ROTATE | sub-keyword | MATHS.c:1902 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;V_NORMALISE | sub-keyword | MATHS.c:1947 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;V_CROSS | sub-keyword | MATHS.c:1973 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;V_PRINT | sub-keyword | MATHS.c:2001 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;HEX | sub-keyword | MATHS.c:2024 | yes | - | - | P |  |
| &nbsp;&nbsp;M_INVERSE | sub-keyword | MATHS.c:2048 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;M_TRANSPOSE | sub-keyword | MATHS.c:2095 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;M_MULT | sub-keyword | MATHS.c:2143 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;M_PRINT | sub-keyword | MATHS.c:2202 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;Q_INVERT | sub-keyword | MATHS.c:2262 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;Q_VECTOR | sub-keyword | MATHS.c:2280 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;Q_EULER | sub-keyword | MATHS.c:2304 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;Q_CREATE | sub-keyword | MATHS.c:2332 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;Q_MULT | sub-keyword | MATHS.c:2363 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;Q_ROTATE | sub-keyword | MATHS.c:2384 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;AES128 | sub-keyword | MATHS.c:2410 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ENCRYPT CBC | sub-keyword | MATHS.c:2421 | - | - | - | U |  |
| &nbsp;&nbsp;DECRYPT CBC | sub-keyword | MATHS.c:2432 | yes | - | - | P |  |
| &nbsp;&nbsp;ENCRYPT ECB | sub-keyword | MATHS.c:2441 | - | - | - | U |  |
| &nbsp;&nbsp;DECRYPT ECB | sub-keyword | MATHS.c:2454 | - | - | - | U |  |
| &nbsp;&nbsp;ENCRYPT CTR | sub-keyword | MATHS.c:2467 | - | - | - | U |  |
| &nbsp;&nbsp;DECRYPT CTR | sub-keyword | MATHS.c:2478 | - | - | - | U |  |
| &nbsp;&nbsp;PID | sub-keyword | MATHS.c:2489 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;START | sub-keyword | MATHS.c:2493 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;STOP | sub-keyword | MATHS.c:2502 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;INIT | sub-keyword | MATHS.c:2510 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ADD | sub-keyword | MATHS.c:2530 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;POWER | sub-keyword | MATHS.c:2536 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;WINDOW | sub-keyword | MATHS.c:2604 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;RANDOMIZE | sub-keyword | MATHS.c:2694 | yes | - | - | P |  |
| &nbsp;&nbsp;INTERPOLATE | sub-keyword | MATHS.c:2710 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;INSERT | sub-keyword | MATHS.c:2770 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;FFT | sub-keyword | MATHS.c:2776 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MADGWICK | sub-keyword | MATHS.c:4100 (cmd_SensorFusion) | - | - | - | U |  |
| &nbsp;&nbsp;MAHONY | sub-keyword | MATHS.c:4157 (cmd_SensorFusion) | - | - | - | U |  |
| &nbsp;&nbsp;MAGNITUDE | sub-keyword | MATHS.c:3987 (cmd_FFT) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PHASE | sub-keyword | MATHS.c:4015 (cmd_FFT) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;INVERSE | sub-keyword | MATHS.c:4043 (cmd_FFT) | yes | yes | yes | P+C |  |
| **SAVE** | command | FileIO.c:200 | t17 | p122, p123 | t11 | P+C |  |
| &nbsp;&nbsp;CONTEXT | sub-keyword | FileIO.c:212 | yes | - | - | P |  |
| &nbsp;&nbsp;CLEAR | sub-keyword | FileIO.c:214 | yes | - | - | P |  |
| &nbsp;&nbsp;IMAGE | sub-keyword | FileIO.c:219 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;DATA | sub-keyword | FileIO.c:271 | yes | yes | - | P+C |  |
| **LOAD** | command | FileIO.c:750 | t17 | p100 | t11 | P+C |  |
| &nbsp;&nbsp;CONTEXT | sub-keyword | FileIO.c:753 | yes | - | - | P |  |
| &nbsp;&nbsp;KEEP | sub-keyword | FileIO.c:755 | yes | - | - | P |  |
| &nbsp;&nbsp;BMP | sub-keyword | FileIO.c:760 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;IMAGE | sub-keyword | FileIO.c:761 | yes | - | - | P |  |
| &nbsp;&nbsp;JPG | sub-keyword | FileIO.c:766 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;GIF | sub-keyword | FileIO.c:771 | - | yes | yes | C |  |
| &nbsp;&nbsp;PNG | sub-keyword | FileIO.c:776 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FONT | sub-keyword | FileIO.c:781 | - | yes | yes | C |  |
| &nbsp;&nbsp;DATA | sub-keyword | FileIO.c:786 | yes | yes | - | P+C |  |
| **MKDIR** | command | FileIO.c:1028 | t17 | p109 | t11 | P+C |  |
| **RMDIR** | command | FileIO.c:1040 | t17 | p122 | t11 | P+C |  |
| **CHDIR** | command | FileIO.c:1210 | t17 | p82 | t11 | P+C |  |
| **KILL** | command | FileIO.c:1292 | t17 | p98 | t11 | P+C |  |
| &nbsp;&nbsp;ALL | sub-keyword | FileIO.c:1305 | yes | - | - | P |  |
| **SEEK** | command | FileIO.c:1355 | t17 | p123 | t11 | P+C |  |
| **FILES** | command | Commands.c:938 | t17 | p91 | t11 | P+C |  |
| &nbsp;&nbsp;NAME | sub-keyword | Commands.c:780 (do_listfiles) | - | - | - | U |  |
| &nbsp;&nbsp;TIME | sub-keyword | Commands.c:781 (do_listfiles) | - | - | - | U |  |
| &nbsp;&nbsp;SIZE | sub-keyword | Commands.c:782 (do_listfiles) | - | - | - | U |  |
| &nbsp;&nbsp;TYPE | sub-keyword | Commands.c:783 (do_listfiles) | - | - | - | U |  |
| **RENAME** | command | FileIO.c:1372 | t17 | p122 | t11 | P+C |  |
| **COPY** | command | FileIO.c:963 | t17 | p84 | t11 | P+C |  |
| **DAC** | command | MM_Custom.c:551 | - | p87 | t11 | C |  |
| &nbsp;&nbsp;STOP | sub-keyword | MM_Custom.c:560 | - | yes | yes | C |  |
| &nbsp;&nbsp;START | sub-keyword | MM_Custom.c:565 | - | yes | yes | C |  |
| **ADC** | command | MM_Custom.c:306 | t17 | p78 | t11 | P+C |  |
| &nbsp;&nbsp;OPEN | sub-keyword | MM_Custom.c:312 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;START | sub-keyword | MM_Custom.c:422 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | MM_Custom.c:487 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;TRIGGER | sub-keyword | MM_Custom.c:493 | - | yes | yes | C |  |
| &nbsp;&nbsp;FREQUENCY | sub-keyword | MM_Custom.c:510 | yes | yes | yes | P+C |  |
| **TURTLE** | command | turtle.c:755 | t17 | p132 | t13 | P+C |  |
| &nbsp;&nbsp;FORWARD | sub-keyword | turtle.c:758 | - | - | yes | C |  |
| &nbsp;&nbsp;FD | sub-keyword | turtle.c:758 | - | - | - | U |  |
| &nbsp;&nbsp;BACK | sub-keyword | turtle.c:762 | - | - | - | U |  |
| &nbsp;&nbsp;BACKWARD | sub-keyword | turtle.c:762 | - | - | yes | C |  |
| &nbsp;&nbsp;BK | sub-keyword | turtle.c:762 | - | - | - | U |  |
| &nbsp;&nbsp;RIGHT | sub-keyword | turtle.c:766 | - | - | yes | C |  |
| &nbsp;&nbsp;TURN RIGHT | sub-keyword | turtle.c:766 | - | - | yes | C |  |
| &nbsp;&nbsp;RT | sub-keyword | turtle.c:766 | - | - | - | U |  |
| &nbsp;&nbsp;LEFT | sub-keyword | turtle.c:776 | - | - | yes | C |  |
| &nbsp;&nbsp;TURN LEFT | sub-keyword | turtle.c:776 | - | - | yes | C |  |
| &nbsp;&nbsp;LT | sub-keyword | turtle.c:776 | - | - | - | U |  |
| &nbsp;&nbsp;PEN UP | sub-keyword | turtle.c:786 | - | - | yes | C |  |
| &nbsp;&nbsp;PENUP | sub-keyword | turtle.c:786 | - | - | - | U |  |
| &nbsp;&nbsp;PU | sub-keyword | turtle.c:786 | - | - | - | U |  |
| &nbsp;&nbsp;PEN DOWN | sub-keyword | turtle.c:790 | - | - | yes | C |  |
| &nbsp;&nbsp;PENDOWN | sub-keyword | turtle.c:790 | - | - | - | U |  |
| &nbsp;&nbsp;PD | sub-keyword | turtle.c:790 | - | - | - | U |  |
| &nbsp;&nbsp;PEN COLOUR | sub-keyword | turtle.c:794 | - | - | yes | C |  |
| &nbsp;&nbsp;PENCOLOR | sub-keyword | turtle.c:794 | - | - | - | U |  |
| &nbsp;&nbsp;PC | sub-keyword | turtle.c:794 | - | - | - | U |  |
| &nbsp;&nbsp;PEN WIDTH | sub-keyword | turtle.c:798 | - | - | - | U |  |
| &nbsp;&nbsp;PENWIDTH | sub-keyword | turtle.c:798 | - | - | - | U |  |
| &nbsp;&nbsp;PW | sub-keyword | turtle.c:798 | - | - | - | U |  |
| &nbsp;&nbsp;SET XY | sub-keyword | turtle.c:802 | - | - | - | U |  |
| &nbsp;&nbsp;SETXY | sub-keyword | turtle.c:802 | - | - | - | U |  |
| &nbsp;&nbsp;MOVE | sub-keyword | turtle.c:802 | - | - | yes | C |  |
| &nbsp;&nbsp;SET X | sub-keyword | turtle.c:809 | - | - | - | U |  |
| &nbsp;&nbsp;SETX | sub-keyword | turtle.c:809 | - | - | - | U |  |
| &nbsp;&nbsp;SET Y | sub-keyword | turtle.c:813 | - | - | - | U |  |
| &nbsp;&nbsp;SETY | sub-keyword | turtle.c:813 | - | - | - | U |  |
| &nbsp;&nbsp;SET HEADING | sub-keyword | turtle.c:817 | - | - | - | U |  |
| &nbsp;&nbsp;HEADING | sub-keyword | turtle.c:817 | - | - | yes | C |  |
| &nbsp;&nbsp;SETHEADING | sub-keyword | turtle.c:817 | - | - | - | U |  |
| &nbsp;&nbsp;SETH | sub-keyword | turtle.c:817 | - | - | - | U |  |
| &nbsp;&nbsp;HOME | sub-keyword | turtle.c:822 | - | - | - | U |  |
| &nbsp;&nbsp;RESET | sub-keyword | turtle.c:826 | - | - | yes | C |  |
| &nbsp;&nbsp;ARC | sub-keyword | turtle.c:834 | - | - | - | U |  |
| &nbsp;&nbsp;ARCL | sub-keyword | turtle.c:841 | - | - | - | U |  |
| &nbsp;&nbsp;ARCLEFT | sub-keyword | turtle.c:841 | - | - | - | U |  |
| &nbsp;&nbsp;ARC LEFT | sub-keyword | turtle.c:841 | - | - | - | U |  |
| &nbsp;&nbsp;ARCR | sub-keyword | turtle.c:848 | - | - | - | U |  |
| &nbsp;&nbsp;ARCRIGHT | sub-keyword | turtle.c:848 | - | - | - | U |  |
| &nbsp;&nbsp;ARC RIGHT | sub-keyword | turtle.c:848 | - | - | - | U |  |
| &nbsp;&nbsp;BEZIER | sub-keyword | turtle.c:855 | - | - | - | U |  |
| &nbsp;&nbsp;CIRCLE | sub-keyword | turtle.c:862 | - | - | - | U |  |
| &nbsp;&nbsp;DOT | sub-keyword | turtle.c:866 | - | - | yes | C |  |
| &nbsp;&nbsp;FCIRCLE | sub-keyword | turtle.c:874 | - | - | - | U |  |
| &nbsp;&nbsp;FRECTANGLE | sub-keyword | turtle.c:886 | - | - | - | U |  |
| &nbsp;&nbsp;FRECT | sub-keyword | turtle.c:886 | - | - | - | U |  |
| &nbsp;&nbsp;ARECTANGLE | sub-keyword | turtle.c:911 | - | - | - | U |  |
| &nbsp;&nbsp;ARECT | sub-keyword | turtle.c:911 | - | - | - | U |  |
| &nbsp;&nbsp;WEDGE | sub-keyword | turtle.c:918 | - | - | - | U |  |
| &nbsp;&nbsp;FILL COLOUR | sub-keyword | turtle.c:925 | - | - | yes | C |  |
| &nbsp;&nbsp;FILLCOLOR | sub-keyword | turtle.c:925 | - | - | - | U |  |
| &nbsp;&nbsp;FC | sub-keyword | turtle.c:925 | - | - | - | U |  |
| &nbsp;&nbsp;FILL PATTERN | sub-keyword | turtle.c:930 | - | - | - | U |  |
| &nbsp;&nbsp;FILLPATTERN | sub-keyword | turtle.c:930 | - | - | - | U |  |
| &nbsp;&nbsp;FP | sub-keyword | turtle.c:930 | - | - | - | U |  |
| &nbsp;&nbsp;NOFILL | sub-keyword | turtle.c:934 | - | - | - | U |  |
| &nbsp;&nbsp;NO FILL | sub-keyword | turtle.c:934 | - | - | - | U |  |
| &nbsp;&nbsp;BEGIN FILL | sub-keyword | turtle.c:938 | - | - | yes | C |  |
| &nbsp;&nbsp;BEGIN_FILL | sub-keyword | turtle.c:938 | - | - | - | U |  |
| &nbsp;&nbsp;BF | sub-keyword | turtle.c:938 | - | - | - | U |  |
| &nbsp;&nbsp;END FILL | sub-keyword | turtle.c:946 | - | - | yes | C |  |
| &nbsp;&nbsp;END_FILL | sub-keyword | turtle.c:946 | - | - | - | U |  |
| &nbsp;&nbsp;EF | sub-keyword | turtle.c:946 | - | - | - | U |  |
| &nbsp;&nbsp;PUSH | sub-keyword | turtle.c:955 | - | - | - | U |  |
| &nbsp;&nbsp;POP | sub-keyword | turtle.c:965 | - | - | - | U |  |
| &nbsp;&nbsp;SHOW TURTLE | sub-keyword | turtle.c:975 | - | - | - | U |  |
| &nbsp;&nbsp;SHOWTURTLE | sub-keyword | turtle.c:975 | - | - | - | U |  |
| &nbsp;&nbsp;ST | sub-keyword | turtle.c:975 | - | - | - | U |  |
| &nbsp;&nbsp;HIDE TURTLE | sub-keyword | turtle.c:980 | - | - | - | U |  |
| &nbsp;&nbsp;HIDETURTLE | sub-keyword | turtle.c:980 | - | - | - | U |  |
| &nbsp;&nbsp;HT | sub-keyword | turtle.c:980 | - | - | - | U |  |
| &nbsp;&nbsp;CURSOR SIZE | sub-keyword | turtle.c:988 | - | - | - | U |  |
| &nbsp;&nbsp;CURSORSIZE | sub-keyword | turtle.c:988 | - | - | - | U |  |
| &nbsp;&nbsp;CS | sub-keyword | turtle.c:988 | - | - | - | U |  |
| &nbsp;&nbsp;CURSOR COLOUR | sub-keyword | turtle.c:996 | - | - | - | U |  |
| &nbsp;&nbsp;CURSORCOLOR | sub-keyword | turtle.c:996 | - | - | - | U |  |
| &nbsp;&nbsp;CC | sub-keyword | turtle.c:996 | - | - | - | U |  |
| &nbsp;&nbsp;STAMP | sub-keyword | turtle.c:1004 | - | - | - | U |  |
| **MEMORY** | command | Memory.c:169 | t17 | p108, p109 | t11 | P+C |  |
| &nbsp;&nbsp;PACK | sub-keyword | Memory.c:172 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;PRINT | sub-keyword | Memory.c:230 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;INPUT | sub-keyword | Memory.c:254 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;UNPACK | sub-keyword | Memory.c:282 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;COPY | sub-keyword | Memory.c:342 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | Memory.c:344 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | Memory.c:366 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;SET | sub-keyword | Memory.c:397 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;BYTE | sub-keyword | Memory.c:400 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;SHORT | sub-keyword | Memory.c:410 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;WORD | sub-keyword | Memory.c:445 | yes | yes | - | P+C |  |
| **AUTOSAVE** | command | MM_Misc.c:3647 | t17 | p79, p80 | t11 | P+C |  |
| &nbsp;&nbsp;N | sub-keyword | MM_Misc.c:3661 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;DEFINE | sub-keyword | FileIO.c:586 (FileLoadProgram) | - | - | - | U |  |
| &nbsp;&nbsp;COLOR | sub-keyword | MMBasic.c:2009 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;SPRITE | sub-keyword | MMBasic.c:2012 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;WII | sub-keyword | MMBasic.c:2015 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;PIXEL FAST | sub-keyword | MMBasic.c:2018 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;ELSE IF | sub-keyword | MMBasic.c:2021 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;END IF | sub-keyword | MMBasic.c:2024 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;EXIT DO | sub-keyword | MMBasic.c:2027 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;CAT | sub-keyword | MMBasic.c:2030 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;AS | sub-keyword | MMBasic.c:2074 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;ON | sub-keyword | Draw.c:540 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OFF | sub-keyword | Draw.c:625 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:630 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:642 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | - | - | - | U |  |
| **OPTION** | command | MM_Misc.c:2420 | t16, t17 | p115, p125, p71, p72, p73, p74, p75, p76 | t10, t11 | P+C |  |
| &nbsp;&nbsp;LOCAL VARIABLES | sub-keyword | MM_Misc.c:2424 | yes | - | - | P |  |
| &nbsp;&nbsp;TRACECACHE | sub-keyword | MM_Misc.c:2429 | - | - | - | U |  |
| &nbsp;&nbsp;CACHE SUB | sub-keyword | MM_Misc.c:2429 | - | - | - | U |  |
| &nbsp;&nbsp;CACHE DEBUG | sub-keyword | MM_Misc.c:2429 | - | - | - | U |  |
| &nbsp;&nbsp;SYMBOLS | sub-keyword | MM_Misc.c:2433 | yes | - | - | P |  |
| &nbsp;&nbsp;ON | sub-keyword | MM_Misc.c:2439 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;OFF | sub-keyword | MM_Misc.c:2440 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;BASE | sub-keyword | MM_Misc.c:2450 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;ESCAPE | sub-keyword | MM_Misc.c:2457 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;EXPLICIT | sub-keyword | MM_Misc.c:2464 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;DEFAULT | sub-keyword | MM_Misc.c:2471 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | MM_Misc.c:2473 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | MM_Misc.c:2474 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;STRING | sub-keyword | MM_Misc.c:2475 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;NONE | sub-keyword | MM_Misc.c:2476 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;F11 | sub-keyword | MM_Misc.c:2479 | - | yes | yes | C |  |
| &nbsp;&nbsp;F12 | sub-keyword | MM_Misc.c:2489 | - | yes | yes | C |  |
| &nbsp;&nbsp;SEARCH PATH | sub-keyword | MM_Misc.c:2514 | - | yes | yes | C |  |
| &nbsp;&nbsp;DISABLE | sub-keyword | MM_Misc.c:2517 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CRLF | sub-keyword | MM_Misc.c:2531 | - | yes | yes | C |  |
| &nbsp;&nbsp;CR | sub-keyword | MM_Misc.c:2535 | - | - | - | U |  |
| &nbsp;&nbsp;LF | sub-keyword | MM_Misc.c:2536 | - | - | - | U |  |
| &nbsp;&nbsp;BREAK | sub-keyword | MM_Misc.c:2539 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MILLISECONDS | sub-keyword | MM_Misc.c:2544 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;LEGACY | sub-keyword | MM_Misc.c:2550 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;AUTORUN | sub-keyword | MM_Misc.c:2578 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;STATUS | sub-keyword | MM_Misc.c:2583 | - | yes | yes | C |  |
| &nbsp;&nbsp;CASE | sub-keyword | MM_Misc.c:2612 | yes | - | - | P |  |
| &nbsp;&nbsp;LOWER | sub-keyword | MM_Misc.c:2614 | yes | - | - | P |  |
| &nbsp;&nbsp;UPPER | sub-keyword | MM_Misc.c:2615 | yes | - | - | P |  |
| &nbsp;&nbsp;TITLE | sub-keyword | MM_Misc.c:2616 | yes | - | - | P |  |
| &nbsp;&nbsp;ANGLE | sub-keyword | MM_Misc.c:2618 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;DEGREES | sub-keyword | MM_Misc.c:2620 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;RADIANS | sub-keyword | MM_Misc.c:2621 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;Y_AXIS | sub-keyword | MM_Misc.c:2623 | - | yes | - | C |  |
| &nbsp;&nbsp;UP | sub-keyword | MM_Misc.c:2626 | - | yes | - | C |  |
| &nbsp;&nbsp;DOWN | sub-keyword | MM_Misc.c:2627 | - | yes | - | C |  |
| &nbsp;&nbsp;TAB | sub-keyword | MM_Misc.c:2631 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;2 | sub-keyword | MM_Misc.c:2633 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;3 | sub-keyword | MM_Misc.c:2634 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;4 | sub-keyword | MM_Misc.c:2635 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;8 | sub-keyword | MM_Misc.c:2636 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;BAUDRATE | sub-keyword | MM_Misc.c:2638 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;VCC | sub-keyword | MM_Misc.c:2651 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PIN | sub-keyword | MM_Misc.c:2661 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;COLOURCODE | sub-keyword | MM_Misc.c:2671 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;COLORCODE | sub-keyword | MM_Misc.c:2672 | - | - | - | U |  |
| &nbsp;&nbsp;REVERSE | sub-keyword | MM_Misc.c:2676 | - | yes | yes | C |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:630 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | yes | - | - | P |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:642 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;RESET | sub-keyword | MiscSTM32.c:1332 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;USBKEYBOARD | sub-keyword | MiscSTM32.c:1340 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;US | sub-keyword | MiscSTM32.c:1351 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;UK | sub-keyword | MiscSTM32.c:1352 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;DE | sub-keyword | MiscSTM32.c:1353 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;FR | sub-keyword | MiscSTM32.c:1354 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;ES | sub-keyword | MiscSTM32.c:1355 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;BE | sub-keyword | MiscSTM32.c:1356 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;SERIAL PULLUP | sub-keyword | MiscSTM32.c:1364 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;ENABLE | sub-keyword | MiscSTM32.c:1369 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MAXCTRLS | sub-keyword | MiscSTM32.c:1375 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;HORIZONTAL OFFSET | sub-keyword | MiscSTM32.c:1382 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;KEYBOARD REPEAT | sub-keyword | MiscSTM32.c:1389 (OtherOptions) | yes | - | yes | P+C |  |
| &nbsp;&nbsp;SLEEP | sub-keyword | MiscSTM32.c:1398 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;SD TIMING | sub-keyword | MiscSTM32.c:1407 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;NORMAL | sub-keyword | MiscSTM32.c:1410 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;FAST | sub-keyword | MiscSTM32.c:1412 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RAM | sub-keyword | MiscSTM32.c:1417 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;FLASH | sub-keyword | MiscSTM32.c:1423 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;MOUSE SENSITIVITY | sub-keyword | MiscSTM32.c:1434 (OtherOptions) | yes | - | - | P |  |
| &nbsp;&nbsp;MOUSE | sub-keyword | MiscSTM32.c:1443 (OtherOptions) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;RTC CALIBRATE | sub-keyword | MiscSTM32.c:1459 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;RTC DRIVE | sub-keyword | MiscSTM32.c:1474 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;DS3231 | sub-keyword | MiscSTM32.c:1481 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;PROFILING | sub-keyword | MiscSTM32.c:1492 (OtherOptions) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;VGA OUTPUT | sub-keyword | MiscSTM32.c:1502 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;HIGH | sub-keyword | MiscSTM32.c:1505 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;MEDIUM | sub-keyword | MiscSTM32.c:1507 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;LOW | sub-keyword | MiscSTM32.c:1509 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;USB POLLING | sub-keyword | MiscSTM32.c:1514 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;EDIT FONT | sub-keyword | MiscSTM32.c:1521 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;LARGE | sub-keyword | MiscSTM32.c:1524 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;SMALL | sub-keyword | MiscSTM32.c:1528 (OtherOptions) | - | yes | yes | C |  |
| &nbsp;&nbsp;VERY LARGE | sub-keyword | MiscSTM32.c:1530 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;OVERCLOCK | sub-keyword | MiscSTM32.c:1538 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;BASELINE | sub-keyword | MiscSTM32.c:1548 (OtherOptions) | - | yes | - | C |  |
| &nbsp;&nbsp;ERROR | sub-keyword | MiscSTM32.c:1558 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;CONTINUE | sub-keyword | MiscSTM32.c:1560 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;ABORT | sub-keyword | MiscSTM32.c:1564 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;DEFAULT MODE | sub-keyword | MiscSTM32.c:1569 (OtherOptions) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;1 | sub-keyword | MiscSTM32.c:1575 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;9 | sub-keyword | MiscSTM32.c:1581 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;10 | sub-keyword | MiscSTM32.c:1584 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;11 | sub-keyword | MiscSTM32.c:1587 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;12 | sub-keyword | MiscSTM32.c:1590 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;14 | sub-keyword | MiscSTM32.c:1593 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;15 | sub-keyword | MiscSTM32.c:1596 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;16 | sub-keyword | MiscSTM32.c:1599 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;18 | sub-keyword | MiscSTM32.c:1602 (OtherOptions) | - | - | - | U |  |
| &nbsp;&nbsp;CONSOLE | sub-keyword | MiscSTM32.c:1608 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;BOTH | sub-keyword | MiscSTM32.c:1612 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SCREEN | sub-keyword | MiscSTM32.c:1637 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PORT | sub-keyword | MiscSTM32.c:1640 (OtherOptions) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;SERIAL | sub-keyword | MiscSTM32.c:1644 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SAVE | sub-keyword | MiscSTM32.c:1651 (OtherOptions) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;LIST | sub-keyword | MiscSTM32.c:1657 (OtherOptions) | yes | yes | yes | P+C |  |
| **PAUSE** | command | MM_Misc.c:769 | t17 | p116 | t11 | P+C |  |
| **DATE$** | command | MM_Misc.c:2126 | t17, t18 | p137, p87 | t11, t14 | P+C |  |
| **TIME$** | command | MM_Misc.c:2268 | t17, t18 | p131, p153 | t13, t14 | P+C |  |
| **IRETURN** | command | MM_Misc.c:2362 | t17, t19 | p154 | t15 | P+C |  |
| **POKE** | command | MM_Misc.c:3417 | t17 | p118, p119 | t11 | P+C |  |
| &nbsp;&nbsp;BYTE | sub-keyword | MM_Misc.c:3422 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SHORT | sub-keyword | MM_Misc.c:3432 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;WORD | sub-keyword | MM_Misc.c:3443 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | MM_Misc.c:3454 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | MM_Misc.c:3464 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;VARTBL | sub-keyword | MM_Misc.c:3478 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;VAR | sub-keyword | MM_Misc.c:3479 | yes | yes | yes | P+C |  |
| **SETTICK** | command | MM_Misc.c:2390 | t17 | p126 | t11 | P+C |  |
| &nbsp;&nbsp;PAUSE | sub-keyword | MM_Misc.c:2398 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;RESUME | sub-keyword | MM_Misc.c:2401 | yes | yes | - | P+C |  |
| **WATCHDOG** | command | MM_Misc.c:3346 | t17 | p132 | t13 | P+C |  |
| &nbsp;&nbsp;OFF | sub-keyword | MM_Misc.c:3349 | yes | yes | yes | P+C |  |
| **CPU** | command | MM_Misc.c:3364 | t17 | p87 | t11 | P+C |  |
| &nbsp;&nbsp;RESTART | sub-keyword | MM_Misc.c:3369 | yes | yes | yes | P+C |  |
| **SORT** | command | MM_Misc.c:391 | t17 | p126 | t11 | P+C |  |
| **DEFINEFONT** | command | MM_Misc.c:3379 | t17 | p87 | t11 | P+C |  |
| **END DEFINEFONT** | command | Commands.c:93 | t17 | p87 | t11 | P+C |  |
| **LONGSTRING** | command | MM_Misc.c:828 | t17 | p101, p102 | t11 | P+C |  |
| &nbsp;&nbsp;SETBYTE | sub-keyword | MM_Misc.c:830 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;APPEND | sub-keyword | MM_Misc.c:846 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;TRIM | sub-keyword | MM_Misc.c:864 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;REPLACE | sub-keyword | MM_Misc.c:881 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;LOAD | sub-keyword | MM_Misc.c:898 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;LEFT | sub-keyword | MM_Misc.c:919 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RIGHT | sub-keyword | MM_Misc.c:938 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MID | sub-keyword | MM_Misc.c:959 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CLEAR | sub-keyword | MM_Misc.c:984 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RESIZE | sub-keyword | MM_Misc.c:993 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;UCASE | sub-keyword | MM_Misc.c:1003 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PRINT | sub-keyword | MM_Misc.c:1021 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;LCASE | sub-keyword | MM_Misc.c:1060 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;COPY | sub-keyword | MM_Misc.c:1077 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CONCAT | sub-keyword | MM_Misc.c:1096 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;AES128 | sub-keyword | MM_Misc.c:1117 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ENCRYPT CBC | sub-keyword | MM_Misc.c:1126 | - | yes | - | C |  |
| &nbsp;&nbsp;DECRYPT CBC | sub-keyword | MM_Misc.c:1138 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ENCRYPT ECB | sub-keyword | MM_Misc.c:1149 | - | - | - | U |  |
| &nbsp;&nbsp;DECRYPT ECB | sub-keyword | MM_Misc.c:1162 | - | - | - | U |  |
| &nbsp;&nbsp;ENCRYPT CTR | sub-keyword | MM_Misc.c:1175 | - | - | - | U |  |
| &nbsp;&nbsp;DECRYPT CTR | sub-keyword | MM_Misc.c:1187 | - | - | - | U |  |
| &nbsp;&nbsp;BASE64 | sub-keyword | MM_Misc.c:1200 | yes | - | - | P |  |
| &nbsp;&nbsp;ENCODE | sub-keyword | MM_Misc.c:1203 | yes | - | - | P |  |
| &nbsp;&nbsp;DECODE | sub-keyword | MM_Misc.c:1217 | yes | - | - | P |  |
| **MODE** | command | MiscSTM32.c:1295 | t17 | p110 | t11 | P+C |  |
| **CONTROLLER** | command | MiscSTM32.c:1190 | - | p84, p85 | - | C |  |
| &nbsp;&nbsp;NUNCHUK | sub-keyword | MiscSTM32.c:1192 | - | yes | - | C |  |
| &nbsp;&nbsp;MOUSE | sub-keyword | MiscSTM32.c:1197 | - | yes | - | C |  |
| &nbsp;&nbsp;CLASSIC | sub-keyword | MiscSTM32.c:1202 | - | yes | - | C |  |
| &nbsp;&nbsp;OPEN | sub-keyword | MiscSTM32.c:453 (cmd_nunchuck) | - | yes | - | C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | MiscSTM32.c:656 (cmd_nunchuck) | - | yes | - | C |  |
| **TIMER** | command | MM_Misc.c:706 | t17, t18 | p131, p153 | t13, t14 | P+C |  |
| **EDIT** | command | Editor.c:1022 | t17 | p89 | t11 | P+C |  |
| **UPDATE FIRMWARE** | command | MM_Misc.c:2681 | t17 | p132 | t13 | P+C |  |
| **CSUB** | command | MM_Misc.c:3379 | t17 | p86 | t11 | P+C |  |
| **END CSUB** | command | Commands.c:93 | t17 | p86, p90 | t11 | P+C |  |
| **GUI** | command | GUI.c:466 | t17 | p93, p94 | - | P+C |  |
| &nbsp;&nbsp;PAGE | sub-keyword | GUI.c:469 | - | - | - | U |  |
| &nbsp;&nbsp;BUTTON | sub-keyword | GUI.c:474 | - | - | - | U |  |
| &nbsp;&nbsp;SWITCH | sub-keyword | GUI.c:480 | - | - | - | U |  |
| &nbsp;&nbsp;CHECKBOX | sub-keyword | GUI.c:486 | - | - | - | U |  |
| &nbsp;&nbsp;RADIO | sub-keyword | GUI.c:492 | - | - | - | U |  |
| &nbsp;&nbsp;LED | sub-keyword | GUI.c:499 | - | - | - | U |  |
| &nbsp;&nbsp;FRAME | sub-keyword | GUI.c:506 | - | - | - | U |  |
| &nbsp;&nbsp;NUMBERBOX | sub-keyword | GUI.c:512 | - | - | - | U |  |
| &nbsp;&nbsp;CANCEL | sub-keyword | GUI.c:514 | - | - | - | U |  |
| &nbsp;&nbsp;TEXTBOX | sub-keyword | GUI.c:525 | - | - | - | U |  |
| &nbsp;&nbsp;FORMATBOX | sub-keyword | GUI.c:538 | - | - | - | U |  |
| &nbsp;&nbsp;SPINBOX | sub-keyword | GUI.c:551 | - | - | - | U |  |
| &nbsp;&nbsp;DISPLAYBOX | sub-keyword | GUI.c:562 | - | - | - | U |  |
| &nbsp;&nbsp;CAPTION | sub-keyword | GUI.c:568 | - | - | - | U |  |
| &nbsp;&nbsp;GAUGE | sub-keyword | GUI.c:574 | - | - | - | U |  |
| &nbsp;&nbsp;BARGAUGE | sub-keyword | GUI.c:580 | - | - | - | U |  |
| &nbsp;&nbsp;AREA | sub-keyword | GUI.c:586 | - | - | - | U |  |
| &nbsp;&nbsp;DELETE | sub-keyword | GUI.c:592 | - | - | - | U |  |
| &nbsp;&nbsp;ALL | sub-keyword | GUI.c:598 | - | - | - | U |  |
| &nbsp;&nbsp;DISABLE | sub-keyword | GUI.c:621 | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | GUI.c:642 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ENABLE | sub-keyword | GUI.c:663 | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | GUI.c:688 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;RESTORE | sub-keyword | GUI.c:713 | - | - | - | U |  |
| &nbsp;&nbsp;REDRAW | sub-keyword | GUI.c:738 | - | - | - | U |  |
| &nbsp;&nbsp;FCOLOUR | sub-keyword | GUI.c:763 | - | - | - | U |  |
| &nbsp;&nbsp;BCOLOUR | sub-keyword | GUI.c:763 | - | - | - | U |  |
| &nbsp;&nbsp;BEEP | sub-keyword | GUI.c:784 | - | - | - | U |  |
| &nbsp;&nbsp;INTERRUPT | sub-keyword | GUI.c:791 | - | - | - | U |  |
| &nbsp;&nbsp;SETUP | sub-keyword | GUI.c:810 | - | - | - | U |  |
| &nbsp;&nbsp;DATE1 | sub-keyword | GUI.c:319 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;DATE2 | sub-keyword | GUI.c:320 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;DATE3 | sub-keyword | GUI.c:321 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;TIME1 | sub-keyword | GUI.c:322 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;TIME2 | sub-keyword | GUI.c:323 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;TIME3 | sub-keyword | GUI.c:324 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;TIME4 | sub-keyword | GUI.c:325 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;DATETIME1 | sub-keyword | GUI.c:326 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;DATETIME2 | sub-keyword | GUI.c:327 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;DATETIME3 | sub-keyword | GUI.c:328 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;DATETIME4 | sub-keyword | GUI.c:329 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;LAT1 | sub-keyword | GUI.c:330 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;LAT2 | sub-keyword | GUI.c:331 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;LONG1 | sub-keyword | GUI.c:332 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;LONG2 | sub-keyword | GUI.c:333 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;ANGLE1 | sub-keyword | GUI.c:334 (GetCtrlParams) | - | - | - | U |  |
| &nbsp;&nbsp;CURSOR | sub-keyword | MM_Misc.c:592 (cmd_guiMX170) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;BITMAP | sub-keyword | MM_Misc.c:596 (cmd_guiMX170) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ON | sub-keyword | Draw.c:540 (cmd_cursor) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;OFF | sub-keyword | Draw.c:625 (cmd_cursor) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | yes | yes | - | P+C |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | yes | - | - | P |  |
| **ONEWIRE** | command | Onewire.c:116 | t17 | p113 | t11 | P+C |  |
| &nbsp;&nbsp;RESET | sub-keyword | Onewire.c:119 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;WRITE | sub-keyword | Onewire.c:121 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;READ | sub-keyword | Onewire.c:123 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SEARCH | sub-keyword | Onewire.c:126 | - | - | - | U |  |
| **TEMPR START** | command | Onewire.c:169 | t17 | p130 | t13 | P+C |  |
| **PWM** | command | PWM.c:77 | t17 | p121 | t11 | P+C |  |
| &nbsp;&nbsp;STOP | sub-keyword | PWM.c:91 | - | yes | yes | C |  |
| **SERVO** | command | PWM.c:77 | t17 | p124 | t11 | P+C |  |
| &nbsp;&nbsp;STOP | sub-keyword | PWM.c:91 | - | yes | yes | C |  |
| **OPEN** | command | SerialFileIO.c:76 | t17 | p113, p114, p84, p85 | t11 | P+C |  |
| **CLOSE** | command | SerialFileIO.c:130 | t17 | p83, p84, p85 | t11 | P+C |  |
| **SPI** | command | SPI.c:372 | t17, t18 | p126, p150 | t11, t14 | P+C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | SPI.c:74 (cmd_spi1) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;WRITE | sub-keyword | SPI.c:79 (cmd_spi1) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;READ | sub-keyword | SPI.c:94 (cmd_spi1) | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;OPEN | sub-keyword | SPI.c:110 (cmd_spi1) | yes | yes | yes | P+C |  |
| **SPI2** | command | SPI.c:373 | t17, t18 | p150 | t14 | P+C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | SPI.c:193 (cmd_spi2) | - | - | - | U |  |
| &nbsp;&nbsp;WRITE | sub-keyword | SPI.c:198 (cmd_spi2) | - | - | - | U |  |
| &nbsp;&nbsp;READ | sub-keyword | SPI.c:213 (cmd_spi2) | - | - | - | U |  |
| &nbsp;&nbsp;OPEN | sub-keyword | SPI.c:229 (cmd_spi2) | - | - | - | U |  |
| **BLIT** | command | sprites.c:1327 | t17 | p80, p81 | t11 | P+C |  |
| &nbsp;&nbsp;SHOW SAFE | sub-keyword | sprites.c:1334 | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | sprites.c:1382 | - | - | - | U |  |
| &nbsp;&nbsp;HIDE ALL | sub-keyword | sprites.c:1419 | - | - | - | U |  |
| &nbsp;&nbsp;RESTORE | sub-keyword | sprites.c:1435 | - | - | - | U |  |
| &nbsp;&nbsp;HIDE SAFE | sub-keyword | sprites.c:1460 | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | sprites.c:1479 | - | - | - | U |  |
| &nbsp;&nbsp;SWAP | sub-keyword | sprites.c:1508 | - | - | - | U |  |
| &nbsp;&nbsp;READ | sub-keyword | sprites.c:1548 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FRAMEBUFFER | sub-keyword | sprites.c:1583 | yes | - | - | P |  |
| &nbsp;&nbsp;COPY | sub-keyword | sprites.c:1597 | - | - | - | U |  |
| &nbsp;&nbsp;LOADARRAY | sub-keyword | sprites.c:1636 | - | - | - | U |  |
| &nbsp;&nbsp;LOADPNG | sub-keyword | sprites.c:1639 | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | sprites.c:1642 | yes | - | - | P |  |
| &nbsp;&nbsp;INTERRUPT | sub-keyword | sprites.c:1645 | - | - | - | U |  |
| &nbsp;&nbsp;NOINTERRUPT | sub-keyword | sprites.c:1651 | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE ALL | sub-keyword | sprites.c:1655 | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | sprites.c:1658 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;TRANSPARENCY | sub-keyword | sprites.c:1697 | - | - | - | U |  |
| &nbsp;&nbsp;NEXT | sub-keyword | sprites.c:1734 | - | - | - | U |  |
| &nbsp;&nbsp;WRITE | sub-keyword | sprites.c:1742 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MOVE | sub-keyword | sprites.c:1766 | - | - | - | U |  |
| &nbsp;&nbsp;SCROLLR | sub-keyword | sprites.c:1802 | - | - | - | U |  |
| &nbsp;&nbsp;SCROLL | sub-keyword | sprites.c:1957 | - | - | - | U |  |
| &nbsp;&nbsp;FLASH | sub-keyword | sprites.c:1213 (blitother) | yes | - | - | P |  |
| &nbsp;&nbsp;MEMORY | sub-keyword | sprites.c:1221 (blitother) | yes | - | - | P |  |
| &nbsp;&nbsp;2 | sub-keyword | ImageSlots.c:235 (BlitFlash) | - | - | - | U |  |
| **XMODEM** | command | XModem.c:71 | t17 | p133 | t13 | P+C |  |
| **ZMODEM** | command | ZModem.c:1249 | t17 | - | - | P |  |
| **CAN** | command | CAN.c:163 | - | p82, p88 | - | C |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | CAN.c:167 | - | yes | - | C |  |
| &nbsp;&nbsp;START | sub-keyword | CAN.c:188 | - | yes | - | C |  |
| &nbsp;&nbsp;STOP | sub-keyword | CAN.c:195 | - | yes | - | C |  |
| &nbsp;&nbsp;SEND | sub-keyword | CAN.c:207 | - | yes | - | C |  |
| &nbsp;&nbsp;READ | sub-keyword | CAN.c:323 | - | yes | - | C |  |
| &nbsp;&nbsp;FILTER | sub-keyword | CAN.c:454 | - | yes | - | C |  |
| &nbsp;&nbsp;GLOBAL | sub-keyword | CAN.c:456 | - | yes | - | C |  |
| &nbsp;&nbsp;OPEN | sub-keyword | CAN.c:547 | - | yes | - | C |  |
| **TYPE** | command | Struct.c:342 | t17 | p132, p85 | - | P+C |  |
| **END TYPE** | command | Struct.c:358 | t17 | p132 | - | P+C |  |
| **VAR** | command | flash.c:250 | t17 | p132, p88 | t11, t13 | P+C |  |
| &nbsp;&nbsp;CLEAR | sub-keyword | flash.c:257 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RESTORE | sub-keyword | flash.c:262 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SAVE | sub-keyword | flash.c:308 | yes | yes | yes | P+C |  |
| **STRUCT** | command | Struct.c:613 | t17, t18 | p129, p132, p152 | - | P+C |  |
| &nbsp;&nbsp;COPY | sub-keyword | Struct.c:616 | - | - | - | U |  |
| &nbsp;&nbsp;SORT | sub-keyword | Struct.c:724 | - | - | - | U |  |
| &nbsp;&nbsp;CLEAR | sub-keyword | Struct.c:964 | - | - | - | U |  |
| &nbsp;&nbsp;SWAP | sub-keyword | Struct.c:1001 | - | - | - | U |  |
| &nbsp;&nbsp;SAVE | sub-keyword | Struct.c:1056 | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Struct.c:1136 | - | - | - | U |  |
| &nbsp;&nbsp;PRINT | sub-keyword | Struct.c:1218 | - | - | - | U |  |
| &nbsp;&nbsp;EXTRACT | sub-keyword | Struct.c:1440 | - | - | - | U |  |
| &nbsp;&nbsp;INSERT | sub-keyword | Struct.c:1556 | - | - | - | U |  |
| **ARRAY SET** | command | Commands.c:3854 | t17 | p79 | - | P+C |  |
| **ARRAY ADD** | command | Commands.c:3855 | t17 | p79 | - | P+C |  |
| **ARRAY INSERT** | command | Commands.c:3856 | t17 | p79 | - | P+C |  |
| **ARRAY SLICE** | command | Commands.c:3857 | t17 | p79 | - | P+C |  |
| BITBYTEFLAG( | internal | Commands.c:2609 | - | - | - | - |  |
| **FLAGS** | command | Commands.c:2641 | t17 | p91 | - | P+C |  |
| **REDIM** | command | Commands.c:3624 | t17 | p122 | - | P+C |  |
| &nbsp;&nbsp;PRESERVE | sub-keyword | Commands.c:3628 | yes | yes | - | P+C |  |
| **BEZIER** | command | Draw.c:3737 | t17 | p80 | - | P+C |  |
| **FILL** | command | Draw.c:3351 | t17 | p91 | - | P+C |  |
| **INTERRUPT** | command | MM_Misc.c:665 | t17 | p97 | - | P+C |  |
| &nbsp;&nbsp;0 | sub-keyword | MM_Misc.c:668 | - | - | - | U |  |
| **CHAIN** | command | Commands.c:1352 | t17 | - | - | P |  |
| &nbsp;&nbsp;DEFINE | sub-keyword | FileIO.c:586 (FileLoadProgram) | - | - | - | U |  |
| **DRIVE** | command | FileIO.c:1162 | t17 | - | - | P |  |
| **LIBRARY** | command | Library.c:723 | t17 | - | - | P |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Library.c:725 | yes | - | - | P |  |
| &nbsp;&nbsp;SAVE | sub-keyword | Library.c:729 | yes | - | - | P |  |
| &nbsp;&nbsp;DELETE | sub-keyword | Library.c:734 | yes | - | - | P |  |
| &nbsp;&nbsp;LIST ALL | sub-keyword | Library.c:743 | yes | - | - | P |  |
| &nbsp;&nbsp;LIST | sub-keyword | Library.c:748 | yes | - | - | P |  |
| &nbsp;&nbsp;DISK SAVE | sub-keyword | Library.c:753 | yes | - | - | P |  |
| &nbsp;&nbsp;DISK LOAD | sub-keyword | Library.c:769 | yes | - | - | P |  |
| &nbsp;&nbsp;O | sub-keyword | Library.c:544 (LibLoad) | yes | - | - | P |  |
| &nbsp;&nbsp;OVERWRITE | sub-keyword | Library.c:544 (LibLoad) | - | - | - | U |  |
| &nbsp;&nbsp;RAM | sub-keyword | Library.c:545 (LibLoad) | yes | - | - | P |  |
| **MOUSE** | command | USBMouse.c:308 | t17 | p146, p94 | - | P+C |  |
| &nbsp;&nbsp;INTERRUPT ENABLE | sub-keyword | USBMouse.c:311 | yes | - | - | P |  |
| &nbsp;&nbsp;INTERRUPT DISABLE | sub-keyword | USBMouse.c:319 | yes | - | - | P |  |
| &nbsp;&nbsp;SET | sub-keyword | USBMouse.c:326 | yes | - | - | P |  |
| **TILEMAP** | command | TileMap.c:692 | t17 | - | - | P |  |
| &nbsp;&nbsp;CREATE | sub-keyword | TileMap.c:695 | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | TileMap.c:699 | - | - | - | U |  |
| &nbsp;&nbsp;ATTR | sub-keyword | TileMap.c:703 | - | - | - | U |  |
| &nbsp;&nbsp;DESTROY | sub-keyword | TileMap.c:707 | - | - | - | U |  |
| &nbsp;&nbsp;SET | sub-keyword | TileMap.c:711 | - | - | - | U |  |
| &nbsp;&nbsp;DRAW | sub-keyword | TileMap.c:715 | - | - | - | U |  |
| &nbsp;&nbsp;SCROLL | sub-keyword | TileMap.c:719 | - | - | - | U |  |
| &nbsp;&nbsp;VIEW | sub-keyword | TileMap.c:723 | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | TileMap.c:727 | - | - | - | U |  |
| &nbsp;&nbsp;SPRITE | sub-keyword | TileMap.c:732 | - | - | - | U |  |
| &nbsp;&nbsp;MOVE | sub-keyword | TileMap.c:737 | - | - | - | U |  |
| **FLASH** | command | ImageSlots.c:84 | t17 | - | - | P |  |
| &nbsp;&nbsp;LOAD IMAGE | sub-keyword | ImageSlots.c:86 | yes | - | - | P |  |
| &nbsp;&nbsp;O | sub-keyword | ImageSlots.c:92 | yes | - | - | P |  |
| &nbsp;&nbsp;OVERWRITE | sub-keyword | ImageSlots.c:92 | yes | - | - | P |  |
| &nbsp;&nbsp;DISK LOAD | sub-keyword | ImageSlots.c:134 | yes | - | - | P |  |
| **GAMEPAD** | command | USBGamepad.c:608 | t17 | - | - | P |  |
| &nbsp;&nbsp;INTERRUPT ENABLE | sub-keyword | USBGamepad.c:611 | yes | - | - | P |  |
| &nbsp;&nbsp;INTERRUPT DISABLE | sub-keyword | USBGamepad.c:619 | yes | - | - | P |  |
| &nbsp;&nbsp;MONITOR SILENT | sub-keyword | USBGamepad.c:625 | yes | - | - | P |  |
| &nbsp;&nbsp;MONITOR | sub-keyword | USBGamepad.c:627 | yes | - | - | P |  |
| &nbsp;&nbsp;CONFIGURE | sub-keyword | USBGamepad.c:629 | yes | - | - | P |  |
| &nbsp;&nbsp;HAPTIC | sub-keyword | USBGamepad.c:643 | yes | - | - | P |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | USBGamepad.c:650 | yes | - | - | P |  |
| &nbsp;&nbsp;COLOR | sub-keyword | USBGamepad.c:650 | - | - | - | U |  |
| **RANDOMIZE** | command | Commands.c:4234 | t17 | - | - | P |  |
| **FLUSH** | command | SerialFileIO.c:420 | t17 | - | - | P |  |
| **FM** | command | FileManager.c:3595 | t17 | - | - | P |  |
| &nbsp;&nbsp;CONTEXT | sub-keyword | FileIO.c:212 (cmd_save) | - | - | - | U |  |
| &nbsp;&nbsp;CLEAR | sub-keyword | FileIO.c:214 (cmd_save) | - | - | - | U |  |
| &nbsp;&nbsp;IMAGE | sub-keyword | FileIO.c:219 (cmd_save) | - | - | - | U |  |
| &nbsp;&nbsp;DATA | sub-keyword | FileIO.c:271 (cmd_save) | - | - | - | U |  |
| (LET) | internal | Commands.c:93 | - | - | - | - |  |
| **NOT** | operator | Operators.c:249 | - | p70 | t8 | C |  |
| **INV** | operator | Operators.c:254 | - | p70 | t8 | C |  |
| **+** | operator | Operators.c:127 | - | p70 | t8, t9 | C |  |
| **-** | operator | Operators.c:142 | - | p70 | t8 | C |  |
| **^** | operator | Operators.c:81 | - | p70 | t8 | C |  |
| ***** | operator | Operators.c:105 | - | p70 | t8 | C |  |
| **/** | operator | Operators.c:114 | - | p70 | t8 | C |  |
| **\** | operator | Operators.c:121 | - | p70 | t8 | C |  |
| **MOD** | operator | Operators.c:150 | - | p70 | t8 | C |  |
| **<<** | operator | Operators.c:216 | - | p70 | t8 | C |  |
| **>>>** | operator | Operators.c:222 | - | p70 | - | C |  |
| **>>** | operator | Operators.c:227 | - | p70 | t8 | C |  |
| **<>** | operator | Operators.c:179 | - | p70 | t8, t9 | C |  |
| **>=** | operator | Operators.c:188 | - | p70 | t8, t9 | C |  |
| **<=** | operator | Operators.c:193 | - | p70 | t8, t9 | C |  |
| **<** | operator | Operators.c:198 | - | p70 | t8, t9 | C |  |
| **>** | operator | Operators.c:203 | - | p70 | t8, t9 | C |  |
| **=** | operator | Operators.c:208 | - | p70 | t8, t9 | C |  |
| **AND** | operator | Operators.c:233 | - | p70 | t8 | C |  |
| **OR** | operator | Operators.c:238 | - | p70 | t8, t9 | C |  |
| **XOR** | operator | Operators.c:243 | - | p70 | t8 | C |  |
| **RGB(** | function | Draw.c:4935 | t18 | p149 | t14 | P+C |  |
| &nbsp;&nbsp;WHITE | sub-keyword | Draw.c:4940 | - | - | - | U |  |
| &nbsp;&nbsp;BLACK | sub-keyword | Draw.c:4941 | - | - | - | U |  |
| &nbsp;&nbsp;NOTBLACK | sub-keyword | Draw.c:4942 | - | - | - | U |  |
| &nbsp;&nbsp;BLUE | sub-keyword | Draw.c:4943 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;GREEN | sub-keyword | Draw.c:4944 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CYAN | sub-keyword | Draw.c:4945 | - | - | - | U |  |
| &nbsp;&nbsp;RED | sub-keyword | Draw.c:4946 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MAGENTA | sub-keyword | Draw.c:4947 | - | - | - | U |  |
| &nbsp;&nbsp;YELLOW | sub-keyword | Draw.c:4948 | - | - | - | U |  |
| &nbsp;&nbsp;BROWN | sub-keyword | Draw.c:4949 | - | - | - | U |  |
| &nbsp;&nbsp;GRAY | sub-keyword | Draw.c:4950 | - | - | - | U |  |
| &nbsp;&nbsp;GREY | sub-keyword | Draw.c:4951 | - | - | - | U |  |
| &nbsp;&nbsp;LIGHTGRAY | sub-keyword | Draw.c:4952 | - | - | - | U |  |
| &nbsp;&nbsp;LIGHTGREY | sub-keyword | Draw.c:4953 | - | - | - | U |  |
| &nbsp;&nbsp;ORANGE | sub-keyword | Draw.c:4954 | - | - | - | U |  |
| &nbsp;&nbsp;PINK | sub-keyword | Draw.c:4955 | - | - | - | U |  |
| &nbsp;&nbsp;GOLD | sub-keyword | Draw.c:4956 | - | - | - | U |  |
| &nbsp;&nbsp;SALMON | sub-keyword | Draw.c:4957 | - | - | - | U |  |
| &nbsp;&nbsp;LILAC | sub-keyword | Draw.c:4958 | - | - | - | U |  |
| &nbsp;&nbsp;FUCHSIA | sub-keyword | Draw.c:4959 | - | - | - | U |  |
| &nbsp;&nbsp;RUST | sub-keyword | Draw.c:4960 | - | - | - | U |  |
| &nbsp;&nbsp;CERULEAN | sub-keyword | Draw.c:4961 | - | - | - | U |  |
| &nbsp;&nbsp;MIDGREEN | sub-keyword | Draw.c:4962 | - | - | - | U |  |
| &nbsp;&nbsp;COBALT | sub-keyword | Draw.c:4963 | - | - | - | U |  |
| &nbsp;&nbsp;MYRTLE | sub-keyword | Draw.c:4964 | - | - | - | U |  |
| &nbsp;&nbsp;BEIGE | sub-keyword | Draw.c:4965 | - | - | - | U |  |
| **PIXEL(** | function | Draw.c:4801 | t17, t18 | p116, p149 | t11, t14 | P+C |  |
| &nbsp;&nbsp;FRAMEBUFFER | sub-keyword | Draw.c:4813 | - | - | - | U |  |
| **DRAW3D(** | function | Draw.c:1753 | t17, t18 | p138, p89 | - | P+C |  |
| &nbsp;&nbsp;XMIN | sub-keyword | Draw.c:1755 | - | - | - | U |  |
| &nbsp;&nbsp;XMAX | sub-keyword | Draw.c:1760 | - | - | - | U |  |
| &nbsp;&nbsp;YMIN | sub-keyword | Draw.c:1765 | - | - | - | U |  |
| &nbsp;&nbsp;YMAX | sub-keyword | Draw.c:1770 | - | - | - | U |  |
| &nbsp;&nbsp;X | sub-keyword | Draw.c:1775 | - | - | - | U |  |
| &nbsp;&nbsp;Y | sub-keyword | Draw.c:1780 | - | - | - | U |  |
| &nbsp;&nbsp;DISTANCE | sub-keyword | Draw.c:1785 | - | - | - | U |  |
| &nbsp;&nbsp;Z | sub-keyword | Draw.c:1790 | - | - | - | U |  |
| **MAP(** | function | Draw.c:2514 | t17, t18 | p102, p103, p143 | t11 | P+C |  |
| **GETSCANLINE** | function | Draw.c:2501 | t18 | p140 | t14 | P+C |  |
| **@(** | function | Draw.c:1015 | - | - | - | U |  |
| **PIN(** | function | External.c:525 | t17, t18 | p116, p148 | t11, t14 | P+C |  |
| **PORT(** | function | External.c:978 | t17, t18 | p119, p149 | t11, t14 | P+C |  |
| **DISTANCE(** | function | External.c:1833 | t18 | p138 | t14 | P+C |  |
| **PULSIN(** | function | External.c:1157 | t18 | p149 | t14 | P+C |  |
| **CWD$** | function | FileIO.c:1256 | t18 | p137 | t14 | P+C |  |
| **DIR$(** | function | FileIO.c:819 | t18 | p138 | t14 | P+C |  |
| &nbsp;&nbsp;DIR | sub-keyword | FileIO.c:830 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FILE | sub-keyword | FileIO.c:832 | - | - | - | U |  |
| &nbsp;&nbsp;ALL | sub-keyword | FileIO.c:834 | - | - | - | U |  |
| **ACOS(** | function | Functions.c:1430 | t18 | p135 | t14 | P+C |  |
| **ABS(** | function | Functions.c:530 | t18 | p135 | t14 | P+C |  |
| **ASC(** | function | Functions.c:640 | t18 | p135 | t14 | P+C |  |
| **ASIN(** | function | Functions.c:1413 | t18 | p135 | t14 | P+C |  |
| **ATN(** | function | Functions.c:654 | t18 | p135 | t14 | P+C |  |
| **ATAN2(** | function | Functions.c:659 | t18 | p135 | t14 | P+C |  |
| **BASE$(** | function | Functions.c:714 | t18 | p135 | - | P+C |  |
| **BOUND(** | function | Functions.c:289 | t18 | p136 | t14 | P+C |  |
| **CHR$(** | function | Functions.c:672 | t18 | p136 | t14 | P+C |  |
| **CHOICE(** | function | Functions.c:1464 | t18 | p136 | - | P+C |  |
| **CINT(** | function | Functions.c:683 | t18 | p136 | t14 | P+C |  |
| **COS(** | function | Functions.c:691 | t18 | p137 | t14 | P+C |  |
| **DEG(** | function | Functions.c:698 | t18 | p138 | t14 | P+C |  |
| **EXP(** | function | Functions.c:706 | t18 | p139 | t14 | P+C |  |
| **FIX(** | function | Functions.c:910 | t18 | p139 | t14 | P+C |  |
| **FIELD$(** | function | Functions.c:343 | t18 | p139 | t14 | P+C |  |
| **INKEY$** | function | Functions.c:1390 | t18 | p141 | t14 | P+C |  |
| **INSTR(** | function | Functions.c:758 | t18 | p141 | t14 | P+C |  |
| **INT(** | function | Functions.c:902 | t18 | p141 | t14 | P+C |  |
| **LEN(** | function | Functions.c:1103 | t18 | p142 | t14 | P+C |  |
| **LOG(** | function | Functions.c:1112 | t18 | p143 | t14 | P+C |  |
| **MID$(** | function | Functions.c:1125 | t17, t18 | p109, p146 | t11, t14 | P+C |  |
| **PI** | function | Functions.c:1149 | t18 | p148 | t14 | P+C |  |
| **POS** | function | Functions.c:1358 | t19 | p154 | t15 | P+C |  |
| **RAD(** | function | Functions.c:1158 | t18 | p149 | t14 | P+C |  |
| SCHANGE$( | internal | Functions.c:1075 | - | - | - | - |  |
| **RND(** | function | Functions.c:1166 | t18 | p149 | t14 | P+C |  |
| **RND** | function | Functions.c:1166 | t18 | p149, p150 | t14 | P+C |  |
| **SGN(** | function | Functions.c:1175 | t18 | p150 | t14 | P+C |  |
| **SIN(** | function | Functions.c:1191 | t18 | p150 | t14 | P+C |  |
| **SPACE$(** | function | Functions.c:1277 | t18 | p150 | t14 | P+C |  |
| **SQR(** | function | Functions.c:1200 | t18 | p152 | t14 | P+C |  |
| **STR$(** | function | Functions.c:1289 | t18 | p152 | t14 | P+C |  |
| **STRING$(** | function | Functions.c:1320 | t18 | p152 | t14 | P+C |  |
| **TAB(** | function | Functions.c:1367 | t18 | p152 | t14 | P+C |  |
| **TAN(** | function | Functions.c:1212 | t18 | p152 | t14 | P+C |  |
| **VAL(** | function | Functions.c:1221 | t18 | p153 | t14 | P+C |  |
| **EVAL(** | function | Functions.c:1227 | t18 | p139 | t14 | P+C |  |
| &nbsp;&nbsp;COLOR | sub-keyword | MMBasic.c:2009 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;SPRITE | sub-keyword | MMBasic.c:2012 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;WII | sub-keyword | MMBasic.c:2015 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;PIXEL FAST | sub-keyword | MMBasic.c:2018 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;ELSE IF | sub-keyword | MMBasic.c:2021 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;END IF | sub-keyword | MMBasic.c:2024 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;EXIT DO | sub-keyword | MMBasic.c:2027 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;CAT | sub-keyword | MMBasic.c:2030 (tokenise) | - | - | - | U |  |
| &nbsp;&nbsp;AS | sub-keyword | MMBasic.c:2074 (tokenise) | - | - | - | U |  |
| TOPBOTTOM( | internal | Functions.c:1447 | - | - | - | - |  |
| **BIN2STR$(** | function | Functions.c:445 | t18 | p135 | t14 | P+C |  |
| &nbsp;&nbsp;BIG | sub-keyword | Functions.c:462 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;DOUBLE | sub-keyword | Functions.c:464 | - | - | - | U |  |
| &nbsp;&nbsp;SINGLE | sub-keyword | Functions.c:467 | - | - | - | U |  |
| &nbsp;&nbsp;INT64 | sub-keyword | Functions.c:472 | - | - | - | U |  |
| &nbsp;&nbsp;INT32 | sub-keyword | Functions.c:475 | - | - | - | U |  |
| &nbsp;&nbsp;INT16 | sub-keyword | Functions.c:479 | - | - | - | U |  |
| &nbsp;&nbsp;INT8 | sub-keyword | Functions.c:483 | - | - | - | U |  |
| &nbsp;&nbsp;UINT64 | sub-keyword | Functions.c:487 | - | - | - | U |  |
| &nbsp;&nbsp;UINT32 | sub-keyword | Functions.c:490 | - | - | - | U |  |
| &nbsp;&nbsp;UINT16 | sub-keyword | Functions.c:494 | - | - | - | U |  |
| &nbsp;&nbsp;UINT8 | sub-keyword | Functions.c:498 | - | - | - | U |  |
| **STR2BIN(** | function | Functions.c:368 | t18 | p151 | t14 | P+C |  |
| &nbsp;&nbsp;BIG | sub-keyword | Functions.c:384 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;DOUBLE | sub-keyword | Functions.c:400 | - | - | - | U |  |
| &nbsp;&nbsp;SINGLE | sub-keyword | Functions.c:404 | - | - | - | U |  |
| &nbsp;&nbsp;INT64 | sub-keyword | Functions.c:408 | - | - | - | U |  |
| &nbsp;&nbsp;INT32 | sub-keyword | Functions.c:412 | - | - | - | U |  |
| &nbsp;&nbsp;INT16 | sub-keyword | Functions.c:416 | - | - | - | U |  |
| &nbsp;&nbsp;INT8 | sub-keyword | Functions.c:420 | - | - | - | U |  |
| &nbsp;&nbsp;UINT64 | sub-keyword | Functions.c:424 | - | - | - | U |  |
| &nbsp;&nbsp;UINT32 | sub-keyword | Functions.c:428 | - | - | - | U |  |
| &nbsp;&nbsp;UINT16 | sub-keyword | Functions.c:432 | - | - | - | U |  |
| &nbsp;&nbsp;UINT8 | sub-keyword | Functions.c:436 | - | - | - | U |  |
| **CALL(** | function | Functions.c:306 | t17, t18 | p136, p82 | - | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | Commands.c:3308 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;STRING | sub-keyword | Commands.c:3310 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | Commands.c:3312 (CheckIfTypeSpecified) | - | - | - | U |  |
| &nbsp;&nbsp;BYVAL | sub-keyword | MMBasic.c:866 (SubLayoutBuild) | - | - | - | U |  |
| &nbsp;&nbsp;BY VAL | sub-keyword | MMBasic.c:866 (SubLayoutBuild) | - | - | - | U |  |
| &nbsp;&nbsp;BYREF | sub-keyword | MMBasic.c:867 (SubLayoutBuild) | - | - | - | U |  |
| &nbsp;&nbsp;BY REF | sub-keyword | MMBasic.c:867 (SubLayoutBuild) | - | - | - | U |  |
| **MSGBOX(** | function | GUI.c:2219 | - | - | - | U |  |
| &nbsp;&nbsp;ON | sub-keyword | Draw.c:540 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OFF | sub-keyword | Draw.c:625 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:630 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:642 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OPEN | sub-keyword | MiscSTM32.c:882 (cmd_mouse) | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | MiscSTM32.c:1156 (cmd_mouse) | - | - | - | U |  |
| **CTRLVAL(** | function | GUI.c:2304 | - | - | - | U |  |
| &nbsp;&nbsp;ON | sub-keyword | Draw.c:540 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;COLOUR | sub-keyword | Draw.c:599 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OFF | sub-keyword | Draw.c:625 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;HIDE | sub-keyword | Draw.c:630 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LOAD | sub-keyword | Draw.c:636 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;SHOW | sub-keyword | Draw.c:642 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;LINK MOUSE | sub-keyword | Draw.c:649 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;UNLINK MOUSE | sub-keyword | Draw.c:658 (cmd_cursor) | - | - | - | U |  |
| &nbsp;&nbsp;OPEN | sub-keyword | MiscSTM32.c:882 (cmd_mouse) | - | - | - | U |  |
| &nbsp;&nbsp;CLOSE | sub-keyword | MiscSTM32.c:1156 (cmd_mouse) | - | - | - | U |  |
| **CLICK(** | function | GUI.c:220 | t18 | - | - | P |  |
| &nbsp;&nbsp;X | sub-keyword | GUI.c:221 | yes | - | - | P |  |
| &nbsp;&nbsp;Y | sub-keyword | GUI.c:223 | yes | - | - | P |  |
| &nbsp;&nbsp;REF | sub-keyword | GUI.c:225 | yes | - | - | P |  |
| &nbsp;&nbsp;LASTREF | sub-keyword | GUI.c:227 | yes | - | - | P |  |
| &nbsp;&nbsp;LASTX | sub-keyword | GUI.c:229 | yes | - | - | P |  |
| &nbsp;&nbsp;LASTY | sub-keyword | GUI.c:231 | yes | - | - | P |  |
| &nbsp;&nbsp;DOWN | sub-keyword | GUI.c:233 | yes | - | - | P |  |
| &nbsp;&nbsp;UP | sub-keyword | GUI.c:235 | yes | - | - | P |  |
| **MATH(** | function | MATHS.c:2808 | t17, t18 | p103, p143, p144, p145, p146 | t11, t14 | P+C |  |
| &nbsp;&nbsp;PID | sub-keyword | MATHS.c:2814 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ATAN3 | sub-keyword | MATHS.c:2835 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;REAL | sub-keyword | MATHS.c:2858 | - | - | - | U |  |
| &nbsp;&nbsp;IMAG | sub-keyword | MATHS.c:2863 | - | - | - | U |  |
| &nbsp;&nbsp;MOD | sub-keyword | MATHS.c:2868 | - | - | - | U |  |
| &nbsp;&nbsp;PHASE | sub-keyword | MATHS.c:2878 | - | - | - | U |  |
| &nbsp;&nbsp;CARG | sub-keyword | MATHS.c:2887 | - | - | - | U |  |
| &nbsp;&nbsp;ADD | sub-keyword | MATHS.c:2892 | - | - | - | U |  |
| &nbsp;&nbsp;MUL | sub-keyword | MATHS.c:2898 | - | - | - | U |  |
| &nbsp;&nbsp;SUB | sub-keyword | MATHS.c:2904 | - | - | - | U |  |
| &nbsp;&nbsp;DIV | sub-keyword | MATHS.c:2910 | - | - | - | U |  |
| &nbsp;&nbsp;POW | sub-keyword | MATHS.c:2916 | - | - | - | U |  |
| &nbsp;&nbsp;CONJ | sub-keyword | MATHS.c:2922 | - | - | - | U |  |
| &nbsp;&nbsp;ASINH | sub-keyword | MATHS.c:2927 | - | - | - | U |  |
| &nbsp;&nbsp;ACOSH | sub-keyword | MATHS.c:2932 | - | - | - | U |  |
| &nbsp;&nbsp;ATANH | sub-keyword | MATHS.c:2937 | - | - | - | U |  |
| &nbsp;&nbsp;ACOS | sub-keyword | MATHS.c:2942 | - | - | - | U |  |
| &nbsp;&nbsp;ASIN | sub-keyword | MATHS.c:2947 | - | - | - | U |  |
| &nbsp;&nbsp;ATAN | sub-keyword | MATHS.c:2952 | - | - | - | U |  |
| &nbsp;&nbsp;SIN | sub-keyword | MATHS.c:2957 | - | - | - | U |  |
| &nbsp;&nbsp;COS | sub-keyword | MATHS.c:2962 | - | - | - | U |  |
| &nbsp;&nbsp;TAN | sub-keyword | MATHS.c:2967 | - | - | - | U |  |
| &nbsp;&nbsp;SINH | sub-keyword | MATHS.c:2972 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;COSH | sub-keyword | MATHS.c:2977 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;TANH | sub-keyword | MATHS.c:2982 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;EXP | sub-keyword | MATHS.c:2987 | - | - | - | U |  |
| &nbsp;&nbsp;LOG | sub-keyword | MATHS.c:2992 | - | - | - | U |  |
| &nbsp;&nbsp;ABS | sub-keyword | MATHS.c:2997 | - | - | - | U |  |
| &nbsp;&nbsp;SQRT | sub-keyword | MATHS.c:3002 | - | - | - | U |  |
| &nbsp;&nbsp;PROJ | sub-keyword | MATHS.c:3007 | - | - | - | U |  |
| &nbsp;&nbsp;CPLX | sub-keyword | MATHS.c:3012 | - | - | - | U |  |
| &nbsp;&nbsp;POLAR | sub-keyword | MATHS.c:3018 | - | - | - | U |  |
| &nbsp;&nbsp;CRC8 | sub-keyword | MATHS.c:3033 | - | - | - | U |  |
| &nbsp;&nbsp;CRC12 | sub-keyword | MATHS.c:3087 | - | - | - | U |  |
| &nbsp;&nbsp;CRC16 | sub-keyword | MATHS.c:3141 | - | - | - | U |  |
| &nbsp;&nbsp;CRC32 | sub-keyword | MATHS.c:3195 | - | - | - | U |  |
| &nbsp;&nbsp;CROSSING | sub-keyword | MATHS.c:3259 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;CORREL | sub-keyword | MATHS.c:3320 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;CHI_P | sub-keyword | MATHS.c:3376 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;CHI | sub-keyword | MATHS.c:3377 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;DOTPRODUCT | sub-keyword | MATHS.c:3477 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;LOG10 | sub-keyword | MATHS.c:3502 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;M_DETERMINANT | sub-keyword | MATHS.c:3520 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;MAX | sub-keyword | MATHS.c:3549 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MIN | sub-keyword | MATHS.c:3612 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MAGNITUDE | sub-keyword | MATHS.c:3674 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MEAN | sub-keyword | MATHS.c:3695 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;MEDIAN | sub-keyword | MATHS.c:3721 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SD | sub-keyword | MATHS.c:3767 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SUM | sub-keyword | MATHS.c:3811 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;RAND | sub-keyword | MATHS.c:3853 | yes | - | - | P |  |
| &nbsp;&nbsp;BASE64 | sub-keyword | MATHS.c:3868 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ENCODE | sub-keyword | MATHS.c:3876 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;DECODE | sub-keyword | MATHS.c:3890 | yes | yes | - | P+C |  |
| **JSON$(** | function | MM_Misc.c:4236 | t18 | p141 | - | P+C |  |
| **LINSTR(** | function | MM_Misc.c:1338 | t18 | p143 | t14 | P+C |  |
| **LCOMPARE(** | function | MM_Misc.c:1398 | t18 | p142 | t14 | P+C |  |
| **LLEN(** | function | MM_Misc.c:1422 | t18 | p143 | t14 | P+C |  |
| **NUNCHUK(** | function | MiscSTM32.c:1233 | - | p147 | t14 | C |  |
| **CLASSIC(** | function | MiscSTM32.c:1209 | - | p137 | t14 | C |  |
| &nbsp;&nbsp;LX | sub-keyword | MiscSTM32.c:1222 | - | - | - | U |  |
| &nbsp;&nbsp;LY | sub-keyword | MiscSTM32.c:1223 | - | - | - | U |  |
| &nbsp;&nbsp;RX | sub-keyword | MiscSTM32.c:1224 | - | - | - | U |  |
| &nbsp;&nbsp;RY | sub-keyword | MiscSTM32.c:1225 | - | - | - | U |  |
| &nbsp;&nbsp;L | sub-keyword | MiscSTM32.c:1226 | - | - | - | U |  |
| &nbsp;&nbsp;R | sub-keyword | MiscSTM32.c:1227 | - | - | - | U |  |
| &nbsp;&nbsp;B | sub-keyword | MiscSTM32.c:1228 | - | - | - | U |  |
| &nbsp;&nbsp;T | sub-keyword | MiscSTM32.c:1229 | - | - | - | U |  |
| **LGETBYTE(** | function | MM_Misc.c:1321 | t18 | p142 | t14 | P+C |  |
| **LGETSTR$(** | function | MM_Misc.c:1298 | t18 | p142 | t14 | P+C |  |
| **DATE$** | function | MM_Misc.c:2199 | t17, t18 | p137, p87 | t11, t14 | P+C |  |
| **DAY$(** | function | MM_Misc.c:2222 | t18 | p138 | t14 | P+C |  |
| &nbsp;&nbsp;NOW | sub-keyword | MM_Misc.c:2231 | - | - | - | U |  |
| **PEEK(** | function | MM_Misc.c:3518 | t18 | p147, p148 | t14 | P+C |  |
| &nbsp;&nbsp;BYTE | sub-keyword | MM_Misc.c:3522 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;INT8 | sub-keyword | MM_Misc.c:3522 | - | - | - | U |  |
| &nbsp;&nbsp;VARADDR | sub-keyword | MM_Misc.c:3529 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;VARHEADER | sub-keyword | MM_Misc.c:3537 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;CFUNADDR | sub-keyword | MM_Misc.c:3545 | yes | - | - | P |  |
| &nbsp;&nbsp;WORD | sub-keyword | MM_Misc.c:3564 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;SHORT | sub-keyword | MM_Misc.c:3570 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;INTEGER | sub-keyword | MM_Misc.c:3576 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FLOAT | sub-keyword | MM_Misc.c:3583 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;VARTBL | sub-keyword | MM_Misc.c:3593 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;PROGMEM | sub-keyword | MM_Misc.c:3594 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;VAR | sub-keyword | MM_Misc.c:3601 | yes | yes | yes | P+C |  |
| **TIME$** | function | MM_Misc.c:2336 | t17, t18 | p131, p153 | t13, t14 | P+C |  |
| **EPOCH(** | function | MM_Misc.c:722 | t18 | p139 | t14 | P+C |  |
| &nbsp;&nbsp;NOW | sub-keyword | MM_Misc.c:728 | - | - | - | U |  |
| **DATETIME$(** | function | MM_Misc.c:486 | t18 | p137 | t14 | P+C |  |
| &nbsp;&nbsp;NOW | sub-keyword | MM_Misc.c:488 | - | - | - | U |  |
| **MM.INFO(** | function | MM_Misc.c:2793 | t15 | p67, p68 | t7 | P+C |  |
| &nbsp;&nbsp;PIN | sub-keyword | MM_Misc.c:2797 | - | - | - | U |  |
| &nbsp;&nbsp;FLASH ADDRESS | sub-keyword | MM_Misc.c:2809 | yes | - | - | P |  |
| &nbsp;&nbsp;WRITEBUFF | sub-keyword | MM_Misc.c:2815 | yes | - | - | P |  |
| &nbsp;&nbsp;PAGE ADDRESS | sub-keyword | MM_Misc.c:2820 | - | yes | yes | C |  |
| &nbsp;&nbsp;FONT ADDRESS | sub-keyword | MM_Misc.c:2826 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;FONT POINTER | sub-keyword | MM_Misc.c:2832 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;FRAMEBUFFER | sub-keyword | MM_Misc.c:2838 | - | yes | - | C |  |
| &nbsp;&nbsp;SAMPLE PLAYING | sub-keyword | MM_Misc.c:2844 | - | - | - | U |  |
| &nbsp;&nbsp;UPTIME | sub-keyword | MM_Misc.c:2850 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;FILESIZE | sub-keyword | MM_Misc.c:2856 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;EXISTS DIR | sub-keyword | MM_Misc.c:2895 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;EXISTS FILE | sub-keyword | MM_Misc.c:2942 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;DIRECTORY | sub-keyword | MM_Misc.c:2981 | - | - | - | U |  |
| &nbsp;&nbsp;MODIFIED | sub-keyword | MM_Misc.c:2990 | yes | - | - | P |  |
| &nbsp;&nbsp;OPTION | sub-keyword | MM_Misc.c:3035 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;ANGLE | sub-keyword | MM_Misc.c:3037 | - | - | - | U |  |
| &nbsp;&nbsp;Y_AXIS | sub-keyword | MM_Misc.c:3040 | - | - | - | U |  |
| &nbsp;&nbsp;AUTORUN | sub-keyword | MM_Misc.c:3043 | yes | - | - | P |  |
| &nbsp;&nbsp;CONSOLE | sub-keyword | MM_Misc.c:3046 | - | - | - | U |  |
| &nbsp;&nbsp;EXPLICIT | sub-keyword | MM_Misc.c:3050 | - | - | - | U |  |
| &nbsp;&nbsp;LEGACY | sub-keyword | MM_Misc.c:3053 | - | - | - | U |  |
| &nbsp;&nbsp;PROFILING | sub-keyword | MM_Misc.c:3056 | - | - | - | U |  |
| &nbsp;&nbsp;USBKEYBOARD | sub-keyword | MM_Misc.c:3059 | - | - | - | U |  |
| &nbsp;&nbsp;DEFAULT | sub-keyword | MM_Misc.c:3061 | - | - | - | U |  |
| &nbsp;&nbsp;BASE | sub-keyword | MM_Misc.c:3066 | - | - | - | U |  |
| &nbsp;&nbsp;MOUSE | sub-keyword | MM_Misc.c:3071 | - | - | - | U |  |
| &nbsp;&nbsp;CONSOLE PORT | sub-keyword | MM_Misc.c:3075 | - | - | - | U |  |
| &nbsp;&nbsp;BREAK | sub-keyword | MM_Misc.c:3079 | - | - | - | U |  |
| &nbsp;&nbsp;CPUSPEED | sub-keyword | MM_Misc.c:3088 | yes | - | - | P |  |
| &nbsp;&nbsp;MODE | sub-keyword | MM_Misc.c:3091 | - | yes | yes | C |  |
| &nbsp;&nbsp;DEVICE | sub-keyword | MM_Misc.c:3094 | yes | - | - | P |  |
| &nbsp;&nbsp;SEARCH PATH | sub-keyword | MM_Misc.c:3100 | - | - | - | U |  |
| &nbsp;&nbsp;PLATFORM | sub-keyword | MM_Misc.c:3102 | yes | - | - | P |  |
| &nbsp;&nbsp;SYSTEM I2C | sub-keyword | MM_Misc.c:3102 | yes | - | - | P |  |
| &nbsp;&nbsp;VERSION | sub-keyword | MM_Misc.c:3104 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;DISK SIZE | sub-keyword | MM_Misc.c:3111 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FREE SPACE | sub-keyword | MM_Misc.c:3120 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FONTWIDTH | sub-keyword | MM_Misc.c:3129 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;FONTHEIGHT | sub-keyword | MM_Misc.c:3132 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;HPOS | sub-keyword | MM_Misc.c:3135 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;VPOS | sub-keyword | MM_Misc.c:3138 | yes | yes | yes | P+C |  |
| &nbsp;&nbsp;OVERRUN | sub-keyword | MM_Misc.c:3141 | - | - | - | U |  |
| &nbsp;&nbsp;FRAMEH | sub-keyword | MM_Misc.c:3144 | - | yes | yes | C |  |
| &nbsp;&nbsp;ONEWIRE | sub-keyword | MM_Misc.c:3147 | - | - | - | U |  |
| &nbsp;&nbsp;I2C | sub-keyword | MM_Misc.c:3150 | yes | - | - | P |  |
| &nbsp;&nbsp;FCOLOUR | sub-keyword | MM_Misc.c:3153 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;FCOLOR | sub-keyword | MM_Misc.c:3153 | - | - | - | U |  |
| &nbsp;&nbsp;BCOLOUR | sub-keyword | MM_Misc.c:3156 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;BCOLOR | sub-keyword | MM_Misc.c:3156 | - | - | - | U |  |
| &nbsp;&nbsp;FONT | sub-keyword | MM_Misc.c:3159 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;RTC | sub-keyword | MM_Misc.c:3162 | - | - | - | U |  |
| &nbsp;&nbsp;LINES | sub-keyword | MM_Misc.c:3165 | - | - | - | U |  |
| &nbsp;&nbsp;LINE | sub-keyword | MM_Misc.c:3169 | yes | - | - | P |  |
| &nbsp;&nbsp;HARDWARE | sub-keyword | MM_Misc.c:3183 | - | - | - | U |  |
| &nbsp;&nbsp;ERRNO | sub-keyword | MM_Misc.c:3186 | - | - | - | U |  |
| &nbsp;&nbsp;ERRMSG | sub-keyword | MM_Misc.c:3189 | - | - | - | U |  |
| &nbsp;&nbsp;FRAMEV | sub-keyword | MM_Misc.c:3192 | - | yes | yes | C |  |
| &nbsp;&nbsp;STACK | sub-keyword | MM_Misc.c:3195 | yes | - | - | P |  |
| &nbsp;&nbsp;WRITE PAGE | sub-keyword | MM_Misc.c:3198 | - | yes | yes | C |  |
| &nbsp;&nbsp;PROGRAM | sub-keyword | MM_Misc.c:3201 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;MAX PAGES | sub-keyword | MM_Misc.c:3204 | - | yes | yes | C |  |
| &nbsp;&nbsp;DRIVE | sub-keyword | MM_Misc.c:3207 | yes | - | - | P |  |
| &nbsp;&nbsp;CURRENT | sub-keyword | MM_Misc.c:3209 | yes | - | - | P |  |
| &nbsp;&nbsp;PATH | sub-keyword | MM_Misc.c:3214 | yes | yes | - | P+C |  |
| &nbsp;&nbsp;CODE SIZE | sub-keyword | MM_Misc.c:3222 | - | - | - | U |  |
| &nbsp;&nbsp;VARCNT | sub-keyword | MM_Misc.c:3235 | yes | - | - | P |  |
| &nbsp;&nbsp;MINHEAP | sub-keyword | MM_Misc.c:3238 | - | - | - | U |  |
| &nbsp;&nbsp;PSRAM SIZE | sub-keyword | MM_Misc.c:3241 | yes | - | - | P |  |
| &nbsp;&nbsp;HEAP | sub-keyword | MM_Misc.c:3244 | yes | - | - | P |  |
| &nbsp;&nbsp;IHEAP | sub-keyword | MM_Misc.c:3255 | - | - | - | U |  |
| &nbsp;&nbsp;HEAPI | sub-keyword | MM_Misc.c:3255 | - | - | - | U |  |
| &nbsp;&nbsp;SHEAP | sub-keyword | MM_Misc.c:3263 | - | - | - | U |  |
| &nbsp;&nbsp;HEAPS | sub-keyword | MM_Misc.c:3263 | - | - | - | U |  |
| &nbsp;&nbsp;RESET | sub-keyword | MM_Misc.c:3271 | - | - | - | U |  |
| &nbsp;&nbsp;BBC BUFFER | sub-keyword | MM_Misc.c:3274 | yes | - | - | P |  |
| &nbsp;&nbsp;TRACK | sub-keyword | MM_Misc.c:3277 | yes | - | - | P |  |
| &nbsp;&nbsp;SOUND | sub-keyword | MM_Misc.c:3281 | yes | - | - | P |  |
| &nbsp;&nbsp;KEYBOARD | sub-keyword | MM_Misc.c:3306 | - | - | - | U |  |
| &nbsp;&nbsp;ID | sub-keyword | MM_Misc.c:3309 | yes | - | - | P |  |
| &nbsp;&nbsp;ID48 | sub-keyword | MM_Misc.c:3320 | - | - | - | U |  |
| &nbsp;&nbsp;SDCARD | sub-keyword | MM_Misc.c:3325 | - | - | - | U |  |
| **KEYDOWN(** | function | MM_Misc.c:514 | t18 | p142 | t14 | P+C |  |
| **TIMER** | function | MM_Misc.c:702 | t17, t18 | p131, p153 | t13, t14 | P+C |  |
| **MOUSE(** | function | MM_Misc.c:531 | t17 | p146, p94 | - | P+C |  |
| **FORMAT$(** | function | MM_Misc.c:558 | t18 | p139 | t14 | P+C |  |
| **TEMPR(** | function | Onewire.c:184 | t18 | p152 | t14 | P+C |  |
| **BAUDRATE(** | function | Serial.c:348 | - | p135 | t14 | C |  |
| **EOF(** | function | SerialFileIO.c:210 | t18 | p139 | t14 | P+C |  |
| **LOC(** | function | SerialFileIO.c:223 | t18 | p143 | t14 | P+C |  |
| **LOF(** | function | SerialFileIO.c:247 | t18 | p143 | t14 | P+C |  |
| **INPUT$(** | function | SerialFileIO.c:174 | t18 | p141 | t14 | P+C |  |
| **SPI(** | function | SPI.c:158 | t18 | p150 | t14 | P+C |  |
| **SPI2(** | function | SPI.c:278 | t17 | p150 | t14 | P+C |  |
| **SPRITE(** | function | sprites.c:162 | t17, t18 | p126, p150, p151 | t11, t14 | P+C |  |
| &nbsp;&nbsp;W | sub-keyword | sprites.c:165 | - | yes | yes | C |  |
| &nbsp;&nbsp;H | sub-keyword | sprites.c:166 | - | yes | yes | C |  |
| &nbsp;&nbsp;X | sub-keyword | sprites.c:167 | - | yes | yes | C |  |
| &nbsp;&nbsp;Y | sub-keyword | sprites.c:168 | - | yes | yes | C |  |
| &nbsp;&nbsp;L | sub-keyword | sprites.c:169 | - | yes | yes | C |  |
| &nbsp;&nbsp;C | sub-keyword | sprites.c:170 | - | yes | yes | C |  |
| &nbsp;&nbsp;V | sub-keyword | sprites.c:171 | - | yes | yes | C |  |
| &nbsp;&nbsp;T | sub-keyword | sprites.c:172 | - | yes | - | C |  |
| &nbsp;&nbsp;E | sub-keyword | sprites.c:173 | - | yes | - | C |  |
| &nbsp;&nbsp;D | sub-keyword | sprites.c:174 | - | yes | - | C |  |
| &nbsp;&nbsp;A | sub-keyword | sprites.c:175 | - | - | - | U |  |
| &nbsp;&nbsp;N | sub-keyword | sprites.c:176 | - | yes | yes | C |  |
| &nbsp;&nbsp;S | sub-keyword | sprites.c:177 | - | yes | yes | C |  |
| ~( | internal | Functions.c:203 | - | - | - | - |  |
| **TRIM$(** | function | Functions.c:607 | t18 | p153 | - | P+C |  |
| &nbsp;&nbsp;L | sub-keyword | Functions.c:619 | - | - | - | U |  |
| &nbsp;&nbsp;R | sub-keyword | Functions.c:621 | - | - | - | U |  |
| &nbsp;&nbsp;B | sub-keyword | Functions.c:623 | - | - | - | U |  |
| **LINPUT(** | function | MM_Misc.c:681 | t18 | p142 | - | P+C |  |
| **GPS(** | function | GPS.c:344 | t18 | p140, p141 | t14 | P+C |  |
| &nbsp;&nbsp;LATITUDE | sub-keyword | GPS.c:347 | - | yes | yes | C |  |
| &nbsp;&nbsp;LONGITUDE | sub-keyword | GPS.c:351 | - | - | - | U |  |
| &nbsp;&nbsp;SPEED | sub-keyword | GPS.c:355 | - | yes | yes | C |  |
| &nbsp;&nbsp;TRACK | sub-keyword | GPS.c:359 | - | yes | yes | C |  |
| &nbsp;&nbsp;VALID | sub-keyword | GPS.c:363 | - | yes | yes | C |  |
| &nbsp;&nbsp;TIME | sub-keyword | GPS.c:367 | - | yes | yes | C |  |
| &nbsp;&nbsp;DATE | sub-keyword | GPS.c:371 | - | yes | yes | C |  |
| &nbsp;&nbsp;SATELLITES | sub-keyword | GPS.c:375 | - | yes | yes | C |  |
| &nbsp;&nbsp;ALTITUDE | sub-keyword | GPS.c:379 | - | yes | yes | C |  |
| &nbsp;&nbsp;DOP | sub-keyword | GPS.c:383 | - | yes | yes | C |  |
| &nbsp;&nbsp;FIX | sub-keyword | GPS.c:387 | - | yes | yes | C |  |
| &nbsp;&nbsp;GEOID | sub-keyword | GPS.c:391 | - | yes | yes | C |  |
| **STRUCT(** | function | Struct.c:1686 | t17, t18 | p129, p152 | - | P+C |  |
| &nbsp;&nbsp;FIND | sub-keyword | Struct.c:1693 | - | - | - | U |  |
| &nbsp;&nbsp;OFFSET | sub-keyword | Struct.c:1915 | - | - | - | U |  |
| &nbsp;&nbsp;SIZEOF | sub-keyword | Struct.c:1952 | - | - | - | U |  |
| &nbsp;&nbsp;TYPE | sub-keyword | Struct.c:1983 | - | - | - | U |  |
| BITBYTEFLAG( | internal | Functions.c:1048 | - | - | - | - |  |
| **DEVICE(** | function | USBMouse.c:340 | t18 | - | - | P |  |
| &nbsp;&nbsp;MOUSE | sub-keyword | USBMouse.c:342 | yes | - | - | P |  |
| &nbsp;&nbsp;X | sub-keyword | USBMouse.c:349 | - | - | - | U |  |
| &nbsp;&nbsp;Y | sub-keyword | USBMouse.c:350 | - | - | - | U |  |
| &nbsp;&nbsp;L | sub-keyword | USBMouse.c:351 | - | - | - | U |  |
| &nbsp;&nbsp;R | sub-keyword | USBMouse.c:352 | - | - | - | U |  |
| &nbsp;&nbsp;M | sub-keyword | USBMouse.c:353 | - | - | - | U |  |
| &nbsp;&nbsp;W | sub-keyword | USBMouse.c:354 | - | - | - | U |  |
| &nbsp;&nbsp;B | sub-keyword | USBMouse.c:355 | - | - | - | U |  |
| &nbsp;&nbsp;D | sub-keyword | USBMouse.c:356 | - | - | - | U |  |
| &nbsp;&nbsp;T | sub-keyword | USBMouse.c:359 | - | - | - | U |  |
| &nbsp;&nbsp;GAMEPAD | sub-keyword | USBMouse.c:364 | yes | - | - | P |  |
| &nbsp;&nbsp;LX | sub-keyword | USBGamepad.c:566 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;LY | sub-keyword | USBGamepad.c:567 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;RX | sub-keyword | USBGamepad.c:568 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;RY | sub-keyword | USBGamepad.c:569 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;H | sub-keyword | USBGamepad.c:573 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;GX | sub-keyword | USBGamepad.c:574 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;GY | sub-keyword | USBGamepad.c:575 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;GZ | sub-keyword | USBGamepad.c:576 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;AX | sub-keyword | USBGamepad.c:577 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;AY | sub-keyword | USBGamepad.c:578 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;AZ | sub-keyword | USBGamepad.c:579 (USBGamepadDevice) | - | - | - | U |  |
| &nbsp;&nbsp;RAW | sub-keyword | USBGamepad.c:581 (USBGamepadDevice) | - | - | - | U |  |
| **TILEMAP(** | function | TileMap.c:758 | t17 | - | - | P |  |
| &nbsp;&nbsp;TILE | sub-keyword | TileMap.c:761 | - | - | - | U |  |
| &nbsp;&nbsp;COLLISION | sub-keyword | TileMap.c:788 | - | - | - | U |  |
| &nbsp;&nbsp;ATTR | sub-keyword | TileMap.c:838 | - | - | - | U |  |
| &nbsp;&nbsp;VIEWX | sub-keyword | TileMap.c:857 | - | - | - | U |  |
| &nbsp;&nbsp;VIEWY | sub-keyword | TileMap.c:870 | - | - | - | U |  |
| &nbsp;&nbsp;COLS | sub-keyword | TileMap.c:883 | - | - | - | U |  |
| &nbsp;&nbsp;ROWS | sub-keyword | TileMap.c:896 | - | - | - | U |  |
| &nbsp;&nbsp;SPRITE | sub-keyword | TileMap.c:909 | - | - | - | U |  |
| &nbsp;&nbsp;X | sub-keyword | TileMap.c:912 | - | - | - | U |  |
| &nbsp;&nbsp;Y | sub-keyword | TileMap.c:924 | - | - | - | U |  |
| &nbsp;&nbsp;HIT | sub-keyword | TileMap.c:948 | - | - | - | U |  |
| &nbsp;&nbsp;W | sub-keyword | TileMap.c:975 | - | - | - | U |  |
| &nbsp;&nbsp;H | sub-keyword | TileMap.c:987 | - | - | - | U |  |
| (LET) | internal | Commands.c:93 | - | - | - | - |  |
| **MM.HRES** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.VRES** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.VER** | variable (folded) |  | t15 | p67 | - | P+C |  |
| **MM.I2C** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.FONTHEIGHT** | variable (folded) |  | t15 | p66 | - | P+C |  |
| **MM.FONTWIDTH** | variable (folded) |  | t15 | p66 | - | P+C |  |
| **MM.HPOS** | variable (folded) |  | t15 | p66 | - | P+C |  |
| **MM.VPOS** | variable (folded) |  | t15 | p66 | - | P+C |  |
| **MM.ERRNO** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.ERRMSG$** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.WATCHDOG** | variable (folded) |  | t15 | p67 | t7 | P+C |  |
| **MM.DEVICE$** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.CMDLINE$** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.WIDTH** | variable (folded) |  | t15 | p66 | - | P+C |  |
| **MM.HEIGHT** | variable (folded) |  | t15 | p66 | - | P+C |  |
| **MM.ONEWIRE** | variable (folded) |  | t15 | p66 | t7 | P+C |  |
| **MM.FLAGS** | variable (folded) |  | t15 | p66 | - | P+C |  |
| **MM.ERRLINE** | variable (folded) |  | t15 | - | - | P |  |
| **MM.END** | variable (folded) |  | - | - | - | U |  |
| **BIN$(** | function (folded) |  | t18 | p135 | t14 | P+C |  |
| **OCT$(** | function (folded) |  | t18 | p147 | t14 | P+C |  |
| **HEX$(** | function (folded) |  | t18 | p141 | t14 | P+C |  |
| **LCASE$(** | function (folded) |  | t18 | p142 | t14 | P+C |  |
| **UCASE$(** | function (folded) |  | t18 | p153 | t14 | P+C |  |
| **LEFT$(** | function (folded) |  | t18 | p142 | t14 | P+C |  |
| **RIGHT$(** | function (folded) |  | t18 | p149 | t14 | P+C |  |
| **MIN(** | function (folded) |  | t18 | p146 | t14 | P+C |  |
| **MAX(** | function (folded) |  | t18 | p146 | t14 | P+C |  |
| **BIT(** | function (folded) |  | t17, t18 | p136, p80 | - | P+C |  |
| **BYTE(** | function (folded) |  | t17, t18 | p136, p81 | - | P+C |  |
| **FLAG(** | function (folded) |  | t17, t18 | p139, p91 | - | P+C |  |
| **#COMMENT END** | directive |  | - | p77 | - | C |  |
| **#COMMENT START** | directive |  | - | p77 | - | C |  |
| **#DEFINE** | directive |  | - | p77 | - | C |  |
| **#INCLUDE** | directive |  | - | p77 | t11 | C |  |
| **#MMDEBUG OFF** | directive |  | - | p77 | - | C |  |
| **#MMDEBUG ON** | directive |  | - | p77 | - | C |  |
| **?** | alias of PRINT |  | t17 | p77 | t11 | P+C |  |
| **COLOR** | alias of COLOUR |  | - | p83 | - | C |  |
| **SPRITE** | alias of BLIT |  | t17, t18 | p126, p127, p128, p129, p150, p151 | t11, t12, t13, t14 | P+C |  |
| **WII** | alias of CONTROLLER |  | t17 | p133 | t13 | P+C |  |
| **PIXEL FAST** | alias of CLS |  | - | - | - | U |  |
| **ELSE IF** | alias of ELSEIF |  | t17 | p90 | t11 | P+C |  |
| **END IF** | alias of ENDIF |  | t17 | p90 | t11 | P+C |  |
| **EXIT DO** | alias of EXIT |  | t17 | p90 | t11 | P+C |  |
| **CAT** | alias of INC |  | t17 | p82 | - | P+C |  |
| **MM.INFO$** | alias of MM.INFO |  | t15 | p67, p68 | t7 | P+C |  |

## 2. Manual entries for what the build does not have (candidates for X)

An entry's name is read through the tokeniser's aliases first (SPRITE is BLIT, WII is CONTROLLER, CAT is INC). Rows whose left cell is a single letter or starts in lower case are parts of a description (a table inside it), not entries, and are left out.

**PicoMite 7.0**: 113 entries name no keyword of the build:

ASTRO (t17), BACKLIGHT (t17), BLUETOOTH CONNECT (t17), BLUETOOTH DISCONNECT (t17), BLUETOOTH FORGET (t17), BLUETOOTH SCAN (t17), BLUETOOTH STATUS (t17), CAMERA (t17), CAMERA CAPTURE (t17), CAMERA CAPTURE JPEG (t17), CAMERA CHANGE (t17), CAMERA CLOSE (t17), CAMERA OPEN OV2640 (t17), CAMERA OPEN OV5640 (t17), CAMERA OPEN XLK (t17), CAMERA REGISTER (t17), CAMERA TEST (t17), CMM2 LOAD (t17), CONFIGURE (t17), FRAME (t17), I2CLCD (t17), KEYBOARD ON (t17), KEYPAD (t17), LCD CMD (t17), LCD INIT (t17), LOCATION (t17), MANDELBROT (t17), MM.ADDRESS$ (t15), MM.CODE (t15), MM.DISPLAY (t15), MM.MESSAGE$ (t15), MM.PERSISTENT (t15), MM.PS2 (t15), MM.SUPPLY (t15), MM.TOPIC$ (t15), MM.USB (t15), ONESHOT (t17), ONESHOT DISABLE (t17), ONESHOT R (t17), PIO (t17), PIO (t18), PIO CONFIGURE (t17), PIO EXECUTE (t17), PIO PROGRAM (t17), PIO READ (t17), PIO SET BASE (t17), PIO START (t17), PIO SYNC (t17), PIO WRITE (t17), PIO WRITEFIFO (t17), PIO( (t18), RAM (t17), RAY (t17), REFRESH (t17), RESOLUTION (t17), RTC GETTIME (t17), SLEW (t17), STAR (t17), STEPPER (t17), TILE (t17), TMC22 (t17), TOUCH( (t18), WEB (t17), WEB CLOSE TCP CLIENT (t17), WEB CONNECT (t17), WEB MQTT CLOSE (t17), WEB MQTT CONNECT (t17), WEB MQTT PUBLISH (t17), WEB MQTT SUBSCRIBE (t17), WEB MQTT UNSUBSCRIBE (t17), WEB NTP (t17), WEB OPEN TCP CLIENT (t17), WEB OPEN TCP STREAM (t17), WEB OPEN TLS CLIENT (t17), WEB OPEN TLS STREAM (t17), WEB PING (t17), WEB SCAN (t17), WEB TCP CLIENT READ (t17), WEB TCP CLIENT REQUEST (t17), WEB TCP CLIENT STREAM (t17), WEB TCP CLIENT WRITE (t17), WEB TCP INTERRUPT I (t17), WEB TCP READ (t17), WEB TCP SEND (t17), WEB TLS CA (t17), WEB TRANSMIT CODE (t17), WEB TRANSMIT FILE (t17), WEB TRANSMIT PAGE (t17), WEB UDP INTERRUPT (t17), WEB UDP SEND (t17), YMODEM SEND (t17)

**PicoMite 7.0**: 92 entries name a sub-keyword the build's handler does not read:

ADC RUN (t17), BLIT COMPRESSED (t17), BLIT MEMORY332 (t17), BLIT MERGE (t17), BLIT RESIZE (t17), CPU SLEEP (t17), CPU SPEED (t17), DEVICE BITSTREAM (t17), DEVICE CAMERA (t17), DEVICE CAMERA (t19), DEVICE HUMID (t17), DEVICE HUMID (t19), DEVICE KEYPAD (t17), DEVICE KEYPAD (t19), DEVICE LCD (t17), DEVICE LCD (t19), DEVICE SERIALRX (t17), DEVICE SERIALTX (t17), DEVICE WII (t17), DEVICE WII (t19), DEVICE WS2812 (t17), DEVICE WS2812 (t19), FLASH MODBUFF LOAD (t17), FLASH UF2 (t17), GUI CALIBRATE (t17), GUI CLICK (t17), GUI CLICK DOWN (t17), GUI CLICK PIN (t17), GUI RESET LCDPANEL (t17), GUI TEST LCDPANEL (t17), GUI TEST TOUCH (t17), LIST PINS (t17), LIST SYSTEM I2C (t17), LIST VARIABLES (t17), MOUSE OPEN (t17), ON PS2 (t17), OPTION AUDIO BLUETOOTH (t16), OPTION AUDIO I2S BCLK (t16), OPTION AUDIO PWM (t16), OPTION AUDIO SPI CS (t16), OPTION AUDIO VS1053 CLK (t16), OPTION AUTOREFRESH OFF (t16), OPTION BACKLIGHT KBD (t16), OPTION CDC POLL RATE (t16), OPTION COMPILE ON (t16), OPTION CONTINUATION LINES ON (t16), OPTION COUNT (t16), OPTION CPUSPEED (t16), OPTION CYW43 PINS (t16), OPTION DISK SAVE (t16), OPTION DISPLAY (t16), OPTION FNK (t16), OPTION GPS (t16), OPTION GUI CONTROLS (t16), OPTION HDMI PINS (t16), OPTION HEARTBEAT ON (t16), OPTION LCD SPI (t16), OPTION LCD320 ON (t16), OPTION LCDPANEL (t16), OPTION LCDPANEL CONSOLE (t16), OPTION LCDPANEL USER (t16), OPTION LOGGING ON (t16), OPTION MODBUFF ENABLE (t16), OPTION NOCHECK ON (t16), OPTION PICO ON (t16), OPTION PICO PLUS (t16), OPTION PLATFORM (t16), OPTION POWER PFM (t16), OPTION PS2 PINS (t16), OPTION PSRAM PIN (t16), OPTION RADIO BOTH (t16), OPTION RESOLUTION (t16), OPTION SDCARD COMBINED CS (t16), OPTION SDCARD CS (t16), OPTION SYSTEM I2C (t16), OPTION SYSTEM SPI CLK (t16), OPTION TCP SERVER PORT (t16), OPTION TELNET CONSOLE OFF (t16), OPTION TFTP OFF (t16), OPTION TOUCH FT6336 IRQ (t16), OPTION TOUCH T_CS (t16), OPTION UDP SERVER PORT (t16), OPTION WEB MESSAGES ON (t16), OPTION WIFI (t16), POKE DISPLAY (t17), POKE DISPLAY HRES (t17), SAVE PERSISTENT (t17), SETPIN GP1 (t17), SETPIN GP25 (t17), WII NUNCHUCK OPEN (t17)

**CMM2 b14**: 5 entries name no keyword of the build:

#COMMENT (p77), #MMDEBUG (p77), CFUNCTION (p85), LS (p102), MM.ESC (p66)

**CMM2 b14**: 6 entries name a sub-keyword the build's handler does not read:

GUI CURSOUR UNLINK (p93), OPTION F15 (p73), OPTION F16 (p74), OPTION F19 (p74), OPTION F20 (p74), SETTICK FAST (p126)

