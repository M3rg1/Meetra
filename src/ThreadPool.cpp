#include "ThreadPool.h"

namespace Meetra {

    std::unique_ptr<ThreadPool> ThreadPool::instance = nullptr;

    ThreadPool::ThreadPool(int num_threads) {
        running = true;
        num_threads = std::clamp(num_threads, MIN_THREADS_NUM, MAX_THREADS_NUM);
        for (auto i = 0; i < num_threads; i++) {
            threads.emplace_back([&] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock{mtx};
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

    ThreadPool *ThreadPool::GetInstance() {
        if (!instance) {
            InitThreadPool(DEFAULT_THREADS_NUM);
            return instance.get();
        }
        return instance.get();
    }

    void ThreadPool::Resize(int num_threads) {
        Shutdown();
        InitThreadPool(num_threads);
    }

    void ThreadPool::Shutdown(){
        instance.reset();
    }

    void ThreadPool::InitThreadPool(int thread_num) {
        if (!instance) {
            instance = std::make_unique<ThreadPool>(thread_num);
        }
    }


    ThreadPool::~ThreadPool() {
        {
            std::scoped_lock lock{mtx};
            running = false;
        }
        task_wait_var.notify_all();
        for (auto &thread : threads) {
            thread.join();
        }
    }

}
