#ifndef MEETRA_TIME_H
#define MEETRA_TIME_H

#include <chrono>
#include "Defs.h"

namespace Meetra::Time {

    using Clock = std::chrono::steady_clock;
    using ms = std::chrono::milliseconds;
    using ns = std::chrono::nanoseconds;
    using TimePoint = std::chrono::time_point<Clock, ns>;

    inline TimePoint Now() {
        return Clock::now();
    }

    template<typename UNITS>
    TimeValue ElapsedTime(TimePoint since) {
        return std::chrono::duration_cast<UNITS>(Now() - since).count() + 1;
    }

}

#endif //MEETRA_TIME_H
