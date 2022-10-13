cmake_minimum_required(VERSION 3.23)
project(SOAproject C)

set(CMAKE_C_STANDARD 99)

add_executable(SOAproject driver/multiflow_driver.c driver/structs.h driver/values.h cmake-build-debug/user/user.c driver/read_write_functions.h driver/lock_functions.h user/user_test.c)
