// file: rtos_lite.h

// description: RTOS Lite header file

// ******************************************************************
#pragma once

#include "stdint.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "array"

#define MAX_TASKS 10
#define MAILBOX_SIZE 10 // Mailbox is a queue of messages for inter-task communication

// ******************************************************************

namespace OS_LITE
{
  // ******************************************************************
  // OS Lite API
  inline volatile uint32_t system_ticks {0}; // System tick counter incremented by a timer interrupt
  typedef void (*TaskFunction)();

  enum class TaskState
  {
    READY,
    RUNNING,
    BLOCKED
  };

  struct Message
  {
    uint32_t id;
    uint32_t value;
  };

  // Responsible for task communication. e.g. Share global information
  struct Mailbox
  {
    Message buffer[MAILBOX_SIZE];
    uint8_t head; // buffer fifo control
    uint8_t tail; // buffer fifo control
    uint8_t count; // buffer storage iterator
  };

  // Task Profile
  struct Task
  {
    TaskFunction func;
    uint8_t priority;
    volatile TaskState state;
    volatile uint32_t delay_interval_ms;
    Mailbox mailbox;
  };

  bool mailbox_receive(Mailbox* mailbox, Message* msg);
  bool mailbox_send(Mailbox* mailbox, const Message msg);

  inline std::array<Task,MAX_TASKS> OS_TASKS;
  inline volatile uint8_t task_count {0};

  bool tick_callback(struct repeating_timer* timer);
  void start_tick();
  void task_sleep(Task* task, uint32_t delay_ms);

  void scheduler_init();

// ******************************************************************
} // namespace OS_LITE
