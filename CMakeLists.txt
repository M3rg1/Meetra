cmake_minimum_required(VERSION 3.20.2)
project(Meetra)

set(CMAKE_CXX_STANDARD 23)

# https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why/45271521
SET(CMAKE_EXE_LINKER_FLAGS "-static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")

# -fno-inline-small-functions
# -funroll-loops
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-exceptions")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra -Wno-unknown-pragmas")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")

add_executable(Meetra src/Main.cpp src/Uci.cpp src/Uci.h src/Bitboards.cpp src/Bitboards.h src/Defs.h src/Perft.h
        src/Board.cpp src/Board.h src/MoveGen.cpp src/MoveGen.h src/MagicNumbers.h src/Search.cpp src/Search.h
        src/Evaluator.cpp src/Evaluator.h src/EvalValues.h src/TranspositionTable.cpp src/TranspositionTable.h
        src/ZobristHash.cpp src/ZobristHash.h src/SearchThread.cpp src/SearchThread.h src/TestSuite.h
        src/Book.h src/Time.h src/Config.h)