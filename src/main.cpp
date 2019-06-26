#ifdef __GNUC__
#   ifndef _GNU_SOURCE
#      define _GNU_SOURCE
#   endif
#endif

#include <iostream>
#include <condition_variable>
#include <cassert>
#include <cstdio>
#include "SpinLock.hpp"
#include "LockfreeStack.hpp"
#include "LockedStack.hpp"

#if defined(WIN32) || defined(_WIN32)
#  include <windows.h>
#  include <tchar.h>
#else
#  include <pthread.h>
#endif

void press_continue() {
#if defined(WIN32) || defined(_WIN32)
    system("pause");
#endif    
}

template <typename _Ty, int _ThreadCount = 8, int _LoopCount = 1000000>
struct LockFreePerformanceTestT
{
    template <class _ProcessUnit>
    static double Run(_ProcessUnit puf)
    {
        bool StartSignal = false;
        std::mutex StartMtx;
        std::condition_variable StartCond;
        //-------------------------------------------------------------------------------
        std::thread ths[_ThreadCount];
        for (int i = 0; i < _ThreadCount; ++i)
        {
            ths[i] = std::thread([&puf, &StartSignal, &StartMtx, &StartCond]() {
                std::unique_lock<std::mutex> guard(StartMtx);
                while (StartSignal == false) {
                    StartCond.wait(guard);
                }
                for (int i = 0; i < _LoopCount; ++i) { puf(); }
            });

            #if defined(WIN32) || defined(_WIN32)
                DWORD_PTR affmask = ((DWORD_PTR)1 << i);
                DWORD_PTR ret = SetThreadAffinityMask(ths[i].native_handle(), affmask);
                //std::cout << "thread " << i << " = " << affmask << ", ret=" << ret << std::endl;
            #else
                cpu_set_t mask;
                CPU_ZERO(&mask);
                CPU_SET(i, &mask);
                int ret = pthread_setaffinity_np(ths[i].native_handle(), sizeof(mask), &mask);
                if (ret < 0) {
                    std::cerr << "set thread affinity failed << std::endl;
                }
                //std::cout << "thread " << i << " = " << affmask << ", ret=" << ret << std::endl;
            #endif
        }
        

        
        //-------------------------------------------------------------------------------
        auto st = std::chrono::high_resolution_clock::now();
        {
            std::unique_lock<std::mutex> guard(StartMtx);
            StartSignal = true;
        }
        StartCond.notify_all();
        //-------------------------------------------------------------------------------
        for (int i = 0; i < _ThreadCount; ++i) { ths[i].join(); }
        const double period_in_ms = 
            static_cast<double>((std::chrono::high_resolution_clock::now() - st).count()) / 
            std::chrono::high_resolution_clock::period::den * 1000.0;
        //-------------------------------------------------------------------------------
        return period_in_ms;
    }
    static void Run()
    {
        _Ty s;
        std::cout << Run([&s]() { s.Push(0); }) << "\t\t";
        std::cout << Run([&s]() { s.Pop(); }) << std::endl;
    }
};


int main()
{
    unsigned num_cpus = std::thread::hardware_concurrency();
    std::cout << "hardware_concurrency " << num_cpus << " threads\n";
    std::cout << "\n";
    //-------------------------------------------------------------------------------
    //press_continue();
    std::cout << "LockedStack with std::mutex" << "\t\t\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, std::mutex>>::Run();
    //-------------------------------------------------------------------------------
    //press_continue()
    std::cout << "LockedStack with SpinLockByTas yield" << "\t\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByTasT<>>>::Run();
    std::cout << "LockedStack with SpinLockByCas yield" << "\t\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByCasT<>>>::Run();

    //-------------------------------------------------------------------------------
    //press_continue();
    std::cout << "LockedStack with SpinLockByTas usleep(1)" << "\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByTasT<1>>>::Run();
    std::cout << "LockedStack with SpinLockByCas usleep(1)" << "\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByCasT<1>>>::Run();
    
    //std::cout << "LockedStack with SpinLockByTas usleep(5)" << "\t\t\t";
    //LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByTasT<5>>>::Run();
    //std::cout << "LockedStack with SpinLockByCas usleep(5)" << "\t\t\t";
    //LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByCasT<5>>>::Run();
    
    //std::cout << "LockedStack with SpinLockByTas usleep(10)" << "\t\t\t";
    //LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByTasT<10>>>::Run();
    //std::cout << "LockedStack with SpinLockByCas usleep(10)" << "\t\t\t";
    //LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByCasT<10>>>::Run();
    //
    //std::cout << "LockedStack with SpinLockByTas usleep(100)" << "\t\t\t";
    //LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByTasT<100>>>::Run();
    //std::cout << "LockedStack with SpinLockByCas usleep(100)" << "\t\t\t";
    //LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByCasT<100>>>::Run();


    //-------------------------------------------------------------------------------
    //press_continue();
    std::cout << "LockedStack with HyperSpinLock1" << "\t\t\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, HyperSpinLock1>>::Run();
    std::cout << "LockedStack with HyperSpinLock1" << "\t\t\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, HyperSpinLock1>>::Run();

    //-------------------------------------------------------------------------------
    //press_continue();
    std::cout << "LockedStack with HyperSpinLock2" << "\t\t\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, HyperSpinLock2>>::Run();
    std::cout << "LockedStack with HyperSpinLock2" << "\t\t\t\t\t";
    LockFreePerformanceTestT<LockedStackT<uint32_t, HyperSpinLock2>>::Run();


    //-------------------------------------------------------------------------------
    //press_continue();
    std::cout << "LockFreeStack with MemoryReclamationByReferenceCounting" << "\t\t";
    LockFreePerformanceTestT<LockFreeStackT<uint32_t, MemoryReclamationByReferenceCountingT<NodeT<uint32_t>>>>::Run();
    std::cout << "LockFreeStack with MemoryReclamationByHazardPointer" << "\t\t";
    LockFreePerformanceTestT<LockFreeStackT<uint32_t, MemoryReclamationByHazardPointerT<NodeT<uint32_t>>>>::Run();


    //-------------------------------------------------------------------------------
    press_continue();
    return 0;
}




/////////////////////////////////////////////////////////////////////////////////////////////////////
/*
#include <vector>                // std::vector
#include <sstream>                // std::stringstream
std::atomic_flag lock_stream = ATOMIC_FLAG_INIT;
std::stringstream stream;

void append_number(int x)
{
    while (lock_stream.test_and_set()) {
    }
    stream << "thread #" << x << '\n';
    lock_stream.clear();
}

int main()
{
    std::vector < std::thread > threads;
    for (int i = 1; i <= 32; ++i)
        threads.push_back(std::thread(append_number, i));
    for (auto & th:threads)
        th.join();

    std::cout << stream.str() << std::endl;;
    return 0;
}
*/



/////////////////////////////////////////////////////////////////////////////////////////////////////
/*
#include <vector>
#include <iostream>

std::atomic_flag lock = ATOMIC_FLAG_INIT;

void f(int n)
{
    for (int cnt = 0; cnt < 100; ++cnt) {
        while (lock.test_and_set(std::memory_order_acquire))  // acquire lock
            ; // spin
        std::cout << "Output from thread " << n << '\n';
        lock.clear(std::memory_order_release);               // release lock
    }
}

int main()
{
    std::vector<std::thread> v;
    for (int n = 0; n < 10; ++n) {
        v.emplace_back(f, n);
    }
    for (auto& t : v) {
        t.join();
    }
}
*/



/////////////////////////////////////////////////////////////////////////////////////////////////////
