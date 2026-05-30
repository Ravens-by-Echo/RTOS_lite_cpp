#include "rtos_task.h"
#include <hardware/sync.h>

namespace OS_LITE
{
  // Global task list and scheduler state
  std::array<Task, MAX_TASKS> OS_TASKS{};
  volatile uint8_t task_count{0};
  volatile uint32_t system_ticks{0};
  int32_t m_last_scheduled_index{-1};
  int32_t m_current_task_index{-1};

  static uint32_t critical_section_enter()
  {
    return save_and_disable_interrupts();
  }

  static void critical_section_exit(uint32_t irq_state)
  {
    restore_interrupts(irq_state);
  }

  Task* task_create(TaskFunction fn, uint8_t priority)
  {
    uint32_t irq = critical_section_enter();

    if (task_count >= MAX_TASKS || fn == nullptr) {
      critical_section_exit(irq);
      return nullptr;
    }

    Task& t = OS_TASKS[task_count++];
    t.func = fn;
    t.priority = priority;
    t.base_priority = priority;
    t.state = TaskState::READY;
    t.delay_interval_ms = 0;
    t.mailbox = {};
    t.wait_type = WaitType::NONE;
    t.wait_object = nullptr;
    t.wake_tick_ms = 0;
    t.wait_mask = 0;
    t.wait_all = false;
    t.clear_on_exit = false;
    t.pending_message = {0, 0};

    critical_section_exit(irq);
    return &t;
  }
}
