cmake_minimum_required(VERSION 3.22)

project(Air_Quality_Server_Tests
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

#------------------------------------------------------------------------------
# Paths
#------------------------------------------------------------------------------

set(ROOT_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SRC_DIR     ${ROOT_DIR}/src)
set(INC_DIR     ${ROOT_DIR}/include)
set(UNITY_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/unity/src)
set(STUBS_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/stubs)

#------------------------------------------------------------------------------
# Include directories
#
# IMPORTANT: stubs/ must come BEFORE the real project includes so that stub
# headers (cmsis_os.h, FreeRTOS.h, ssd1306.h, …) shadow the hardware-specific
# originals.
#------------------------------------------------------------------------------

set(TEST_INCLUDE_DIRS
    ${STUBS_DIR}
    ${INC_DIR}
    ${UNITY_DIR}
)

#------------------------------------------------------------------------------
# Compiler options
#------------------------------------------------------------------------------

set(TEST_C_FLAGS
    -Wall
    -Wextra
    -Wpedantic
)

#------------------------------------------------------------------------------
# Common source lists
#------------------------------------------------------------------------------

set(UNITY_SRC       ${UNITY_DIR}/unity.c)
set(FREERTOS_STUBS  ${STUBS_DIR}/freertos_stubs.c)
set(SSD1306_STUBS   ${STUBS_DIR}/ssd1306_stubs.c)
set(GLOBALS_STUBS   ${STUBS_DIR}/globals_stubs.c)

#------------------------------------------------------------------------------
# test_display  —  tests display_iaq_classify()
#------------------------------------------------------------------------------

add_executable(test_display
    ${UNITY_SRC}
    ${FREERTOS_STUBS}
    ${SSD1306_STUBS}
    ${GLOBALS_STUBS}
    ${SRC_DIR}/app/display.c
    ${SRC_DIR}/app/displayed_sensor_management.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_display.c
)

target_include_directories(test_display PRIVATE ${TEST_INCLUDE_DIRS})
target_compile_options(test_display     PRIVATE ${TEST_C_FLAGS})
# _POSIX_C_SOURCE exposes strnlen() used in display.c
target_compile_definitions(test_display PRIVATE UNIT_TEST _POSIX_C_SOURCE=200809L)

add_test(NAME test_display COMMAND test_display)

#------------------------------------------------------------------------------
# test_frame_parser  —  tests crc16_ccitt() and parse_sensor_frame()
#------------------------------------------------------------------------------

add_executable(test_frame_parser
    ${UNITY_SRC}
    ${SRC_DIR}/app/frame_parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_frame_parser.c
)

target_include_directories(test_frame_parser PRIVATE ${TEST_INCLUDE_DIRS})
target_compile_options(test_frame_parser     PRIVATE ${TEST_C_FLAGS})

add_test(NAME test_frame_parser COMMAND test_frame_parser)

#------------------------------------------------------------------------------
# test_sensor_management  —  tests displayed_sensor_update()
#------------------------------------------------------------------------------

add_executable(test_sensor_management
    ${UNITY_SRC}
    ${FREERTOS_STUBS}
    ${SRC_DIR}/app/displayed_sensor_management.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sensor_management.c
)

target_include_directories(test_sensor_management PRIVATE ${TEST_INCLUDE_DIRS})
target_compile_options(test_sensor_management     PRIVATE ${TEST_C_FLAGS})
target_compile_definitions(test_sensor_management PRIVATE UNIT_TEST)

add_test(NAME test_sensor_management COMMAND test_sensor_management)
