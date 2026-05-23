#include "rtos_lite.h"

void blink();

int main()
{
  os_lite::Task tasks[] = {
    {blink, 1000, 0}
  };
  os_lite::run_scheduler(tasks, sizeof(tasks) / sizeof(tasks[0]));
}

void blink()
{
  const uint LED_PIN = 25;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  static bool state {false};
  gpio_put(LED_PIN, state);
  state = !state;
}
