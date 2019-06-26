#pragma once
#include <thread>
#include <mutex>
#include "Node.h"

template<typename _Ty, typename _Lock>
class LockedStackT
{
    typedef NodeT<_Ty> Node;
public:
    ~LockedStackT()
    {
        std::lock_guard<_Lock> lock(lock_);
        while (head_) {
            Node* node = head_;
            head_ = node->next;
            delete node;
        }
    }

    void Push(const _Ty& val)
    {
        Node* node(new Node(val, nullptr)); 
        {
            // 不需要鎖構造函數，他可能是一個耗時操作
            std::lock_guard<_Lock> lock(lock_);
            node->next = head_;
            head_ = node;
        }
    }

    std::unique_ptr<_Ty> Pop()
    {
        std::unique_ptr<_Ty> ret;
        Node* node;
        {
            // 同上，只需要鎖鏈表本身，其他操作可以放到鏈表外執行
            std::lock_guard<_Lock> lock(lock_);
            node = head_;
            if (node) head_ = node->next;
        }
        if (node) {
            ret.swap(node->data);
            delete node;
        }
        return std::move(ret);
    }

private:
    Node* head_ = nullptr;
    _Lock lock_;
};