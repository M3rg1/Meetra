#ifndef MEETRA_TIMER_H
#define MEETRA_TIMER_H
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#include <condition_variable>
#include <mutex>

class Timer {
    //std::atomic<bool> active{false};
    std::condition_variable cond_var;
    std::mutex mtx;
    bool active = false;

public:
    void SetTimeout(auto function, long delay) {
        active = true;
        std::jthread t([&]() {
            std::unique_lock<std::mutex> lock(mtx);
            cond_var.wait_for(lock, std::chrono::milliseconds(delay), [&](){ return !active; });
            if(!active) return;
            function();
        });
        t.detach();
    }

    // this is ok if it doesnt end properly, because it will be just pooling info from the search
    // the only problem is that we might have multiple pooling threads running at the same time
    // because the previous one didnt exit correcly, careful with that - could test with high sleep time
    // like 10 seconds or whatever and then see what happens
    // could just redo exactly the same way as the above, with cond var and getting woken up
    // when Stop method is called
    void SetInterval(auto function, long interval) {
        active = true;
        std::thread t([&]() {
            while(active) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                if(!active) return;
                function();
            }
        });
        t.detach();
    }

    void Stop() {
        std::unique_lock<std::mutex> lock(mtx);
        active = false;
        cond_var.notify_all();
    }

};

#endif //MEETRA_TIMER_H
