cmake_minimum_required(VERSION 3.25.1)
project(TheMindClient C)

set(CMAKE_C_STANDARD 17)

add_executable(TheMindClient src/main.c
        src/utils.c
        src/utils.h
        src/ANSI-color-codes.h)
