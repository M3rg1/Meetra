#ifndef MEETRA_MACROS_H
#define MEETRA_MACROS_H


#ifndef TIMER
#define TIMER
#include <chrono>
#define INIT_TIMER auto start = std::chrono::high_resolution_clock::now(); \
                   auto end = std::chrono::high_resolution_clock::now();
#define START_TIMER start = std::chrono::high_resolution_clock::now();
#define STOP_TIMER end = std::chrono::high_resolution_clock::now();
#define GET_TIME_NS std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()
#define GET_TIME_MS std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()
#define GET_TIME_SEC std::chrono::duration_cast<std::chrono::seconds>(end-start).count()
#endif // TIMER

#ifdef DEBUG_BUILD
#include <iostream>
#define DEBUG_LOG(msg) do { std::cerr << msg << std::endl; } while (0)
#else
# define DEBUG_LOG(msg) do {} while (0)
#endif // DEBUG_BUILD

#endif //MEETRA_MACROS_H
