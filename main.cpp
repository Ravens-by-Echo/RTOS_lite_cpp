#include "rtos_lite.h"

static void blink(const uint8_t LED_PIN);
void blink25();
void blink26();

int main()
{
  OS_LITE::OS_TASKS = {{
    {blink25, 1, OS_LITE::TaskState::READY, 0, {}},
    {blink26, 2, OS_LITE::TaskState::READY, 0, {}},
  }};

  OS_LITE::task_count = 2;
  OS_LITE::start_tick();
  OS_LITE::scheduler_init();

  return 0;
}

void blink25()
{
  blink(25);
  OS_LITE::task_sleep(&OS_LITE::OS_TASKS[0], 1000); // Sleep for 1000 ms
}

void blink26()
{
  blink(26);
  OS_LITE::task_sleep(&OS_LITE::OS_TASKS[1], 1000); // Sleep for 1000 ms
}

static void blink(const uint8_t LED_PIN)
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  static bool state {false};

  gpio_put(LED_PIN, state);
  state = !state;
}
