#pragma once
#ifndef _ENABLE_ATOMIC_ALIGNMENT_FIX
#  define _ENABLE_ATOMIC_ALIGNMENT_FIX 1
#endif
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>


/////////////////////////////////////////////////////////////////////////////////////////////////////
template <size_t _SleepWhenAcquireFailedInMicroSeconds = size_t(-1)>
class SpinLockByTasT
{
    std::atomic_flag locked_flag_ = ATOMIC_FLAG_INIT;

public:
    void lock()
    {
        while (locked_flag_.test_and_set(std::memory_order_acquire)) {
        //while (locked_flag_.test_and_set()) {
            if (_SleepWhenAcquireFailedInMicroSeconds == size_t(-1)) {
                std::this_thread::yield();
            }
            else if (_SleepWhenAcquireFailedInMicroSeconds != 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(_SleepWhenAcquireFailedInMicroSeconds));
            }
        }
    }

    void unlock()
    {
        locked_flag_.clear(std::memory_order_release);
        //locked_flag_.clear();
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
template <size_t _SleepWhenAcquireFailedInMicroSeconds = size_t(-1)>
class SpinLockByCasT
{
    std::atomic<bool> locked_flag_ = ATOMIC_VAR_INIT(false);

public:
    void lock()
    {
        bool exp = false;
        while (!locked_flag_.compare_exchange_strong(exp, true, std::memory_order_acquire)) {
        //while (!locked_flag_.compare_exchange_strong(exp, true)) {
            exp = false;
            if (_SleepWhenAcquireFailedInMicroSeconds == size_t(-1)) {
                std::this_thread::yield();
            }
            else if (_SleepWhenAcquireFailedInMicroSeconds != 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(_SleepWhenAcquireFailedInMicroSeconds));
            }
        }
    }

    void unlock()
    {
        locked_flag_.store(false, std::memory_order_release);
        //locked_flag_.store(false);
    }
};



/////////////////////////////////////////////////////////////////////////////////////////////////////
#define HYPER_YIELD_THRESHOLD_V1 1
class HyperSpinLock1
{
    std::atomic_flag locked_flag_ = ATOMIC_FLAG_INIT;

public:
    void lock()
    {
        uint32_t loop_count, spin_count, yield_cnt;
        int32_t pause_count;

        if (locked_flag_.test_and_set())
        {
            loop_count = 0;
            spin_count = 1;
            do 
            {
                if (loop_count < HYPER_YIELD_THRESHOLD_V1) {
                    for (pause_count=spin_count; pause_count>0; --pause_count) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                    spin_count *= 2;
                }
                else {
                    yield_cnt = loop_count - HYPER_YIELD_THRESHOLD_V1;
                    if ((yield_cnt & 63) == 63) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                    else if ((yield_cnt & 3) == 3) {
                        std::this_thread::sleep_for(std::chrono::microseconds(0));
                    }
                    else {
                        std::this_thread::yield();
                    }
                }
                ++loop_count;
            } while (locked_flag_.test_and_set());
        }
    }

    void unlock()
    {
        
        locked_flag_.clear();
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
#include <xmmintrin.h>
#define HYPER_YIELD_THRESHOLD2 16
class HyperSpinLock2
{
    std::atomic_flag locked_flag_ = ATOMIC_FLAG_INIT;

public:
    void lock()
    {
        uint32_t loop_count, spin_count, yield_cnt;
        int32_t pause_count;

        //if (locked_flag_.test_and_set(std::memory_order_acquire))
        if (locked_flag_.test_and_set())
        {
            yield_cnt = 0;
            loop_count = 0;
            spin_count = 1;
            do
            {
                if (loop_count < HYPER_YIELD_THRESHOLD2) {
                    for (pause_count=spin_count; pause_count>0; --pause_count) {
                        //_mm_pause(); //HT pause
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                    spin_count *= 2;
                    ++loop_count;
                }
                else {
                    if ((yield_cnt & 0xf) == 0xf) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                    else {
                        std::this_thread::yield();
                    }
                    ++yield_cnt;
                }
            //} while (locked_flag_.test_and_set(std::memory_order_acquire));
            } while (locked_flag_.test_and_set());
        }
    }

    void unlock()
    {
        //locked_flag_.clear(std::memory_order_release);
        locked_flag_.clear();
    }
};

