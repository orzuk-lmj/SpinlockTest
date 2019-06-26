#pragma once
#ifndef _ENABLE_ATOMIC_ALIGNMENT_FIX
#  define _ENABLE_ATOMIC_ALIGNMENT_FIX 1
#endif
#include <atomic>
#include <cassert>

template <class _Node, size_t _MaxPopThreadCount = 16>
class MemoryReclamationByHazardPointerT
{
    struct HazardPointer
    {
        std::atomic<std::thread::id> id;
        std::atomic<_Node*> ptr = ATOMIC_VAR_INIT(nullptr);
    };
    HazardPointer hps_[_MaxPopThreadCount];
    std::atomic<_Node*> will_be_deleted_list_ = ATOMIC_VAR_INIT(nullptr);

    void _ReleaseImpl(_Node* node)
    {
        // 檢查HazardPointers中是否有線程正在訪問當前指針
        size_t i = 0;
        while (i < _MaxPopThreadCount) {
            if (hps_[i].ptr.load() == node)
                break;
            ++i;
        }
        if (i == _MaxPopThreadCount) { // 無任何線程正在訪問當前指針，直接刪除
            delete node;
        }
        else {  // 有線程正在訪問，加入緩存列表
            node->next = will_be_deleted_list_.load();
            while (!will_be_deleted_list_.compare_exchange_strong(node->next, node));
        }
    }

public:
    ~MemoryReclamationByHazardPointerT()
    {
        // 自己不能刪除自己，正常退出HazardPointer始終會持有一個節點，只能在此做清理
        size_t count = 0;
        _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
        while (to_delete_list) {
            _Node* node = to_delete_list;
            to_delete_list = node->next;
            delete node;
            ++count;
        }
        assert(count < 2);
    }

    inline void Addref() {}

    bool Store(_Node* node)
    {
        struct HazardPointerOwner
        {
            HazardPointer* hp;
            HazardPointerOwner(HazardPointer* hps)
                : hp(nullptr)
            {
                for (size_t i = 0; i < _MaxPopThreadCount; ++i) {
                    std::thread::id id;
                    if (hps[i].id.compare_exchange_strong(id, std::this_thread::get_id())) {
                        hp = &hps[i];
                        break;
                    }
                }
            }
            ~HazardPointerOwner()
            {
                if (hp) {
                    hp->ptr.store(nullptr);
                    hp->id.store(std::thread::id());
                }
            }
        };
        thread_local HazardPointerOwner owner(hps_);
        if (!node || !owner.hp) return false;
        owner.hp->ptr.store(node);
        return true;
    }

    bool Release(_Node* node)
    {
        if (!node) return false;
        _ReleaseImpl(node); // 對當前傳入指針進行釋放操作
                            // 循環檢測will_be_deleted_list_, 可以另開一個線程定時檢測效率會更高
        _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
        while (to_delete_list) {
            _Node* node = to_delete_list;
            to_delete_list = node->next;
            _ReleaseImpl(node);
        }
        return true;
    }
};