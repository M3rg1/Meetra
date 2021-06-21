cmake_minimum_required(VERSION 3.19)
project(Meetra)

set(CMAKE_CXX_STANDARD 20)

set (CMAKE_CXX_FLAGS "-DDEBUG_BUILD")

add_executable(Meetra src/main.cpp src/UciHandler.cpp src/UciHandler.h src/StringTokenStream.cpp src/StringTokenStream.h src/Bitboards.cpp src/Bitboards.h src/Types.h src/Macros.h src/Board.cpp src/Board.h src/FenLoader.cpp src/FenLoader.h src/Misc.h src/MoveGenerator.cpp src/MoveGenerator.h src/MoveList.cpp src/MoveList.h)