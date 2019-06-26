#pragma once
#ifndef _ENABLE_ATOMIC_ALIGNMENT_FIX
#  define _ENABLE_ATOMIC_ALIGNMENT_FIX 1
#endif
#include <atomic>
#include <cassert>

template <typename _Node>
class MemoryReclamationByReferenceCountingT
{
    std::atomic<size_t> counter_ = ATOMIC_VAR_INIT(0);
    std::atomic<_Node*> will_be_deleted_list_ = ATOMIC_VAR_INIT(nullptr);

    void InsertToList(_Node* first, _Node* last)
    {
        last->next = will_be_deleted_list_;
        while (!will_be_deleted_list_.compare_exchange_strong(last->next, first)) {
            //std::this_thread::sleep_for(std::chrono::microseconds(1));
            //std::this_thread::yield();
        };
    }
    void InsertToList(_Node* head)
    {
        _Node* last = head;
        while (_Node* next = last->next) { 
            last = next;
        }
        InsertToList(head, last);
    }

public:
    ~MemoryReclamationByReferenceCountingT()
    {
        // 如果線程正常退出，Reference Counting算法能刪除所有數據
        assert(will_be_deleted_list_.load() == nullptr);
        assert(counter_.load() == 0);
        _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
        while (to_delete_list) {
            _Node* node = to_delete_list;
            to_delete_list = node->next;
            delete node;
        }
    }

    inline void Addref()
    {
        ++counter_;
    }

    inline bool Store(_Node* node) { return true; }

    bool Release(_Node* node)
    {
        if (!node) return false;
        if (counter_ == 1)
        {
            _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
            if (!--counter_) {
                while (to_delete_list) {
                    _Node* node = to_delete_list;
                    to_delete_list = node->next;
                    delete node;
                }
            }
            else if (to_delete_list) {
                InsertToList(to_delete_list);
            }
            delete node;
        }
        else {
            if (node) InsertToList(node, node);
            --counter_;
        }
        return true;
    }
};