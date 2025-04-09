#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

// #include <btstack_run_loop.h>
// #include <hardware/flash.h>
// #include <hardware/sync.h>
// #include <pico/bootrom.h>

#include "pico_bluetooth.h"
#include "sdkconfig.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// // Defined in my_platform.c
// struct uni_platform* get_my_platform(void);

int main() {
  stdio_init_all();

  // initialize CYW43 driver architecture (will enable BT if/because
  // CYW43_ENABLE_BLUETOOTH == 1)
  if (cyw43_arch_init()) {
    printf("failed to initialise cyw43_arch\n");
    return -1;
  }

  // Turn-on LED. Turn it off once init is done.
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(1000);

  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  sleep_ms(1000);

  bluetooth_init();

  bluetooth_run();

  return 0;
}
