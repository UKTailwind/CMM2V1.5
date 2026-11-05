# Appendix A – Licences

| Part | Licence | Where it is |
|---|---|---|
| The compiler (cc.app: its passes cc0, cc1 and cc2) | GNU General Public License, version 3 | In the cc folder, as a separate program. It is not part of the firmware. |
| The C preprocessor, cpp | From Fuzix | In cc.app. |
| The translator from MMBasic to C | The MMBasic licence (Geoff Graham and Peter Mather) | In cc.app. |
| The runtime of compiled programs, and the BASIC runtime | Not under the compiler's GNU General Public License. The BASIC runtime is under the MMBasic licence. | In the firmware. |

The source of all of these is on GitHub, in the repository github.com/UKTailwind/FUZIX, branch cmm2. apps/cc/make.py in the firmware's repository, github.com/UKTailwind/CMM2V1.5, builds them from the commit it names.
