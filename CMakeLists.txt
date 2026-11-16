cmake_minimum_required(VERSION 3.22)

# Colour Maximite 2 firmware (STM32H743II).
# Migrated from the STM32CubeIDE project CMM2V1.5. Compiler and linker flags
# mirror the CubeIDE Debug and Release build configurations.

project(CMM2 LANGUAGES C CXX ASM)                  # (C++: only ymfm, the OPL3 synth, and its glue, OPLChip.cpp)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

set(TARGET CMM2V1.5)
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32H743II_FLASH.ld)

# --- Sources ----------------------------------------------------------------
# Like CubeIDE, every .c/.s/.S file under the source folders is built.
# Link order matches CubeIDE (folders sorted, then files sorted within each
# folder) so the binary layout is the same.
set(SOURCE_ROOTS Drivers Middlewares Src startup)

set(_globs)
foreach(root ${SOURCE_ROOTS})
    list(APPEND _globs ${root}/*.c ${root}/*.cpp ${root}/*.s ${root}/*.S)
endforeach()
file(GLOB_RECURSE _found CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} ${_globs})
list(REMOVE_DUPLICATES _found)                      # *.s and *.S overlap on Windows
list(FILTER _found EXCLUDE REGEX "conflicted copy") # Dropbox conflict files

set(_dirs)
foreach(f ${_found})
    get_filename_component(d ${f} DIRECTORY)
    list(APPEND _dirs ${d})
endforeach()
list(REMOVE_DUPLICATES _dirs)
list(SORT _dirs)

set(SOURCES)
foreach(d ${_dirs})
    set(_in_dir)
    foreach(f ${_found})
        get_filename_component(fd ${f} DIRECTORY)
        if(fd STREQUAL d)
            list(APPEND _in_dir ${f})
        endif()
    endforeach()
    list(SORT _in_dir)
    list(APPEND SOURCES ${_in_dir})
endforeach()

list(LENGTH SOURCES _count)
message(STATUS "CMM2: ${_count} source files")

add_executable(${TARGET} ${SOURCES})

# --- Compiler flags ---------------------------------------------------------
# Drop CMake's default per-config flags (-O3 -DNDEBUG etc.); set ours below.
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_ASM_FLAGS_DEBUG "")
set(CMAKE_ASM_FLAGS_RELEASE "")

set(MCU_FLAGS -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb)

target_compile_definitions(${TARGET} PRIVATE USE_HAL_DRIVER STM32H743xx
    CJSON_NESTING_LIMIT=128)     # JSON$: well inside the stack (cJSON's 1000 is about 100 KB of it)

target_include_directories(${TARGET} PRIVATE
    FATFS
    Inc
    Drivers/STM32H7xx_HAL_Driver/Inc
    Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    Middlewares/Third_Party/tinyusb/src
    Drivers/CMSIS/Device/ST/STM32H7xx/Include
    Drivers/CMSIS/Include
    Inc/Image
)

# Wi-Fi (docs/Web_Plan.md, step 2): cyw43-driver, lwIP and mbedTLS in Middlewares/Third_Party, at the commits the
# PicoMite's pico-sdk has; the CMM2's port in Src/net and Inc/net. lwIP and mbedTLS take the PicoMite's settings
# (Inc/net/lwipopts.h, mbedtls_config.h), with TLS (PICOMITEWEB_TLS) and the input buffer its RP2350 builds have.
target_include_directories(${TARGET} PRIVATE
    Inc/net
    Middlewares/Third_Party/cyw43-driver/src
    Middlewares/Third_Party/cyw43-driver/firmware
    Middlewares/Third_Party/lwip/src/include
    Middlewares/Third_Party/lwip/src/apps/altcp_tls
    Middlewares/Third_Party/mbedtls/include
    Middlewares/Third_Party/puff                    # (zlib's inflater, for the chip's compressed firmware)
    # Bluetooth (docs/Bluetooth_Plan.md, step 3): BTstack (tools/port_btstack.py) and the PicoMite's BT host code (Src/net/bt)
    Middlewares/Third_Party/btstack/src
    Middlewares/Third_Party/btstack/platform/embedded
    Middlewares/Third_Party/btstack/3rd-party/micro-ecc
    Middlewares/Third_Party/btstack/3rd-party/rijndael
    Middlewares/Third_Party/btstack/3rd-party/bluedroid/encoder/include
    Middlewares/Third_Party/btstack/3rd-party/bluedroid/encoder/srce
    Middlewares/Third_Party/btstack/3rd-party/bluedroid/decoder/include
    Middlewares/Third_Party/btstack/3rd-party/yxml
    Middlewares/Third_Party/btstack/3rd-party/md5
    Src/net/bt
)
target_compile_definitions(${TARGET} PRIVATE CYW43_LWIP=1 PICO_CYW43_ARCH_POLL=1 PICOMITEWEB_TLS
    ENABLE_BLE=1 ENABLE_CLASSIC=1                   # (BTstack's, as pico_btstack_ble and the PicoMite's BT host set them)
    MBEDTLS_CONFIG_FILE=<mbedtls_config.h> MBEDTLS_SSL_IN_CONTENT_LEN=16384)
# C11 for them (pico-sdk's bus code uses static_assert, which newlib gives only to C11)
set(NET_SOURCES ${SOURCES})
list(FILTER NET_SOURCES INCLUDE REGEX "^(Middlewares/Third_Party/(cyw43-driver|lwip|mbedtls|btstack)|Src/net)/")
set_property(SOURCE ${NET_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -std=gnu11)
# and NDEBUG, as the PicoMite's Release builds have them: no asserts, and the driver's CYW43_DEBUG chatter (its firmware
# banner at every start) silent
set_property(SOURCE ${NET_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG)
# The PicoMite's network code (Src/net/picomite, tools/port_net.py) is copied as it is: what its own headers gave it
# comes from Inc/net/WebMite_cmm2.h, made its first include
# The PicoMite's Bluetooth host (Src/net/bt/picomite, tools/port_bt.py) likewise, with Inc/net/BTMite_cmm2.h
set(PICOMITE_BT_SOURCES ${SOURCES})
list(FILTER PICOMITE_BT_SOURCES INCLUDE REGEX "^Src/net/bt/picomite/")
set_property(SOURCE ${PICOMITE_BT_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -include BTMite_cmm2.h
    -Wno-pointer-sign -Wno-discarded-qualifiers)
set(PICOMITE_NET_SOURCES ${SOURCES})
list(FILTER PICOMITE_NET_SOURCES INCLUDE REGEX "^Src/net/picomite/")
set_property(SOURCE ${PICOMITE_NET_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -include WebMite_cmm2.h
    -Wno-pointer-sign -Wno-discarded-qualifiers)   # (the CMM2's interpreter API is char *, the PicoMite's unsigned char *)

target_compile_options(${TARGET} PRIVATE
    ${MCU_FLAGS}
    --specs=nano.specs
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections -fdata-sections -Wall -fstack-usage>
    # __FILE__ (used by assert) becomes "../Src/x.c" as in CubeIDE, not an absolute path
    $<$<COMPILE_LANGUAGE:C>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=../>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<CONFIG:Release>>:-Os -std=gnu99>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<CONFIG:Debug>>:-Og -g3 -std=gnu11>
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp -Wa,-W>
    $<$<AND:$<COMPILE_LANGUAGE:ASM>,$<CONFIG:Debug>>:-g3>
    # C++ is ymfm (Src/ymfm) and OPLChip.cpp alone: no exceptions, RTTI, unwind tables or guarded statics
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections -fdata-sections -Wall -std=gnu++14 -fno-exceptions -fno-rtti
        -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-threadsafe-statics -fno-use-cxa-atexit -Wno-format>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:-O2>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug>>:-Og -g3>
)

# The PC sampler (Src/PcSampler.c): the "Sampler" preset, a Release build with it, for finding hot code.
option(CMM2_PCSAMPLER "Build the PC sampler (diagnostic)" OFF)
if(CMM2_PCSAMPLER)
    target_compile_definitions(${TARGET} PRIVATE PCSAMPLER)
    target_compile_options(${TARGET} PRIVATE -g)        # (line numbers for the report; -g leaves the code as it is)
endif()

# Equivalence checks for performance work (the "Check" preset): a Release build that raises an error where a
# shortcut and the slow path it replaces disagree (FINDVAREND_CHECK: findvar's end against skipvar's).
option(CMM2_CHECKS "Build the equivalence checks (diagnostic)" OFF)
if(CMM2_CHECKS)
    target_compile_definitions(${TARGET} PRIVATE FINDVAREND_CHECK)
endif()

# Per-file optimisation overrides from the CubeIDE Release configuration.
# These come after -Os on the command line, so they win.  A function in them marked OPTSIZE (Inc/Configuration.h) is
# still -Os: start-up, loading, LIST, FILES, MM.INFO, OPTION and the like, whose speed does not matter (1 Nov 2026).
set(RELEASE_O2_FILES
    Commands.c Draw.c FileIO.c Functions.c MATHS.c MMBasic.c MM_Misc.c
    Memory.c Operators.c OtherDisplays.c Serial.c
    Timers.c ltdc.c main.c mmc_stm32.c stm32h7xx_it.c
    Symbols.c Struct.c                                      # (on every variable reference: 17 Oct, measured)
)
# MiscSTM32.c (OPTION, LIST OPTIONS), flash.c (flash writes) and SerialFileIO.c (serial file transfer) are -Os, as
# every other file is: their speed is the hardware's, and -O2 cost 5.6 KB of bank 1 (27 Oct 2026, Peter agreed)
set(RELEASE_OZ_FILES FileManager.c Editor.c)
list(TRANSFORM RELEASE_O2_FILES PREPEND Src/)
list(TRANSFORM RELEASE_OZ_FILES PREPEND Src/)
set_property(SOURCE ${RELEASE_O2_FILES} APPEND PROPERTY COMPILE_OPTIONS $<$<CONFIG:Release>:-O2>)
set_property(SOURCE ${RELEASE_OZ_FILES} APPEND PROPERTY COMPILE_OPTIONS $<$<CONFIG:Release>:-Oz>)
# MATHS.c's complex arithmetic (the FFT, the complex MATH functions) inline, without libgcc's __muldc3 and the like,
# which only try to rescue a product or quotient that overflowed to Inf/NaN: 3 KB less flash (Peter, 21 Oct: the
# PicoMite's flag for MATHS.c, without its -Os)
set_property(SOURCE Src/MATHS.c APPEND PROPERTY COMPILE_OPTIONS -fcx-fortran-rules)

# --- Linker -----------------------------------------------------------------
target_link_options(${TARGET} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    --specs=nosys.specs
    --specs=nano.specs
    -static
    -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET}.map
    -Wl,--gc-sections
    # Same as "-u _printf_float -u _scanf_float" (written this way because
    # CMake merges repeated "-u" options). The CubeIDE Debug configuration
    # did not enable float printf/scanf.
    $<$<CONFIG:Release>:-Wl,-u,_printf_float>
    $<$<CONFIG:Release>:-Wl,-u,_scanf_float>
)
target_link_libraries(${TARGET} PRIVATE -Wl,--start-group c m -Wl,--end-group)
set_target_properties(${TARGET} PROPERTIES SUFFIX ".elf" LINK_DEPENDS ${LINKER_SCRIPT})

# --- Outputs (.hex, .bin, .list, size) --------------------------------------
# The .bin is one block from 0x08000000 to bank 2's sector 2, mmbrun's (.xip) after the options: writing it erases the
# options, the program (OPTION FLASH, sectors 3 up) is kept (Peter, 4 Nov 2026: OPTION DISK SAVE before, LOAD after)
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET}> ${TARGET}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary --gap-fill 0xFF $<TARGET_FILE:${TARGET}> ${TARGET}.bin   # (the options blank)
    COMMAND ${CMAKE_OBJDUMP} -h -S $<TARGET_FILE:${TARGET}> > ${TARGET}.list
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS ${TARGET}.hex ${TARGET}.bin ${TARGET}.list ${TARGET}.map
    VERBATIM
)
