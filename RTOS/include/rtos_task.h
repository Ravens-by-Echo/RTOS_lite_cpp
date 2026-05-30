#pragma once

#include "stdint.h"
#include "array"

#define MAX_TASKS 10
#define MAILBOX_SIZE 10 // Mailbox is a queue of messages for inter-task communication

namespace OS_LITE
{
  typedef void (*TaskFunction)();
  constexpr uint32_t WAIT_FOREVER = UINT32_MAX;

  enum class WaitType : uint8_t
  {
    NONE,
    SLEEP,
    SEMAPHORE,
    MUTEX,
    EVENT_FLAGS,
    QUEUE_RX,
    QUEUE_TX
  };

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
  struct MessageQueue
  {
    Message buffer[MAILBOX_SIZE];
    uint8_t head; // buffer fifo control
    uint8_t tail; // buffer fifo control
    uint8_t count; // buffer storage iterator
  };

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
    uint8_t base_priority;
    volatile TaskState state;
    volatile uint32_t delay_interval_ms;
    Mailbox mailbox;

    // Wait Metadata
    WaitType wait_type;
    void* wait_object;
    uint32_t wake_tick_ms;
    uint32_t wait_mask;
    bool wait_all;
    bool clear_on_exit;
    Message pending_message;
  };

  extern std::array<Task,MAX_TASKS> OS_TASKS;
  extern volatile uint8_t task_count;
  extern volatile uint32_t system_ticks; // System tick counter incremented by a timer interrupt
  extern int32_t m_last_scheduled_index;
  extern int32_t m_current_task_index;

  Task* task_create(TaskFunction fn, uint8_t priority);
}
