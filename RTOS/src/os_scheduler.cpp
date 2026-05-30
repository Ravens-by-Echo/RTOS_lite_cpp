
#include "os_scheduler.h"
#include <hardware/sync.h>

namespace OS_LITE
{
  // IRQ protection
  uint32_t critical_section_enter()
  {
    return save_and_disable_interrupts();
  }

  void critical_section_exit(uint32_t irq_state)
  {
    restore_interrupts(irq_state);
  }

  // Common functions
  void block_current(WaitType wait_type, void* wait_object, uint32_t timeout_ms)
  {
    const uint32_t irq_state = critical_section_enter();
    Task* current_task = &OS_TASKS[m_current_task_index];
    current_task->wait_type = wait_type;
    current_task->wait_object = wait_object;
    current_task->wake_tick_ms = (timeout_ms == WAIT_FOREVER) ? WAIT_FOREVER : system_ticks + timeout_ms;
    current_task->state = TaskState::BLOCKED;
    critical_section_exit(irq_state);
  }

  static int8_t highest_waiter(WaitType wait_type, void* wait_object)
  {
    int8_t highest_index = -1;
    int16_t highest_priority = -1;

    for (uint8_t i = 0; i < task_count; i++) {
      if (OS_TASKS[i].state == TaskState::BLOCKED &&
          OS_TASKS[i].wait_type == wait_type &&
          OS_TASKS[i].wait_object == wait_object &&
          OS_TASKS[i].priority > highest_priority)
      {
        highest_priority = OS_TASKS[i].priority;
        highest_index = i;
      }
    }

    return highest_index;
  }

  // semaphore
  bool semaphore_take(Semaphore* s, uint32_t timeout_ms)
  {
    const uint32_t irq = critical_section_enter();

    if (s->count > 0)
    {
      s->count--;
      critical_section_exit(irq);
      return true;
    }

    block_current(WaitType::SEMAPHORE, s, timeout_ms);
    critical_section_exit(irq);
    return false;
  }

  bool semaphore_give(Semaphore* s)
  {
    const uint32_t irq = critical_section_enter();

    // always create one token (bounded)
    if (s->count < s->max_count) {
      s->count++;
    }

    // wake one highest-priority waiter for this semaphore
    int8_t w = highest_waiter(WaitType::SEMAPHORE, s);
    if (w >= 0) {
      OS_TASKS[w].wait_type = WaitType::NONE;
      OS_TASKS[w].wait_object = nullptr;
      OS_TASKS[w].state = TaskState::READY;
    }

    critical_section_exit(irq);
    return true;
  }

  // Mutex + priority inheritance
  bool mutex_lock(Mutex* m, uint32_t timeout_ms)
  {
    const uint32_t irq = critical_section_enter();
    if (m->owner == -1)
    {
      m->owner = static_cast<int8_t>(m_current_task_index);
      m->locked_count = 1;
      critical_section_exit(irq);
      return true;
    }

    if (m->owner == m_current_task_index)
    {
      m->locked_count++;
      critical_section_exit(irq);
      return true;
    }

    // priority inheritance
    if (OS_TASKS[m_current_task_index].priority > OS_TASKS[m->owner].priority)
    {
      OS_TASKS[m->owner].priority = OS_TASKS[m_current_task_index].priority;
    }

    block_current(WaitType::MUTEX, m, timeout_ms);
    critical_section_exit(irq);
    return false;
  }

  bool mutex_unlock(Mutex* m)
  {
    const uint32_t irq = critical_section_enter();
    if (m->owner != m_current_task_index)
    {
      critical_section_exit(irq);
      return false; // not owner
    }

    if (--m->locked_count > 0)
    {
      critical_section_exit(irq);
      return true; // still locked by owner
    }

    // restore owner priority before unlocking
    OS_TASKS[m_current_task_index].priority = OS_TASKS[m_current_task_index].base_priority;

    const int8_t waiter = highest_waiter(WaitType::MUTEX, m);
    if (waiter >= 0)
    {
      m->owner = waiter;
      m->locked_count = 1;
      OS_TASKS[waiter].wait_type = WaitType::NONE;
      OS_TASKS[waiter].wait_object = nullptr;
      OS_TASKS[waiter].state = TaskState::READY;
    }
    else
    {
      m->owner = -1;
      m->locked_count = 0;
    }
    critical_section_exit(irq);
    return true;
  }

  // event flags
  static inline bool event_match(uint32_t bits, uint32_t mask, bool wait_all)
  {
    if (wait_all)
    {
      return (bits & mask) == mask;
    }
    else
    {
      return (bits & mask) != 0;
    }
  }


  bool event_flags_wait(EventFlags* flags, uint32_t mask, bool wait_all, bool clear_on_exit, uint32_t timeout_ms)
  {
    const uint32_t irq = critical_section_enter();

    if (event_match(flags->bits, mask, wait_all))
    {
      if (clear_on_exit)
      {
        flags->bits &= ~mask;
      }
      critical_section_exit(irq);
      return true;
    }
    Task& task = OS_TASKS[m_current_task_index];
    task.wait_mask = mask;
    task.wait_all = wait_all;
    task.clear_on_exit = clear_on_exit;

    block_current(WaitType::EVENT_FLAGS, flags, timeout_ms);
    critical_section_exit(irq);
    return false;
  }

  void event_flags_set(EventFlags* flags, uint32_t bits)
  {
    const uint32_t irq = critical_section_enter();
    flags->bits |= bits;

    // Check if any waiting tasks can be unblocked
    for (uint8_t i = 0; i < task_count; i++)
    {
      Task& task = OS_TASKS[i];
      if (task.state == TaskState::BLOCKED &&
          task.wait_type == WaitType::EVENT_FLAGS &&
          task.wait_object == flags &&
          event_match(flags->bits, task.wait_mask, task.wait_all))
      {
        if (task.clear_on_exit)
        {
          flags->bits &= ~task.wait_mask;
        }
        task.wait_type = WaitType::NONE;
        task.wait_object = nullptr;
        task.state = TaskState::READY;
      }
    }

    critical_section_exit(irq);
  }

  void event_flags_clear(EventFlags* flags, uint32_t bits)
  {
    const uint32_t irq = critical_section_enter();
    flags->bits &= ~bits;
    critical_section_exit(irq);
  }
}
