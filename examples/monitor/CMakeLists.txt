add_executable(dualshock4_monitor
    monitor.c
)

target_link_libraries(
    dualshock4_monitor PUBLIC
    pico_stdlib
    pico_cyw43_arch_none
)

target_compile_definitions(dualshock4_monitor
    PRIVATE CFG_TUD_CDC=1
)

pico_enable_stdio_usb(dualshock4_monitor 1)
pico_enable_stdio_uart(dualshock4_monitor 0)
pico_add_extra_outputs(dualshock4_monitor)
