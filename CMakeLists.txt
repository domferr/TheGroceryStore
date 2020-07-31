cmake_minimum_required(VERSION 3.16)
project(TheGroceryStore C)

set(CMAKE_C_STANDARD 99)

add_executable(TheGroceryStore src/direttore.c src/config.c src/config.h src/utils.c src/utils.h src/supermercato.c)