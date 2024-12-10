cmake_minimum_required(VERSION 3.25.1)
project(TheMindRobotIA C)

set(CMAKE_C_STANDARD 17)

add_executable(TheMindRobotIA src/main.c
        src/utils.c
        src/utils.h
        src/GameState.c
        src/GameState.h
        src/queue.c
        src/queue.h)
