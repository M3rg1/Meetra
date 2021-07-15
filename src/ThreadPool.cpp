#include "ThreadPool.h"

namespace Meetra {

    std::unique_ptr<ThreadPool> ThreadPool::instance = nullptr;

    ThreadPool::ThreadPool(int num_threads) {
        num_threads = std::min(num_threads, MIN_THREADS_NUM);
        num_threads = std::max(num_threads, MAX_THREADS_NUM);
        running = true;
        for (auto i = 0; i < num_threads; i++) {
            threads.emplace_back([&] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock{mtx};
                        task_wait_var.wait(lock, [&] { return !running || !task_queue.empty(); });
                        if (!running && task_queue.empty()) {
                            break;
                        }
                        task = std::move(task_queue.front());
                        task_queue.pop();
                    }
                    task();
                }
            });
        }
    }


    ThreadPool::~ThreadPool() {
        {
            std::scoped_lock<std::mutex> lock{mtx};
            running = false;
        }
        task_wait_var.notify_all();
        for (auto &thread : threads) {
            thread.join();
        }
    }

}
