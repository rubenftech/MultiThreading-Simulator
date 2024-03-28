cmake_minimum_required(VERSION 3.27)
project(sim_main)

set(CMAKE_CXX_STANDARD 11)

add_executable(sim_main main.c, sim_api.c, sim_api.h, core_api.c,core_api.h)
