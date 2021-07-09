#ifndef MEETRA_THREADPOOL_H
#define MEETRA_THREADPOOL_H


#include <condition_variable>
#include <functional>
#include <iostream>
#include <future>
#include <vector>
#include <thread>
#include <queue>

namespace Meetra {

    class ThreadPool {
    public:
        static ThreadPool *GetInstance() {
            if (!instance) {
                InitThreadPool(4);
                return instance.get();
            }
            return instance.get();
        }

        static void Resize(int num_threads) {
            instance.reset();
            InitThreadPool(num_threads);
        }

        explicit ThreadPool(int num_threads);
        ~ThreadPool();

        static void InitThreadPool(auto thread_num) {
            if (!instance) {
                instance = std::make_unique<ThreadPool>(thread_num);
            }
        }

        template<typename F, typename... Args>
        static void PushTask(F f, Args &&... args) {
            GetInstance()->i_PushTask(f, args...);
        }

    private:
        template<typename F, typename... Args>
        void i_PushTask(F f, Args &&... args) {
            {
                std::scoped_lock<std::mutex> lock{mtx};
                task_queue.emplace(std::bind(f, std::forward<Args>(args)...));
            }
            task_wait_var.notify_one();
        }

        static std::unique_ptr<ThreadPool> instance;

        std::vector<std::thread> threads;
        std::queue<std::function<void()>> task_queue;
        std::condition_variable task_wait_var;
        std::mutex mtx;

        bool running;
    };

/*    std::vector<std::thread> ThreadPool::threads = std::vector<std::thread>();
    std::queue<std::function<void()>> ThreadPool::task_queue = std::queue<std::function<void()>>();
    std::mutex ThreadPool::mtx = std::mutex();
    bool ThreadPool::running = true;*/

/*    inline void InitThreadPool() {
        ThreadPool *tp = Meetra::ThreadPool::GetInstance();
        Meetra::ThreadPool::i_PushTask()
    }*/

}


#endif //MEETRA_THREADPOOL_H
