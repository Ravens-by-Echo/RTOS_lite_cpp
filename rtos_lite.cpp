// file: rtos_lite.cpp

// description: RTOS Lite implementation file

// ******************************************************************

#include "rtos_lite.h"

// ******************************************************************

namespace os_lite
{
  uint32_t now()
  {
    return to_ms_since_boot(get_absolute_time());
  }

  void run_scheduler(Task tasks[], uint8_t task_count) {
    while (true) {
      uint32_t current_time = now();

      for (uint8_t i = 0; i < task_count; i++) {
        if (current_time - tasks[i].last_run_ms >= tasks[i].interval_ms) {
          tasks[i].func();
          tasks[i].last_run_ms = current_time;
        }
      }
    }
  }
} // namespace os_lite
