// file: rtos_lite.h

// description: RTOS Lite header file

// ******************************************************************
#pragma once

#include "stdint.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "array"
#include "os_scheduler.h"
#include "rtos_task.h"

// ******************************************************************

namespace OS_LITE
{
  // message queue
  bool message_queue_send(MessageQueue* queue, const Message msg, uint32_t timeout_ms);
  bool message_queue_receive(MessageQueue* queue, Message* msg, uint32_t timeout_ms);

  // Round Robin implementation
  static int8_t pick_next_task_index();

  bool mailbox_receive(Mailbox* mailbox, Message* msg);
  bool mailbox_send(Mailbox* mailbox, const Message msg);

  bool tick_callback(struct repeating_timer* timer);
  const bool tick_reached(uint32_t now, uint32_t deadline);
  void start_tick();
  void task_sleep(Task* task, uint32_t delay_ms);
  void task_sleep_current(uint32_t delay_ms);

  void scheduler_init();

// ******************************************************************
} // namespace OS_LITE
