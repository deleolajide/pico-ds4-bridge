#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

#include "pico_bluetooth.h"
#include "comm.h"

int main(void) {
  stdio_init_all();

  if (cyw43_arch_init()) {
    printf("failed to init cyw43\n");
    return -1;
  }

  g_shared_data.lock = spin_lock_init(0);
  g_shared_data.timestamp = to_ms_since_boot(get_absolute_time());

  printf("#################################################################\n");
  printf("##  Bluetooth Monitor Example                                  ##\n");
  printf("#################################################################\n");

  bluetooth_init();
  bluetooth_run();

  return 0;
}