// file: rtos_lite.cpp

// description: RTOS Lite implementation file

// ******************************************************************

#include "rtos_lite.h"
#include "stdio.h"
#include <hardware/sync.h>

// ******************************************************************

namespace OS_LITE
{
  /*
  */
  static inline void block_current(WaitType wait_type, void* wait_object, uint32_t timeout_ms)
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

  static inline uint32_t critical_section_enter()
  {
    return save_and_disable_interrupts();
  }

  static inline void critical_section_exit(uint32_t irq_state)
  {
    restore_interrupts(irq_state);
  }

  bool mailbox_receive(Mailbox* mailbox, Message* msg)
  {
    const uint32_t irq_state = critical_section_enter();
    if (mailbox->count == 0)
    {
      critical_section_exit(irq_state);
      return false;
    }

    *msg = mailbox->buffer[mailbox->head];
    mailbox->head = (mailbox->head + 1) % MAILBOX_SIZE;
    mailbox->count--;

    critical_section_exit(irq_state);
    return true;
  }

  bool mailbox_send(Mailbox* mailbox, const Message msg)
  {

    const uint32_t irq_state = critical_section_enter();
    if (mailbox->count == MAILBOX_SIZE)
    {
      critical_section_exit(irq_state);
      return false;
    }

    mailbox->buffer[mailbox->tail] = msg;
    mailbox->tail = (mailbox->tail + 1) % MAILBOX_SIZE;
    mailbox->count++;

    critical_section_exit(irq_state);
    return true;
  }

  const bool tick_reached(uint32_t now, uint32_t deadline) {
    const bool reached = static_cast<int32_t>(now - deadline) >= 0;
    return reached;
  }

  bool tick_callback(struct repeating_timer* t) {
    (void)t;
    system_ticks++;

    for (uint8_t i = 0; i < task_count; i++)
    {
      if (OS_TASKS[i].state != TaskState::BLOCKED)
      {
        continue;
      }

      if (OS_TASKS[i].wait_type == WaitType::SLEEP)
      {
        if (tick_reached(system_ticks, OS_TASKS[i].delay_interval_ms))
        {
          OS_TASKS[i].wait_type = WaitType::NONE;
          OS_TASKS[i].wait_object = nullptr;
          OS_TASKS[i].state = TaskState::READY;
        }
        continue;
      }

      if (OS_TASKS[i].wake_tick_ms != WAIT_FOREVER &&
          tick_reached(system_ticks, OS_TASKS[i].wake_tick_ms))
      {
        OS_TASKS[i].wait_type = WaitType::NONE;
        OS_TASKS[i].wait_object = nullptr;
        OS_TASKS[i].state = TaskState::READY; // timeout wake
      }
    }

    return true;
  }

  void start_tick()
  {
    static struct repeating_timer timer;
    add_repeating_timer_ms(1, tick_callback, NULL, &timer);
  }

  void task_sleep(Task* task, uint32_t delay_ms)
  {
    const uint32_t irq_state = critical_section_enter();
    task->delay_interval_ms = system_ticks + delay_ms;
    task->wait_type = WaitType::SLEEP;
    task->wait_object = nullptr;
    task->state = TaskState::BLOCKED;
    critical_section_exit(irq_state);
  }

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
    int8_t waiter = highest_waiter(WaitType::SEMAPHORE, s);
    if (waiter >= 0)
    {
      OS_TASKS[waiter].wait_type = WaitType::NONE;
      OS_TASKS[waiter].wait_object = nullptr;
      OS_TASKS[waiter].state = TaskState::READY;
      critical_section_exit(irq);
      return true;
    }
    if (s->count < s->max_count)
    {
      s->count++;
    }
    critical_section_exit(irq);
    return true;
  }

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

  bool message_queue_send(MessageQueue* queue, const Message msg, uint32_t timeout_ms)
  {
    const uint32_t irq = critical_section_enter();
    if (queue->count < MAILBOX_SIZE)
    {
      queue->buffer[queue->tail] = msg;
      queue->tail = (queue->tail + 1) % MAILBOX_SIZE;
      queue->count++;
      critical_section_exit(irq);
      return true;
    }

    OS_TASKS[m_current_task_index].pending_message = msg;
    block_current(WaitType::QUEUE_TX, queue, timeout_ms);
    critical_section_exit(irq);
    return false;
  }

  bool message_queue_receive(MessageQueue* queue, Message* msg, uint32_t timeout_ms)
  {
    const uint32_t irq = critical_section_enter();
    if (queue->count > 0)
    {
      *msg = queue->buffer[queue->head];
      queue->head = (queue->head + 1) % MAILBOX_SIZE;
      queue->count--;
      critical_section_exit(irq);
      return true;
    }

    block_current(WaitType::QUEUE_RX, queue, timeout_ms);
    critical_section_exit(irq);
    return false;
  }

  static int8_t pick_next_task_index()
  {
    int16_t highest_priority = -1;

    if (task_count == 0)
    {
      return -1;
    }

    // Highest priority task with Ready state will be selected to run next
    for (uint8_t i = 0; i < task_count; i++) {
      if (OS_TASKS[i].state == TaskState::READY &&
          OS_TASKS[i].priority > highest_priority)
      {
        highest_priority = OS_TASKS[i].priority;
      }
    }

    if (highest_priority < 0)
    {
      return -1;
    }

    // Round robin start point
    uint8_t start_index;
    if (m_last_scheduled_index >= 0)
    {
      start_index = static_cast<uint8_t>(m_last_scheduled_index +1) % task_count;
    }
    else
    {
      start_index = 0;
    }

    for (uint8_t i = 0; i < task_count; i++) {
      const uint8_t index = (start_index + i) % task_count;
      if (OS_TASKS[index].state == TaskState::READY &&
          OS_TASKS[index].priority == static_cast<uint8_t>(highest_priority))
      {
        return static_cast<int8_t>(index);
      }
    }

    return -1;
  };

  void scheduler_init() {
    start_tick();
    while (true) {
      uint32_t irq_state = critical_section_enter();

      int8_t best = pick_next_task_index();

      // Update state within critical section to prevent task state changes before execution
      if (best != -1) {
        OS_TASKS[best].state = TaskState::RUNNING;
        m_current_task_index = best;
        m_last_scheduled_index = best;
      }
      critical_section_exit(irq_state);

      if (best != -1) {
        // run function outside of critical section to allow tick interrupts and task state changes during execution
        OS_TASKS[best].func();

        irq_state = critical_section_enter();
        if (OS_TASKS[best].state == TaskState::RUNNING) {
          OS_TASKS[best].state = TaskState::READY;
        }
        m_current_task_index = -1;
        critical_section_exit(irq_state);
      }
      else
      {
        tight_loop_contents(); // No ready tasks, so we can do a tight loop until the next tick interrupt
      }

    }
  }
} // namespace OS_LITE
