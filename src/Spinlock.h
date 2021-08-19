#ifndef MEETRA_SPINLOCK_H
#define MEETRA_SPINLOCK_H

#include <atomic>

namespace Meetra{

    class Spinlock {

    public:

        void Lock() {
            while (lock.test_and_set(std::memory_order_acquire)) {
                while (lock.test(std::memory_order_relaxed));
            }
        }

        void Unlock() {
            lock.clear(std::memory_order_release);
        }

    private:
        std::atomic_flag lock = ATOMIC_FLAG_INIT;
    };

    class ScopedSpinlock {
    public:
        explicit ScopedSpinlock(Spinlock &sl) : spinlock(sl){
            spinlock.Lock();
        }
        ~ScopedSpinlock(){
            spinlock.Unlock();
        }
    private:
        Spinlock &spinlock;
    };

}

#endif //MEETRA_SPINLOCK_H
