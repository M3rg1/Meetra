#ifndef MEETRA_TIMER_H
#define MEETRA_TIMER_H
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>

class Timer {
    std::atomic<bool> active{true};

public:
    void SetTimeout(auto function, long delay) {
        active = true;
        std::thread t([=, this]() {
            if(!active.load()) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            if(!active.load()) return;
            function();
        });
        t.detach();
    }

    void SetInterval(auto function, long interval) {
        active = true;
        std::thread t([=, this]() {
            while(active.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                if(!active.load()) return;
                function();
            }
        });
        t.detach();
    }

    void Stop() {
        active = false;
    }

};

#endif //MEETRA_TIMER_H
