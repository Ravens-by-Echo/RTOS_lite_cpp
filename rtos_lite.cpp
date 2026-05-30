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

  bool tick_callback(struct repeating_timer *t) {
    system_ticks++;

    for (uint8_t i = 0; i < task_count; i++) {
      if (OS_TASKS[i].state == TaskState::BLOCKED &&
          tick_reached(system_ticks, OS_TASKS[i].delay_interval_ms))
      {
        OS_TASKS[i].state = TaskState::READY;
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
    task->state = TaskState::BLOCKED;
    critical_section_exit(irq_state);
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
        critical_section_exit(irq_state);
      }
      else
      {
        tight_loop_contents(); // No ready tasks, so we can do a tight loop until the next tick interrupt
      }

    }
  }
} // namespace OS_LITE
