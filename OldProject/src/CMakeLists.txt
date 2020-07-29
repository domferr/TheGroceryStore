cmake_minimum_required(VERSION 3.16)
project(src C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(src
        cashier.c
        cashier.h
        client.c
        client.h
        client_in_queue.c
        client_in_queue.h
        config.c
        config.h
        grocerystore.c
        grocerystore.h
        logger.c
        logger.h
        main.c
        manager.c
        manager.h
        queue.c
        queue.h
        scfiles.c
        scfiles.h
        signal_handler.c
        signal_handler.h
        threadpool.c
        threadpool.h
        types.h
        utils.c
        utils.h)
