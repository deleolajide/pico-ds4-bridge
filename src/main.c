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
  sleep_ms(250);

  // initialize GPIO for UART
  uart_init(uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);  // GP0: TX
  gpio_set_function(1, GPIO_FUNC_UART);  // GP1: RX

  // initialize dualshock4 shared data
  g_shared_data.lock = spin_lock_init(0);
  g_shared_data.timestamp = to_ms_since_boot(get_absolute_time());
  g_shared_data.ctrl = (ds4_report_t*)malloc(sizeof(ds4_report_t));
  if (g_shared_data.ctrl == NULL) {
    printf("failed to allocate memory for global report\n");
    return -1;
  }
  memset(g_shared_data.ctrl, 0, sizeof(ds4_report_t));
  ds4_report_t* local_report_ptr = (ds4_report_t*)malloc(sizeof(ds4_report_t));
  if (local_report_ptr == NULL) {
    printf("failed to allocate memory for local report\n");
    return -1;
  }
  memset(local_report_ptr, 0, sizeof(ds4_report_t));

  // Note(wy.choi): This needs to be called before bluetooth_init to bluetooth
  // power control to be ready.
  sleep_ms(250);
  multicore_launch_core1(bluetooth_tasks);
  sleep_ms(250);

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
  printf("[INFO] Bluetooth initialized\n");

  absolute_time_t started = get_absolute_time();

  uint32_t last_timestamp = 0;
  uint32_t timestamp = 0;
  bool is_updated = false;
  bool is_connected = false;

  volatile uint32_t blink_on = 0;
  volatile uint32_t counter = 0;

  absolute_time_t last_report_time = get_absolute_time();

  uint32_t ds4_update_count = 0;
  uint32_t ds4_missed_count = 0;
  absolute_time_t stats_start = get_absolute_time();

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
          g_shared_data.ctrl = local_report_ptr;
          local_report_ptr = tmp;
          is_updated = true;
          is_connected = true;
        }
        spin_unlock(g_shared_data.lock, save);
      }

      // report when dualshock4 is updated or send default report if update is
      // not received for 5ms
      if (is_updated) {
        tud_hid_report(0, local_report_ptr, sizeof(ds4_report_t));
        last_report_time = get_absolute_time();
        ds4_update_count++;

        // blink the LED every 100 reports
        if (counter++ % 100 == 0) {
          cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, blink_on ^= 1);
          sleep_ms(1);
        }
      } else if (is_connected) {
        absolute_time_t now = get_absolute_time();
        int64_t elapsed_us = absolute_time_diff_us(last_report_time, now);
        if (elapsed_us > 40000) {
          tud_hid_report(0, &zero_report, sizeof(ds4_report_t));
          last_report_time = get_absolute_time();
          is_connected = false;
          ds4_missed_count++;
          printf("[INFO] Reset report\n");
        }
      }

      absolute_time_t now = get_absolute_time();
      int64_t stat_elapsed_us = absolute_time_diff_us(stats_start, now);
      if (stat_elapsed_us >= 1000000) {
        double elapsed_sec = absolute_time_diff_us(started, now) / 1000000.0;
        printf("[INFO] Elapsed: %f, Updates: %u, Misses: %u\n", elapsed_sec, ds4_update_count, ds4_missed_count);
        ds4_update_count = 0;
        ds4_missed_count = 0;
        stats_start = now;
      }
    }
  }

  return 0;
}
