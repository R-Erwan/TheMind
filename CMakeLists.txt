cmake_minimum_required(VERSION 3.25.1)
project(TheMind C)

set(CMAKE_C_STANDARD 17)

add_executable(TheMind main.c
        playersRessources.c
        playersRessources.h
        Game.c
        Game.h
        queue.c
        queue.h
        utils.c
        utils.h)
