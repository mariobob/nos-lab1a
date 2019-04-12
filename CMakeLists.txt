cmake_minimum_required(VERSION 3.13)
project(nos_lab1a C)

set(CMAKE_C_STANDARD 99)

add_executable(main main.c utilities.h)
add_executable(seller seller.c utilities.h)
add_executable(smoker smoker.c utilities.h)
