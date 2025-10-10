cmake_minimum_required(VERSION 4.0)

include(pico_sdk_import.cmake)

project(GM_Clock)
set(CMAKE_CXX_STANDARD 20)

add_executable(GM_Clock main.cpp)
