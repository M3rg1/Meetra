#ifndef MEETRA_TIMER_H
#define MEETRA_TIMER_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include "ThreadPool.h"

namespace Meetra {

    class Timer {
        std::condition_variable_any cond_var;
        std::recursive_mutex mtx;
        bool active = false;

    public:
        void SetTimeout(auto function, long delay) {
            active = true;
            ThreadPool::PushTask([&, delay, function]() {
                std::unique_lock lock(mtx);
                cond_var.wait_for(lock, std::chrono::milliseconds(delay), [&]() { return !active; });
                if (!active) return;
                function();
            });
        }

        void SetInterval(auto function, long interval) {
            active = true;
            ThreadPool::PushTask([&, interval, function]() {
                while (active) {
                    std::unique_lock lock(mtx);
                    cond_var.wait_for(mtx, std::chrono::milliseconds(interval), [&]() { return !active; });
                    if (!active) return;
                    function();
                }
            });
        }

        void Stop() {
            {
                std::scoped_lock lock(mtx);
                active = false;
            }
            cond_var.notify_all();
        }

    };

}

#endif //MEETRA_TIMER_H
