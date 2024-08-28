#pragma once
//========================================================================
//[File Name]:NMemoryPool.h
//[Description]:A implemetation of memory pool based on linked list,
//              which allow to alloc any standerd type(like:char,int etc.)
//               and user-defined struct.
//[Author]:Nico Hu
//[Date]:2020-07-22
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <stdint.h>
#include <mutex>

#define THREAD_SAFE

template<typename T, typename SizeType = uint16_t, size_t BlockSize = 4096>
class NMemoryPool
{
    union Slot {
        T element;
        Slot* next;
    };
public:
    typedef T* pT;
    // 定义 rebind<U>::other 接口
    template <typename U> struct rebind {
        typedef NMemoryPool<U> other;
    };

    explicit NMemoryPool(SizeType arr_size) noexcept :ArrSize_(arr_size), 
        timesB_(ArrSize_ / BlockSize + 1){ Expand(); }
    ~NMemoryPool() noexcept {
        try
        {
            SlotPtr_t curr = currBlock_;
            while (curr != nullptr) {
                SlotPtr_t prev = curr->next;
                operator delete(reinterpret_cast<void*>(curr));
                curr = prev;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "NMemoryPool raise ex:" << e.what() << std::endl;
            //throw e;
        }        
    }
    pT Alloc() {
#ifdef THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_);
#endif // THREAD_SAFE   
        if (freeSlots_ != nullptr) {
            pT p_r = reinterpret_cast<pT>(freeSlots_);
            freeSlots_ = freeSlots_->next;
            return p_r;
        }
        else {
            if(lastSlot_ < currSlot_ )
                Expand();
            SizeType* p_mem = reinterpret_cast<SizeType*>(currSlot_);
            *p_mem = ArrSize_;
            pT p_r = reinterpret_cast<pT>(p_mem + 1);
            int size = ArrSize_ * sizeof(Slot_t) + sizeof(SizeType);
            currSlot_ = currSlot_ + size;
            return p_r;
        }
    }
    void Free(pT p) {
#ifdef THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_);
#endif // THREAD_SAFE   
        if (p != nullptr) {
            SlotPtr_t p_s = reinterpret_cast<SlotPtr_t>(p);
            p_s->next = freeSlots_;
            freeSlots_ = p_s;
        }
    }
    template <typename U, typename... Args>
    void Construct(U* p, Args&&... args) {
        for(int i =0; i < ArrSize_; i++)
            new (p++) U(std::forward<Args>(args)...);
    }
    template <typename U>
    void Destroy(U* p) {
        for (int i = 0; i < ArrSize_; i++)
            (p++)->~U();
    }

protected:
    void Expand() {
        try
        {
            DataPtr_t newBlock = reinterpret_cast<DataPtr_t>(operator new(BlockSize * timesB_));
            reinterpret_cast<SlotPtr_t>(newBlock)->next = currBlock_;
            currBlock_ = reinterpret_cast<SlotPtr_t>(newBlock);
            DataPtr_t body = newBlock + sizeof(SlotPtr_t);
            uintptr_t result = reinterpret_cast<uintptr_t>(body);
            auto n = alignof(Slot_t);
            size_t bodyPadding = (alignof(Slot_t) - result) % alignof(Slot_t);
            currSlot_ = reinterpret_cast<SlotPtr_t>(body + bodyPadding);
            lastSlot_ = reinterpret_cast<SlotPtr_t>(newBlock + BlockSize - (ArrSize_ * sizeof(Slot_t) + sizeof(SizeType)) + 1);
        }
        catch (std::bad_alloc& e)
        {
            std::cout << "NMemoryPool raise ex:" << e.what() << std::endl;
            throw e;
        }
        catch (...)
        {
            std::cout << "NMemoryPool raise ex." << std::endl;
        }
    }

protected:
    typedef char* DataPtr_t;
    typedef Slot Slot_t;
    typedef Slot* SlotPtr_t;

    SlotPtr_t currBlock_{ nullptr };
    SlotPtr_t currSlot_{ nullptr };
    SlotPtr_t lastSlot_{ nullptr };
    SlotPtr_t freeSlots_{ nullptr };

    SizeType ArrSize_{ 0 };
    uint16_t timesB_{ 1 };
    std::mutex mtx_;

    static_assert(BlockSize >= 2 * sizeof(Slot_t), "[NMemoryPool]:BlockSize is too small!");
};