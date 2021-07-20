#ifndef MEETRA_THREADPOOL_H
#define MEETRA_THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>
#include <thread>
#include <queue>
#include <future>

namespace Meetra {

#define MIN_THREADS_NUM 16
#define MAX_THREADS_NUM 256
#define DEFAULT_THREADS_NUM 16

    class ThreadPool {
    public:
        static ThreadPool *GetInstance();
        static void Resize(int num_threads);
        explicit ThreadPool(int num_threads);
        ~ThreadPool();
        static void Init(int thread_num);
        static void Shutdown();


        template<typename F, typename... Args>
        static auto PushTask(F f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
            return GetInstance()->i_PushTask(f, args...);
        }

    private:
        template<typename F, typename... Args>
        auto i_PushTask(F f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
            using return_type = typename std::result_of<F(Args...)>::type;
            auto task = std::make_shared< std::packaged_task<return_type()> >(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            std::future<return_type> res = task->get_future();
            {
                std::scoped_lock lock(mtx);
                task_queue.emplace([task](){ (*task)(); });
                //task_queue.emplace(std::bind(f, std::forward<Args>(args)...));
            }
            task_wait_var.notify_one();
            return res;
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
