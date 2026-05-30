cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(RTOS_LITE_SOURCES
    src/os_scheduler.cpp
    src/rtos_lite.cpp
    src/rtos_task.cpp
)

set(RTOS_LITE_HEADERS
    include/os_scheduler.h
    include/rtos_lite.h
    include/rtos_task.h
)

add_library(rtos_lite STATIC)
target_sources(rtos_lite PRIVATE
    ${RTOS_LITE_SOURCES}
    ${RTOS_LITE_HEADERS}   # optional: helps IDE show headers in target
)

target_include_directories(rtos_lite PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtos_lite PUBLIC
    pico_stdlib
    hardware_sync
    hardware_timer
)
