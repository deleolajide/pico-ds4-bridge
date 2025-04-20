add_executable(${PROJECT_NAME}
    main.c
    dualshock4_comm.c
    pico_bluetooth.c
    usb_descriptors.c
)

include_directories(config)

target_include_directories(${PROJECT_NAME} PRIVATE
    config
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include
    ${PICO_SDK_PATH}/lib/btstack/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_ble
    pico_btstack_cyw43
    pico_multicore
    bluepad32
    tinyusb_device
)

# Useful for debugging
# target_compile_definitions(${PROJECT_NAME} PRIVATE
#        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
#        WANT_HCI_DUMP=1 # This enables btstack debug
#        ENABLE_SEGGER_RTT=1
# )

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
