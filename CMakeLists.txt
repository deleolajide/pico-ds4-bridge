cmake_minimum_required(VERSION 3.13)

if(DEFINED PICO_BOARD)
    message(DEBUG, "Using PICO_BOARD ${PICO_BOARD}")
else()
    message(VERBOSE, "PICO_BOARD not defined. Using 'pico2_w'")
    set(PICO_BOARD "pico2_w" CACHE STRING)
endif()

if(DEFINED PICO_PLATFORM)
    message(DEBUG, "Using PICO_PLATFORM ${PICO_PLATFORM}")
else()
    message(VERBOSE, "PICO_PLATFORM not defined. Using 'rp2350'")
    set(PICO_PLATFORM "rp2350" CACHE STRING)
endif()

# Initialize the SDK based on PICO_SDK_PATH
# NOTE: this must happen before project()
include(pico_sdk_import.cmake)

project(bluepad_stand_alone C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()


###############################################################################

add_executable(${PROJECT_NAME}
    src/main.c
    src/pico_bluetooth.c
)

include_directories(${PROJECT_NAME} src)
include_directories(${PICO_SDK_PATH}/lib/btstack/src)

set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../bluepad32)

add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_ble
    pico_btstack_cyw43
    bluepad32
    tinyusb_device
)

# Useful for debugging
# target_compile_definitions(${PROJECT_NAME} PRIVATE
#        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
#        WANT_HCI_DUMP=1 # This enables btstack debug
#        ENABLE_SEGGER_RTT=1
# )

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
