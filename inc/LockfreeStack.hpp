#pragma once
#ifndef _ENABLE_ATOMIC_ALIGNMENT_FIX
#  define _ENABLE_ATOMIC_ALIGNMENT_FIX 1
#endif
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include "Node.h"
#include "MemoryReclamationByReferenceCounting.hpp"
#include "MemoryReclamationByHazardPointer.hpp"

template<typename _Ty, typename _MemoryReclamation>
class LockFreeStackT
{
    typedef NodeT<_Ty> Node;
    struct TaggedPointer
    {
        //long p1, p2, p3; // cache line padding
        Node* ptr;
        size_t tag;
        //long p8, p9, p10; // cache line padding

        TaggedPointer() {}
        TaggedPointer(Node* _ptr, size_t _tag) : ptr(_ptr), tag(_tag) {}
    };

    std::atomic<TaggedPointer> head_ = ATOMIC_VAR_INIT(TaggedPointer(nullptr, 0));
    _MemoryReclamation memory_reclamation_;

public:
    ~LockFreeStackT()
    {
        TaggedPointer o(nullptr, 0);
        head_.exchange(o);

        Node* head = o.ptr;
        while (head) {
            Node* node = head;
            head = node->next;
            delete node;
        }
    }

    void Push(const _Ty& val)
    {
        TaggedPointer o = head_.load();
        TaggedPointer n(new Node(val, o.ptr), o.tag + 1);
        while (!head_.compare_exchange_strong(o, n)) {
            n.ptr->next = o.ptr;
            n.tag = o.tag + 1;
            std::this_thread::sleep_for(std::chrono::microseconds(5));
            //std::this_thread::yield();
        }
    }

    std::unique_ptr<_Ty> Pop()
    {
        memory_reclamation_.Addref();
        TaggedPointer o = head_.load(), n;
        while (true)
        {
            if (!o.ptr) break;
            memory_reclamation_.Store(o.ptr);
            // HazardPointer算法儲存(相當於上鎖)後，需要對有效值進行二次確認，否則還是有先刪除的問題
            // 這樣做並沒效率問題，不等的情況CAS操作也會進行循環，因此可以作為針對任何內存回收算法的固定寫法
            const TaggedPointer t = head_.load();
            if (memcmp(&t, &o, sizeof(TaggedPointer))) {
                o = t;
                std::this_thread::sleep_for(std::chrono::microseconds(5));
                //std::this_thread::yield();
                continue;
            }
            n.ptr = o.ptr->next;
            n.tag = o.tag + 1;
            if (head_.compare_exchange_strong(o, n)) {
                break;
            }
        }
        memory_reclamation_.Store(nullptr);
        
        std::unique_ptr<_Ty> ret;
        if (o.ptr) {
            ret.swap(o.ptr->data);
            memory_reclamation_.Release(o.ptr);
        }
        return std::move(ret);
    }
};
