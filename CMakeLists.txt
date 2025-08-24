cmake_minimum_required(VERSION 3.13)

if(DEFINED PICO_BOARD)
    message(DEBUG, "Using PICO_BOARD ${PICO_BOARD}")
else()
    message(VERBOSE, "PICO_BOARD not defined. Using 'pico2_w'")
    set(PICO_BOARD "pico2_w")
endif()

if(DEFINED PICO_PLATFORM)
    message(DEBUG, "Using PICO_PLATFORM ${PICO_PLATFORM}")
else()
    message(VERBOSE, "PICO_PLATFORM not defined. Using 'rp2350'")
    set(PICO_PLATFORM "rp2350")
endif()

# Initialize the SDK based on PICO_SDK_PATH
# NOTE: this must happen before project()
include(pico_sdk_import.cmake)

project(pico_ds4_bridge C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

###############################################################################

# add_compile_definitions()

add_subdirectory(src)

include_directories(config src)

set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/lib/bluepad32)
add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)