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

}

#endif //MEETRA_TIMER_H
