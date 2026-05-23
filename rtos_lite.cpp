// file: rtos_lite.cpp

// description: RTOS Lite implementation file

// ******************************************************************

#include "rtos_lite.h"

// ******************************************************************

namespace OS_LITE
{
  /*
  */
  bool mailbox_receive(Mailbox* mailbox, Message* msg)
  {
    if (mailbox->count == 0)
    {
      return false;
    }

    *msg = mailbox->buffer[mailbox->head];
    mailbox->head = (mailbox->head + 1) % MAILBOX_SIZE;
    mailbox->count--;

    return true;
  }

  bool mailbox_send(Mailbox* mailbox, const Message msg)
  {
    if (mailbox->count == MAILBOX_SIZE)
    {
      return false;
    }

    mailbox->buffer[mailbox->tail] = msg;
    mailbox->tail = (mailbox->tail + 1) % MAILBOX_SIZE;
    mailbox->count++;

    return true;
  }

  bool tick_callback(struct repeating_timer *t) {
    system_ticks++;

    for (int i = 0; i < task_count; i++) {
      if (OS_TASKS[i].state == TaskState::BLOCKED &&
          system_ticks % OS_TASKS[i].delay_interval_ms == 0)
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
    task->delay_interval_ms = delay_ms;
    task->state = TaskState::BLOCKED;
  }

  void scheduler_init() {
    while (true) {
      int8_t best = -1;
      int8_t best_priority = -10;

      for (uint8_t i = 0; i < task_count; i++) {
        if (OS_TASKS[i].state == TaskState::READY &&
          OS_TASKS[i].priority > best_priority) {

          best_priority = OS_TASKS[i].priority;
          best = i;
        }
      }

      if (best != -1) {
        OS_TASKS[best].state = TaskState::RUNNING;
        OS_TASKS[best].func();
        OS_TASKS[best].state = TaskState::READY;
      }
    }
  }
} // namespace os_lite
