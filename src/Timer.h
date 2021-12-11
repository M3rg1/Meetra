#ifndef MEETRA_TIMER_H
#define MEETRA_TIMER_H

#include <chrono>
#include "Defs.h"

namespace Time {

    using Clock = std::chrono::steady_clock;
    using ms = std::chrono::milliseconds;
    using ns = std::chrono::nanoseconds;
    using TimePoint = std::chrono::time_point<Clock, ns>;

    inline TimePoint Now() {
        return Clock::now();
    }

    template<typename UNITS>
    TimeRep ElapsedSince(TimePoint t) {
        return std::chrono::duration_cast<UNITS>(Now() - t).count();
    }

    inline uint64_t CalculateNps(uint64_t nodes, TimeRep elapsed_ns) {
        return static_cast<uint64_t>((static_cast<double>(nodes) / static_cast<double>(elapsed_ns + 1)) * 1000000000.0);
    }

    inline TimeRep NsToMs(TimeRep ns) {
        return ns / 1000000;
    }

}

#endif //MEETRA_TIMER_H
