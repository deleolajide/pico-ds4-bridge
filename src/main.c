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

void blink_noti(uint32_t count) {
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
  blink_noti(2);

  // initialize GPIO for UART
  uart_init(uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);  // GP0: TX
  gpio_set_function(1, GPIO_FUNC_UART);  // GP1: RX

  // initialize dualshock4 shared data
  g_shared_data.lock = spin_lock_init(0);
  g_shared_data.timestamp = to_ms_since_boot(get_absolute_time());

  multicore_launch_core1(bluetooth_tasks);

  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  blink_noti(2);

  ds4_report_t* report = (ds4_report_t*)malloc(sizeof(ds4_report_t));
  uint32_t last_timestamp = 0;
  uint32_t timestamp = 0;
  bool is_fresh = false;

  uint32_t counter = 0;
  uint32_t blink = 0;

  while (true) {
    tud_task();
    is_fresh = false;
    {
      uint32_t save = spin_lock_blocking(g_shared_data.lock);
      timestamp = g_shared_data.timestamp;
      if (timestamp > last_timestamp) {
        last_timestamp = timestamp;
        ds4_report_t* tmp = g_shared_data.ctrl;
        g_shared_data.ctrl = report;
        report = tmp;
        is_fresh = true;
      }
      spin_unlock(g_shared_data.lock, save);
    }
    if (is_fresh) {
      tud_hid_report(0, report, sizeof(ds4_report_t));
      counter++;
      if (counter % 100 == 0) {
        blink = !blink;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, blink);
        sleep_ms(10);
      }
    }
  }

  return 0;
}
