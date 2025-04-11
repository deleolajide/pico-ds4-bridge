#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <tusb.h>

// #include <btstack_run_loop.h>
// #include <hardware/flash.h>
// #include <hardware/sync.h>
// #include <pico/bootrom.h>

#include "dualshock4_shared_data.h"
#include "pico_bluetooth.h"
#include "sdkconfig.h"
#include "tusb_config.h"
#include "usb_descriptors.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

void bluetooth_tasks() {
  bluetooth_init();
  bluetooth_run();
}

int main() {
  stdio_init_all();

  // initialize CYW43 driver architecture (will enable BT if/because
  // CYW43_ENABLE_BLUETOOTH == 1)
  if (cyw43_arch_init()) {
    printf("failed to initialise cyw43_arch\n");
    return -1;
  }

  uart_init(uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);  // GP0: TX
  gpio_set_function(1, GPIO_FUNC_UART);  // GP1: RX

  // initialize dualshock4 shared data
  spin_lock_init(&g_ds4_shared_data.lock);
  g_ds4_shared_data.timestamp = to_ms_since_boot(get_absolute_time());

  multicore_launch_core1(bluetooth_tasks);

  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(500);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  sleep_ms(500);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(500);

  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  while (true) {
    tud_task();

    bool is_fresh = false;
    ds4_report_t report;
    static uint32_t last_timestamp = 0;
    {
      uint32_t save = spin_lock_blocking(&g_ds4_shared_data.lock);
      if (g_ds4_shared_data.timestamp > last_timestamp) {
        last_timestamp = g_ds4_shared_data.timestamp;
        report = g_ds4_shared_data.controller;
        is_fresh = true;
      }
      spin_unlock(&g_ds4_shared_data.lock, save);
    }
    if (is_fresh) {
      printf("##########################################################\n");
      printf("L stick: %d %d\n", report.left_stick_x, report.left_stick_y);
      printf("R stick: %d %d\n", report.right_stick_x, report.right_stick_y);
      printf("L trigger: %d\n", report.left_trigger);
      printf("R trigger: %d\n", report.right_trigger);
      printf("Dpad: 0x%x%x%x%X\n", report.dpad);
      printf("Buttons: 0b%x%x%x%x\n", report.button_south, report.button_west, report.button_north, report.button_east);
    }
    sleep_ms(5);
  }

  return 0;
}
