#pragma once

#include "stdint.h"
#include "rtos_task.h"

namespace OS_LITE
{

  // Kernel objects and API definitions
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

  // semaphore
  bool semaphore_take(Semaphore* sem, uint32_t timeout_ms);
  bool semaphore_give(Semaphore* sem);

  // Mutex + priority inheritance
  bool mutex_lock(Mutex* mutex, uint32_t timeout_ms);
  bool mutex_unlock(Mutex* mutex);

  // event flags
  bool event_flags_wait(EventFlags* flags, uint32_t mask, bool wait_all, bool clear_on_exit, uint32_t timeout_ms);
  void event_flags_set(EventFlags* flags, uint32_t bits);
  void event_flags_clear(EventFlags* flags, uint32_t bits);

  // IRQ protection
  uint32_t critical_section_enter();
  void critical_section_exit(uint32_t irq_state);

  void block_current(WaitType wait_type, void* wait_object, uint32_t timeout_ms);
}
