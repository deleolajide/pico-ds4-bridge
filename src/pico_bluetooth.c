#include "pico_bluetooth.h"

#include <stddef.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>

#include "sdkconfig.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// struct uni_platform* get_my_platform(void) {
//     static struct uni_platform plat = {
//         .name = "Naverlabs Pico2 W",
//         .init = my_platform_init,
//         .on_init_complete = my_platform_on_init_complete,
//         .on_device_discovered = my_platform_on_device_discovered,
//         .on_device_connected = my_platform_on_device_connected,
//         .on_device_disconnected = my_platform_on_device_disconnected,
//         .on_device_ready = my_platform_on_device_ready,
//         .on_oob_event = my_platform_on_oob_event,
//         .on_controller_data = my_platform_on_controller_data,
//         .get_property = my_platform_get_property,
//     };

//     return &plat;
// }


void init_bluetooth(void) {
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(500);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  sleep_ms(500);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(500);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  sleep_ms(500);

  // // Must be called before uni_init()
  // uni_platform_set_custom(get_my_platform());

  // // Initialize BP32
  // uni_init(0, NULL);
}