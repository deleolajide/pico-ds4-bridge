#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <tusb.h>

#include "dualshock4_comm.h"
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

  // initialize CYW43 driver architecture
  if (cyw43_arch_init()) {
    printf("failed to initialise cyw43_arch\n");
    return -1;
  }

  uart_init(uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);  // GP0: TX
  gpio_set_function(1, GPIO_FUNC_UART);  // GP1: RX

  // initialize dualshock4 shared data
  g_ds4_shared_data.lock = spin_lock_init(0);
  g_ds4_shared_data.timestamp = to_ms_since_boot(get_absolute_time());

  multicore_launch_core1(bluetooth_tasks);

  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(100);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  sleep_ms(100);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(100);

  ds4_report_t report = {.report_id = 0x01,
                         .left_stick_x = DS4_JOYSTICK_MID,
                         .left_stick_y = DS4_JOYSTICK_MID,
                         .right_stick_x = DS4_JOYSTICK_MID,
                         .right_stick_y = DS4_JOYSTICK_MID,
                         .dpad = 0x08,
                         .button_west = 0,
                         .button_south = 0,
                         .button_east = 0,
                         .button_north = 0,
                         .button_l1 = 0,
                         .button_r1 = 0,
                         .button_l2 = 0,
                         .button_r2 = 0,
                         .button_select = 0,
                         .button_start = 0,
                         .button_l3 = 0,
                         .button_r3 = 0,
                         .button_home = 0,
                         .sensor_data = {},
                         .touchpad_active = 0,
                         .padding = 0,
                         .tpad_increment = 0,
                         .touchpad_data = {},
                         .mystery_2 = {}};

  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  while (true) {
    tud_task();
    bool is_fresh = false;
    static uint32_t last_timestamp = 0;
    {
      uint32_t save = spin_lock_blocking(g_ds4_shared_data.lock);
      if (g_ds4_shared_data.timestamp > last_timestamp) {
        last_timestamp = g_ds4_shared_data.timestamp;
        report = g_ds4_shared_data.controller;
        is_fresh = true;
      }
      spin_unlock(g_ds4_shared_data.lock, save);
    }
    if (is_fresh) {
      tud_hid_report(0, &report, sizeof(report));
    }
  }

  return 0;
}
