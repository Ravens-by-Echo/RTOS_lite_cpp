#include "rtos_lite.h"

void blink25();
void blink1();

int main()
{
  gpio_init(25);
  gpio_set_dir(25, GPIO_OUT);
  gpio_init(1);
  gpio_set_dir(1, GPIO_OUT);

  OS_LITE::OS_TASKS = {{
    {blink25, 1, OS_LITE::TaskState::READY, 0, {}},
    {blink1, 2, OS_LITE::TaskState::READY, 0, {}},
  }};

  OS_LITE::task_count = 2;
  OS_LITE::scheduler_init();

  return 0;
}

void blink25()
{
  static bool state{false};
  gpio_put(25, state);
  state = !state;

  OS_LITE::task_sleep(&OS_LITE::OS_TASKS[0], 2000); // 500 ms
}

void blink1()
{
  static bool state{false};
  gpio_put(1, state);
  state = !state;

  OS_LITE::task_sleep(&OS_LITE::OS_TASKS[1], 1000); // 500 ms
}
