#ifndef MEETRA_TIMER_H
#define MEETRA_TIMER_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include "ThreadPool.h"

namespace Meetra {

    class Timer {
        std::condition_variable cond_var;
        std::mutex mtx;
        bool active = false;
        std::future<void> future;

    public:
        void SetTimeout(auto function, ulong delay) {
            active = true;
            future = ThreadPool::PushTask([&, delay, function]() {
                std::unique_lock lock(mtx);
                cond_var.wait_for(lock, std::chrono::milliseconds(delay), [&]() { return !active; });
                if (!active) return;
                function();
            });
        }

        void SetInterval(auto function, ulong interval) {
            active = true;
            future = ThreadPool::PushTask([&, interval, function]() {
                while (active) {
                    std::unique_lock lock(mtx);
                    cond_var.wait_for(lock, std::chrono::milliseconds(interval), [&]() { return !active; });
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
            future.wait();
        }

    };

}

#endif //MEETRA_TIMER_H
