// file: rtos_lite.h

// description: RTOS Lite header file

// ******************************************************************
#pragma once

#include "stdint.h"
#include "pico/stdlib.h"

// ******************************************************************

namespace os_lite
{
    // ******************************************************************
    // OS Lite API

    struct Task
    {
      void (*func)();
      uint32_t interval_ms;
      uint32_t last_run_ms {0};
    };

    uint32_t now();
    void run_scheduler(Task tasks[], uint8_t task_count);

    // ******************************************************************
} // namespace os_lite
