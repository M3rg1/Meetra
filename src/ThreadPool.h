#ifndef MEETRA_THREADPOOL_H
#define MEETRA_THREADPOOL_H


#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>
#include <thread>
#include <queue>

namespace Meetra {

#define MIN_THREADS_NUM 8
#define MAX_THREADS_NUM 256
#define DEFAULT_THREADS_NUM 8

    class ThreadPool {
    public:
        static ThreadPool *GetInstance();
        static void Resize(int num_threads);
        explicit ThreadPool(int num_threads);
        ~ThreadPool();
        static void Init(int thread_num);
        static void Shutdown();


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

        std::vector<std::jthread> threads;
        std::queue<std::function<void()>> task_queue;
        std::condition_variable task_wait_var;
        std::mutex mtx;

        bool running;
    };

}


#endif //MEETRA_THREADPOOL_H
