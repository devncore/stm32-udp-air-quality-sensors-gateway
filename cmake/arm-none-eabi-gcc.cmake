# CMake toolchain file for ARM Cortex-M cross-compilation
# Target: STM32F401RE (Cortex-M4 with FPU)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Toolchain prefix
set(TOOLCHAIN_PREFIX arm-none-eabi-)

# Find the toolchain binaries
find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)
find_program(CMAKE_GDB ${TOOLCHAIN_PREFIX}gdb)

# Don't try to run test executables on the host
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search paths configuration
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Prevent CMake from testing the compiler
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
