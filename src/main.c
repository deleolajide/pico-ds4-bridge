#include <stdlib.h>

#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <tusb.h>

#include "comm.h"
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

void blink(uint32_t count) {
  while (count-- > 0) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(200);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(200);
  }
}

int main() {
  stdio_init_all();

  // initialize CYW43 driver architecture
  if (cyw43_arch_init()) {
    printf("failed to initialise cyw43_arch\n");
    return -1;
  }

  // notify the user that the device started
  blink(2);

  // initialize GPIO for UART
  uart_init(uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);  // GP0: TX
  gpio_set_function(1, GPIO_FUNC_UART);  // GP1: RX

  // initialize dualshock4 shared data
  g_shared_data.lock = spin_lock_init(0);
  g_shared_data.timestamp = to_ms_since_boot(get_absolute_time());
  g_shared_data.ctrl = (ds4_report_t*)malloc(sizeof(ds4_report_t));

  multicore_launch_core1(bluetooth_tasks);

  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  // Initialize USB HID Device stack
  while (!is_ds4_initialized) {
    tud_task();
    sleep_ms(2);
  }
  const ds4_report_t zero_report = default_ds4_report();
  do {
    tud_task();
    sleep_ms(2);
  } while (!tud_hid_report(0, &zero_report, sizeof(ds4_report_t)));
  blink(2);
  sleep_ms(10);

  uint32_t last_timestamp = 0;
  uint32_t timestamp = 0;
  bool is_updated = false;
  bool is_connected = false;

  volatile uint32_t blink_on = 0;
  volatile uint32_t counter = 0;
  absolute_time_t last_report_time = get_absolute_time();
  ds4_report_t* report_ptr = (ds4_report_t*)malloc(sizeof(ds4_report_t));

  while (true) {
    tud_task();

    if (tud_hid_ready()) {
      is_updated = false;
      int32_t time_diff;
      {
        uint32_t save = spin_lock_blocking(g_shared_data.lock);

        timestamp = g_shared_data.timestamp;
        time_diff = timestamp - last_timestamp;

        if (time_diff > 0) {
          last_timestamp = timestamp;

          ds4_report_t* tmp = g_shared_data.ctrl;
          g_shared_data.ctrl = report_ptr;
          report_ptr = tmp;
          is_updated = true;
          is_connected = true;
        }
        spin_unlock(g_shared_data.lock, save);
      }

      // report when dualshock4 is updated or send default report if update is
      // not received for 5ms
      if (is_updated) {
        tud_hid_report(0, report_ptr, sizeof(ds4_report_t));
        last_report_time = get_absolute_time();
        // blink the LED every 100 reports
        if (counter++ % 100 == 0) {
          cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, blink_on ^= 1);
        }
      } else if (is_connected) {
        absolute_time_t now = get_absolute_time();
        int64_t elapsed_us = absolute_time_diff_us(last_report_time, now);
        if (elapsed_us > 50000) {
          tud_hid_report(0, &zero_report, sizeof(ds4_report_t));
          last_report_time = get_absolute_time();
          is_connected = false;
        }
        printf("[INFO] Reset report\n");
      }
    }
  }

  return 0;
}
