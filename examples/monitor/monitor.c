#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

int main(void) {
  stdio_init_all();

  if (cyw43_arch_init()) {
    printf("failed to init cyw43\n");
    return -1;
  }

  while (true) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(250);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(250);
    printf("Hello, World!\n");
  }
  return 0;
}