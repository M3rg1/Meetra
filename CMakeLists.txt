cmake_minimum_required(VERSION 3.19)
project(Popper)

set(CMAKE_CXX_STANDARD 20)

add_executable(Popper src/main.cpp src/UciHandler.cpp src/UciHandler.h src/utils/StringTokenStream.cpp src/utils/StringTokenStream.h)