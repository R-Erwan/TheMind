cmake_minimum_required(VERSION 3.25.1)
project(TheMind C)

set(CMAKE_C_STANDARD 17)

add_executable(TheMind src/main.c
        src/playersRessources.c
        src/playersRessources.h
        src/Game.c
        src/Game.h
        src/queue.c
        src/queue.h
        src/utils.c
        src/utils.h
        src/statsManager.c
        src/statsManager.h)
