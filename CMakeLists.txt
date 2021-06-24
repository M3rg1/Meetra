cmake_minimum_required(VERSION 3.19)
project(Meetra)

set(CMAKE_CXX_STANDARD 20)

#message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
#message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
#message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
#message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")

#set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wconversion -Wno-unknown-pragmas -DDEBUG_BUILD")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

add_executable(Meetra src/main.cpp src/UciHandler.cpp src/UciHandler.h src/StringTokenStream.cpp src/StringTokenStream.h src/Bitboards.cpp src/Bitboards.h src/Types.h src/Macros.h src/Board.cpp src/Board.h src/FenLoader.cpp src/FenLoader.h src/Misc.h src/MoveGenerator.cpp src/MoveGenerator.h src/MoveList.cpp src/MoveList.h src/MagicNumbers.h src/Perft.cpp src/Perft.h)