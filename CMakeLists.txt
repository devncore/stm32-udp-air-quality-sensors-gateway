cmake_minimum_required(VERSION 3.22)

# Project configuration
set(PROJECT_NAME Air_Quality_Server)
set(MCU_FAMILY STM32F4xx)
set(MCU_MODEL STM32F401xE)
set(CPU_PARAMETERS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# Toolchain file must be set before project()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi-gcc.cmake)

project(${PROJECT_NAME}
    VERSION 1.0.0
    LANGUAGES C ASM
)

# Language standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Standard: ${CMAKE_C_STANDARD}")

#------------------------------------------------------------------------------
# Source files
#------------------------------------------------------------------------------

# Application sources
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/network.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/displayed_sensor_management.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/frame_parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/error_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/watchdog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/reset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/hal_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/uart_rx.c
)

# Driver sources
set(DRIVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/esp8266/esp8266.c
)

# stm32-ssd1306 (SSD1306 / SH1107 OLED driver)
set(SSD1306_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/stm32-ssd1306)
set(SSD1306_SOURCES
    ${SSD1306_DIR}/ssd1306.c
    ${SSD1306_DIR}/ssd1306_fonts.c
)
set(SSD1306_INCLUDE_DIRS
    ${SSD1306_DIR}
)

# STM32CubeMX generated sources
set(CUBEMX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mxcube)

set(CUBEMX_SOURCES
    ${CUBEMX_DIR}/Core/Src/main.c
    ${CUBEMX_DIR}/Core/Src/freertos.c
    ${CUBEMX_DIR}/Core/Src/stm32f4xx_it.c
    ${CUBEMX_DIR}/Core/Src/stm32f4xx_hal_msp.c
    ${CUBEMX_DIR}/Core/Src/stm32f4xx_hal_timebase_tim.c
    ${CUBEMX_DIR}/Core/Src/system_stm32f4xx.c
    ${CUBEMX_DIR}/Core/Src/sysmem.c
    ${CUBEMX_DIR}/Core/Src/syscalls.c
)

# Startup file (from CubeMX)
set(STARTUP_SOURCES
    ${CUBEMX_DIR}/startup_stm32f401xe.s
)

# HAL sources (from CubeMX)
set(HAL_SOURCES
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
)

# FreeRTOS sources
set(FREERTOS_SOURCES
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/croutine.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/list.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/queue.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/timers.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
)

#------------------------------------------------------------------------------
# Include directories
#------------------------------------------------------------------------------

set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# CubeMX include directories
set(CUBEMX_INCLUDE_DIRS
    ${CUBEMX_DIR}/Core/Inc
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CUBEMX_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CUBEMX_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CUBEMX_DIR}/Drivers/CMSIS/Include
)

# FreeRTOS include directories
set(FREERTOS_INCLUDE_DIRS
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/include
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    ${CUBEMX_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
)

#------------------------------------------------------------------------------
# Compile definitions
#------------------------------------------------------------------------------

set(PROJECT_DEFINITIONS
    ${MCU_MODEL}
    USE_HAL_DRIVER
    $<$<CONFIG:Debug>:DEBUG>
)

#------------------------------------------------------------------------------
# Compiler flags
#------------------------------------------------------------------------------

set(C_FLAGS
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wdouble-promotion
    -Wformat=2
    -Wformat-truncation
    -Wundef
    -fno-common
    -ffunction-sections
    -fdata-sections
    $<$<CONFIG:Debug>:-Og -g3 -ggdb>
    $<$<CONFIG:Release>:-Os>
)

set(ASM_FLAGS
    ${CPU_PARAMETERS}
    -x assembler-with-cpp
)

#------------------------------------------------------------------------------
# Linker configuration
#------------------------------------------------------------------------------

set(LINKER_SCRIPT ${CUBEMX_DIR}/STM32F401XX_FLASH.ld)

set(LINKER_FLAGS
    ${CPU_PARAMETERS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${PROJECT_NAME}.map
    --specs=nano.specs
    --specs=nosys.specs
    -lc
    -lm
)

#------------------------------------------------------------------------------
# Executable target
#------------------------------------------------------------------------------

add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
    ${DRIVER_SOURCES}
    ${SSD1306_SOURCES}
    ${CUBEMX_SOURCES}
    ${STARTUP_SOURCES}
    ${HAL_SOURCES}
    ${FREERTOS_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_INCLUDE_DIRS}
    ${CUBEMX_INCLUDE_DIRS}
    ${FREERTOS_INCLUDE_DIRS}
    ${LWRB_INCLUDE_DIRS}
    ${SSD1306_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${PROJECT_DEFINITIONS}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${C_FLAGS}>
    $<$<COMPILE_LANGUAGE:ASM>:${ASM_FLAGS}>
)

target_link_options(${PROJECT_NAME} PRIVATE
    ${LINKER_FLAGS}
)

#------------------------------------------------------------------------------
# Post-build commands
#------------------------------------------------------------------------------

# Generate binary file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generating binary file: ${PROJECT_NAME}.bin"
)

# Generate hex file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMENT "Generating hex file: ${PROJECT_NAME}.hex"
)

# Print size information
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Size information:"
)

#------------------------------------------------------------------------------
# Flash targets (optional)
#------------------------------------------------------------------------------

add_custom_target(flash
    COMMAND st-flash write ${PROJECT_NAME}.bin 0x08000000
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing ${PROJECT_NAME}.bin to target..."
)

add_custom_target(flash-openocd
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
            -c "program ${PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing ${PROJECT_NAME}.elf via OpenOCD..."
)
