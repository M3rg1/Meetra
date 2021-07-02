#include "ThreadPool.h"

namespace Meetra {

    ThreadPool *ThreadPool::instance = nullptr;

    ThreadPool::ThreadPool(int num_threads) {
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
            std::unique_lock<std::mutex> lock{mtx};
            running = false;
        }

        task_wait_var.notify_all();

        for (auto &thread : threads) {
            thread.join();
        }

        delete(instance);
    }


/*    template<typename F, typename... Args>
    void ThreadPool::i_PushTask(F f, Args &&... args) {
        {
            std::unique_lock<std::mutex> lock{mtx};
            task_queue.emplace(std::bind(f, std::forward<Args>(args)...));
        }
        task_wait_var.notify_one();
    }*/


}
