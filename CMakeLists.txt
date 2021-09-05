cmake_minimum_required(VERSION 3.20.2)
project(Meetra)

set(CMAKE_CXX_STANDARD 23)

# https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why/45271521
# https://stackoverflow.com/questions/65335620/terminate-called-after-throwing-an-instance-of-stdsystem-error/65348893#65348893
# -lrt for ubuntu
SET(CMAKE_EXE_LINKER_FLAGS "-static -pthread -Wl,--undefined=pthread_create,--undefined=pthread_cancel")

#set(GCC_COVERAGE_COMPILE_FLAGS "-DDEBUG_BUILD")
#set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wconversion -Wno-unknown-pragmas -DDEBUG_BUILD -pthread")

# -fno-inline-small-functions -- no inlining
# -fno-exceptions -- turn off exceptions (*possibly* very small speedup)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra -Wno-unknown-pragmas -Wno-class-memaccess -Wno-sign-compare")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")

message("GCC_COVERAGE_COMPILE_FLAGS is ${GCC_COVERAGE_COMPILE_FLAGS}")

add_executable(Meetra src/Main.cpp src/Uci.cpp src/Uci.h src/Bitboards.cpp src/Bitboards.h src/Types.h src/Perft.h
        src/Board.cpp src/Board.h src/MoveGen.cpp src/MoveGen.h src/MagicNumbers.h src/Search.cpp src/Search.h
        src/Evaluator.cpp src/Evaluator.h src/EvalValues.h src/TranspositionTable.cpp src/TranspositionTable.h
        src/ZobristHash.cpp src/ZobristHash.h src/SearchThread.cpp src/SearchThread.h src/Utils.h src/TestSuite.h
        src/Book.cpp src/Book.h)