cmake_minimum_required(VERSION 3.16)
project(ElevatorSimulation C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
enable_testing()

add_library(elevator_core
    src/queue.c
    src/dlist.c
    src/person.c
    src/elevator.c
)

# sim 需要 main，等 src/sim.c 写好后再加入可执行文件。
# 每个测试单独 add_executable，并 add_test，放在 test/ 里。
