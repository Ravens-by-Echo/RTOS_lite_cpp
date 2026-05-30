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
  // Kernel objects and API definitions

  constexpr uint32_t WAIT_FOREVER = UINT32_MAX; // Special value to indicate infinite wait time for blocking calls

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

  struct Semaphore
  {
    volatile uint16_t count;
    uint16_t max_count;
  };

  struct Mutex // Mutual exclusion lock with priority inheritance
  {
    int8_t owner; // -1 = unlocked, otherwise holds the index of the owning task
    volatile uint8_t locked_count;
  };

  struct EventFlags
  {
    volatile uint32_t bits;
  };

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
    volatile TaskState state;
    volatile uint32_t delay_interval_ms;
    Mailbox mailbox;

    // Wait Metadata
    WaitType wait_type;
    void* wait_object;
    uint32_t wait_tick_ms;
    uint32_t wait_mask;
    bool wait_all;
    bool clear_on_exit;
    Message pending_message;
  };

  // semaphore
  bool semaphore_take(Semaphore* sem, uint32_t timeout_ms);
  bool semaphore_give(Semaphore* sem);

  // Mutex + priority inheritance
  bool mutex_lock(Mutex* mutex, uint32_t timeout_ms);
  bool mutex_unlock(Mutex* mutex);

  // event flags
  bool event_flags_wait(EventFlags* flags, uint32_t mask, bool wait_all, bool clear_on_exit, uint32_t timeout_ms);
  bool event_flags_set(EventFlags* flags, uint32_t bits);
  bool event_flags_clear(EventFlags* flags, uint32_t bits);

  // message queue
  bool message_queue_send(MessageQueue* queue, const Message msg, uint32_t timeout_ms);
  bool message_queue_receive(MessageQueue* queue, Message* msg, uint32_t timeout_ms);

  // Round Robin implementation
  static int32_t m_last_scheduled_index {-1};
  static int8_t pick_next_task_index();

  bool mailbox_receive(Mailbox* mailbox, Message* msg);
  bool mailbox_send(Mailbox* mailbox, const Message msg);

  inline std::array<Task,MAX_TASKS> OS_TASKS;
  inline volatile uint8_t task_count {0};

  bool tick_callback(struct repeating_timer* timer);
  const bool tick_reached(uint32_t now, uint32_t deadline);
  void start_tick();
  void task_sleep(Task* task, uint32_t delay_ms);

  // Protect against IRS races and shared data
  static inline uint32_t critical_section_enter();
  static inline void critical_section_exit(uint32_t irq_state);

  void scheduler_init();

// ******************************************************************
} // namespace OS_LITE
