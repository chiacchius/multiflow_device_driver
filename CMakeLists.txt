cmake_minimum_required(VERSION 3.23)
project(SOAproject C)

set(CMAKE_C_STANDARD 99)

add_executable(SOAproject multiflow_driver.c structs.h values.h cmake-build-debug/user/user.c read_write_functions.h lock_functions.h user/user_test.c)
