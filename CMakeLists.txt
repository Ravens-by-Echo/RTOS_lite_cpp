cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include(/home/ravens-by-echo/Root/code_dependencies/pico-sdk-master/pico_sdk_init.cmake)

project(RTOS_lite C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

set(PROJECT_HEADERS
  rtos_lite.h
)

set(PROJECT_SOURCES
  rtos_lite.cpp
)

# rest of your project
add_executable(${PROJECT_NAME}
  main.cpp
  ${PROJECT_SOURCES}
  ${PROJECT_HEADERS}
)

# Create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

# Link with the pico_stdlib which aggregates commonly used features
target_link_libraries(${PROJECT_NAME} pico_stdlib)
