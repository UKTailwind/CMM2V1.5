# Colour Maximite 2 firmware

MMBasic for the Colour Maximite 2 (STM32H743, the G2 board), being brought up to the PicoMite V7 interpreter. The
plan, and where it stands, is in [docs/PicoMite_Parity_Audit_and_Plan.md](docs/PicoMite_Parity_Audit_and_Plan.md).

## How to set up and build

The build uses CMake and Ninja with the GCC that comes with STM32CubeIDE. There is no CubeIDE project. It builds
from a command prompt, and VS Code is optional.

### 1. Install

| | |
|---|---|
| **Git** | To clone the repository. |
| **CMake 3.21 or later** | On the PATH. It's built here with 4.2.1, from the cmake.org installer. |
| **STM32CubeIDE 2.0.0** | In its default place (`C:\ST\STM32CubeIDE_2.0.0`). The build uses its GNU Tools for STM32 13.3.rel1. |
| **STM32CubeProgrammer** | In its default place, to flash the board. |
| VS Code (optional) | With the extensions `.vscode\extensions.json` recommends: C/C++, CMake Tools and Cortex-Debug. |
| Python 3 with pyserial (optional) | For the test tools in `tools\`. |

Ninja comes with the repository (`tools\ninja`).

**If your GCC is somewhere else**, set the environment variable `ARM_TOOLCHAIN_DIR` to the folder that holds
`arm-none-eabi-gcc.exe`. It takes precedence over the path in `CMakePresets.json`. For example:

```
setx ARM_TOOLCHAIN_DIR "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\13.3 rel1\bin"
```

Use GCC 13.3: the image is close to the end of flash, and another compiler gives other sizes. Arm's own Arm GNU
Toolchain 13.3.Rel1 also builds it, to an image a few hundred bytes smaller, but the board tests here use ST's.

### 2. Clone

```
git clone https://github.com/UKTailwind/CMM2V1.5.git
```

The repository is private. Accept the invitation first, and sign in when git asks; Git for Windows' credential
manager handles that.

### 3. Build

From a command prompt in the clone:

```
build.cmd                  Release (the default)
build.cmd Release clean    a clean build (needed after changing the compiler)
```

The output is `build\Release\CMM2V1.5.elf` and `.bin`. The last lines give the size: text is about 1 MB.

In VS Code: open the folder, pick the **Release** preset when CMake Tools asks, then build with Ctrl+Shift+B.

### 4. Flash and debug

```
"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=SWD mode=UR freq=480 -w build\Release\CMM2V1.5.elf -v -rst
```

The VS Code task **Flash (ST-Link)** runs the same command.

- **Always connect under reset (`mode=UR`).** freq=1000 has failed verification here; 480 hasn't.
- **Never connect to a running board.** A hot-plug connect fails ("Unable to get core ID"). On an ST-Link V2-1
  with the old V2J29 firmware it wedged the probe. Updating the probe to V2J46 with STM32CubeProgrammer cured that.
- **For debugging,** use **Debug CMM2 (ST-Link)** in VS Code, which connects under reset. Don't use **Attach**.
  `.vscode\settings.json` gives the CubeIDE 2.0.0 paths of the GCC and the ST-LINK GDB server; change them if
  yours are elsewhere.

### 5. The first boot

- **The console runs at 460800 baud,** no longer 115200. `OPTION BAUDRATE` still changes it.
- **The first boot after flashing resets most options to their defaults.** It keeps the keyboard, RTC, console
  port, SD speed, display mode, mouse and GUI settings.
- **A saved program from older firmware is reloaded from its source file,** because the stored program format
  has changed.

## Working in the tree

- **Line endings vary.** Most sources are CRLF, but `Commands.c`, `FileIO.c`, `kilo.c` and the docs are LF.
  Keep each file as it is.
- **Tests:** each program in `tests\cmm2` has a `.expected` file holding the output it should give. To run them
  all on a board: `python tools\goldens.py COM4 OUTDIR --corpus tests\cmm2 --dir A:/c`
- **CSUBs** for the CMM2 are built by `mmb2csub --target cmm2` (the PicoMite's `user-tools`).
  See [docs/mmb2csub_CMM2_Plan.md](docs/mmb2csub_CMM2_Plan.md).

## Where to read next

| | |
|---|---|
| [docs/PicoMite_Parity_Audit_and_Plan.md](docs/PicoMite_Parity_Audit_and_Plan.md) | The plan, and where it stands (section 7.1). |
| [docs/Notes_for_Gerry_b14.md](docs/Notes_for_Gerry_b14.md) | How 6.00.00b14 compared with this tree, and the plan for merging it. The merge is done (P1 step 8 in the plan). |
| [docs/Gerry_b14_Review_and_Merge_Plan.md](docs/Gerry_b14_Review_and_Merge_Plan.md) | The full review of b14 that the merge followed. |
