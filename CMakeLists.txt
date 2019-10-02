# cmake_minimum_required(VERSION <specify CMake version here>)
project(UDP-communicator C)

set(CMAKE_C_STANDARD 99)

add_executable(UDP-communicator src/communicator.c)
target_link_libraries(UDP-communicator ws2_32)
