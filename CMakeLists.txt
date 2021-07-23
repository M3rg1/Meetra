cmake_minimum_required(VERSION 3.19)
project(Meetra)

set(CMAKE_CXX_STANDARD 20)

#message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
#message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
#message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
#message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")

set(GCC_COVERAGE_COMPILE_FLAGS "-pthread")
#set(GCC_COVERAGE_COMPILE_FLAGS "-DDEBUG_BUILD")
#set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wconversion -Wno-unknown-pragmas -DDEBUG_BUILD -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

add_executable(Meetra src/Main.cpp src/Uci.cpp src/Uci.h src/StringTokenStream.cpp src/StringTokenStream.h src/Bitboards.cpp src/Bitboards.h src/Types.h src/Perft.h src/Board.cpp src/Board.h src/MoveGen.cpp src/MoveGen.h src/MagicNumbers.h src/Search.cpp src/Search.h src/Evaluation.cpp src/Evaluation.h src/EvalValues.h src/Timer.h src/ThreadPool.cpp src/ThreadPool.h src/TranspositionTable.cpp src/TranspositionTable.h src/ZobristHash.cpp src/ZobristHash.h src/PVTable.h src/SearchThread.cpp src/SearchThread.h)