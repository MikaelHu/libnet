#pragma once
//========================================================================
//[File Name]:NMemoryStorage.h
//[Description]: A implemetation of memory storage based on NMemoryPool,
//               which allowed to Alloc different size of memorys. for
//               example: typename T = int, size list = {1, 16, 8, 64, 128, 10},
//              NMemoryStorage will create all these size of NMemoryPool, and you can
//              call Alloc(16) to get the memory of sizeof(int)*16. if the input size is
//              not include in the size list, the NMemoryStorage will create this kind of 
//              NMemoryPool, and it will aslo return the wanted size of memory.
//[Author]:Nico Hu
//[Date]:2020-07-23
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <initializer_list>
#include <memory>
#include "NMemoryPool.h"


#define SAFE_DELETE(p) {if(p){delete (p); (p) = nullptr;}}
#define SAFE_DELETE_ARRAY(p) {if(p){delete[] (p); (p) = nullptr;}}

//#define THREAD_SAFE

template<typename T, typename SizeType = uint16_t>
class NMemoryStorage {
    struct NMemPoolNode {
        SizeType sizeType{ 0 };
        NMemoryPool<T, SizeType>* pMemPool{ nullptr };
        NMemPoolNode* pNext{ nullptr };
    };

public:
    typedef T* pT;

    explicit NMemoryStorage(std::initializer_list<size_t> il_type_sizes) {
        for (auto it = il_type_sizes.begin(); it != il_type_sizes.end(); ++it) {
            auto p_mem_pool_node = new NMemPoolNode();
            p_mem_pool_node->sizeType = *it;
            p_mem_pool_node->pMemPool = new NMemoryPool<T, SizeType>(*it);
            p_mem_pool_node->pNext = nullptr;
            AddMemPool(&mpMems_, p_mem_pool_node);
        }
    }
    ~NMemoryStorage() { Release(); }

    pT Alloc(size_t size)
    {
#ifdef THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_);
#endif // THREAD_SAFE  
        if (size <= 0)
            return nullptr;
        NMemPoolNode* p_node = mpMems_;
        while (p_node) {
            if (p_node->sizeType >= size) {
                return p_node->pMemPool->Alloc();
            }
            p_node = p_node->pNext;
        }

        auto p_mem_pool_node = new NMemPoolNode();
        p_mem_pool_node->sizeType = size;
        p_mem_pool_node->pMemPool = new NMemoryPool<T, SizeType>(size);
        p_mem_pool_node->pNext = nullptr;
        AddMemPool(&mpMems_, p_mem_pool_node);

        return p_mem_pool_node->pMemPool->Alloc();
    }
    int Free(pT p)
    {
#ifdef THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_);
#endif // THREAD_SAFE
        if (p == nullptr)
            return -2;
        SizeType* p_s = reinterpret_cast<SizeType*>(p);
        SizeType type_size = *(p_s-1);
        pT p_mem = reinterpret_cast<pT>(p);
        NMemPoolNode* p_node = mpMems_;
        while (p_node) {
            if (p_node->sizeType == type_size) {
                p_node->pMemPool->Free(p_mem);
                return 0;
            }
            p_node = p_node->pNext;
        }
        return -1;
    }

protected:
    void Release() {
        NMemPoolNode* p_node = mpMems_;
        while (p_node) {
            NMemPoolNode* p_delete = p_node;
            p_node = p_node->pNext;
            SAFE_DELETE(p_delete->pMemPool);
            SAFE_DELETE(p_delete);
        }
    }
    void AddMemPool(NMemPoolNode** p_head, NMemPoolNode* p_new_node) {
        if (*p_head == nullptr) {
            p_new_node->pNext = *p_head;
            *p_head = p_new_node;
        }
        else {
            NMemPoolNode* p_node = *p_head;
            NMemPoolNode* p_pre = nullptr;
            while (p_node) {
                if (p_node->sizeType > p_new_node->sizeType) {
                    if (p_pre) {
                        p_new_node->pNext = p_node;
                        p_pre->pNext = p_new_node;
                    }
                    else {
                        p_new_node->pNext = p_node;
                        *p_head = p_new_node;
                    }
                    break;
                }
                p_pre = p_node;
                p_node = p_node->pNext;
            }
            if (p_node == nullptr)
                p_pre->pNext = p_new_node;
        }
    }

protected: 
    NMemPoolNode* mpMems_{ nullptr };
    std::mutex mtx_;
};