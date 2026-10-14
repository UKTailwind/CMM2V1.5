# CMake toolchain file for arm-none-eabi-gcc (bare-metal Cortex-M)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Folder containing arm-none-eabi-gcc. Leave empty to find it on PATH.
set(ARM_TOOLCHAIN_DIR "" CACHE PATH "Directory containing arm-none-eabi-gcc")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARM_TOOLCHAIN_DIR)

if(ARM_TOOLCHAIN_DIR)
    file(TO_CMAKE_PATH "${ARM_TOOLCHAIN_DIR}" _arm_dir)
    set(_arm_prefix "${_arm_dir}/arm-none-eabi-")
else()
    set(_arm_prefix "arm-none-eabi-")
endif()
if(CMAKE_HOST_WIN32)
    set(_arm_exe ".exe")
endif()

set(CMAKE_C_COMPILER   "${_arm_prefix}gcc${_arm_exe}")
set(CMAKE_ASM_COMPILER "${_arm_prefix}gcc${_arm_exe}")
set(CMAKE_OBJCOPY      "${_arm_prefix}objcopy${_arm_exe}")
set(CMAKE_OBJDUMP      "${_arm_prefix}objdump${_arm_exe}")
set(CMAKE_SIZE         "${_arm_prefix}size${_arm_exe}")

# Can't link a test executable without a linker script
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
