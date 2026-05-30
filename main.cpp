#include "rtos_lite.h"
#include "rtos_task.h"

using namespace OS_LITE;

void blink25_producer();
void blink1_consumer();
void heartbeat_task();

constexpr uint32_t EVT_NEW_MSG = (1u << 0);

// Kernel objects
Semaphore m_sem{0, 1};          // binary semaphore
Mutex m_mutex{-1, 0};           // unlocked
EventFlags m_events{0};
MessageQueue m_queue{{}, 0, 0, 0};

volatile uint32_t m_shared_counter = 0;

int main()
{
  gpio_init(16);
  gpio_set_dir(16, GPIO_OUT);

  gpio_init(25);
  gpio_set_dir(25, GPIO_OUT);

  gpio_init(1);
  gpio_set_dir(1, GPIO_OUT);

  task_create(heartbeat_task, 1); // low priority background task
  task_create(blink25_producer, 10);
  task_create(blink1_consumer, 5);

  scheduler_init();
  return 0;
}

void heartbeat_task()
{
  static bool led_state{false};
  led_state = !led_state;
  gpio_put(16, led_state);
  task_sleep_current(1000);
}

void blink25_producer()
{
  static bool led_state{false};
  led_state = !led_state;
  gpio_put(25, led_state);

  // Mutex-protected shared data
  if (mutex_lock(&m_mutex, 0))
  {
    m_shared_counter++;
    mutex_unlock(&m_mutex);
  }

  // Queue + event flag
  Message msg{1, m_shared_counter};
  if (message_queue_send(&m_queue, msg, 0))
  {
    event_flags_set(&m_events, EVT_NEW_MSG);
    // Semaphore signal
    semaphore_give(&m_sem);
  }

  task_sleep_current(500);
}

void blink1_consumer()
{
  // Wait for producer signal
  if (!semaphore_take(&m_sem, WAIT_FOREVER)) return;

  // Wait until queue data event is set
  if (!event_flags_wait(&m_events, EVT_NEW_MSG, false, true, WAIT_FOREVER)) return;

  Message msg{};
  if (message_queue_receive(&m_queue, &msg, 0))
  {
    // Use message value to drive LED1
    gpio_put(1, (msg.value & 1u) ? 1 : 0);
  }

  task_sleep_current(100);
}
