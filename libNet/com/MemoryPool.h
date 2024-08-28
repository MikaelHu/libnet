#pragma once
//========================================================================
//[File Name]:MemoryPool.h
//[Description]: A implemetation of memory pool based on linked list,
//              which allow to alloc any standerd type(like:char,int etc.)
//               and user-defined POD-type data,with wanted size_type, 
//              for example: typename T = int, and SizeType size_type = 16,
//              when call Alloc(), it will return a memory of the size of sizeof(int)*16;
//[Author]:Nico Hu
//[Date]:2020-07-22
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <stdint.h>
#include <mutex>

//#define THREAD_SAFE

template<typename T, typename SizeType = uint16_t>
class MemoryPool
{
    enum { e_EXPAND_SIZE = 64 };
public:
    explicit MemoryPool(SizeType arr_size, size_t pool_size = e_EXPAND_SIZE):ArrSize_(arr_size){
        Expand(pool_size);
    }
    ~MemoryPool(){ Release(); }

    T* Alloc(){
#ifdef THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_);
#endif // THREAD_SAFE  
        if (listMem_ == nullptr){
            Expand();
        }
        MemoryPool<T>* p_head = listMem_;
        listMem_ = listMem_->listMem_;
        return reinterpret_cast<T*>(p_head);
    }
    void Free(T* p){   
#ifdef THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_);
#endif // THREAD_SAFE 
        if (p != nullptr) {
            MemoryPool<T>* p_head = reinterpret_cast<MemoryPool<T>*>(p);
            p_head->listMem_ = listMem_;
            p_head->ArrSize_ = ArrSize_;
            listMem_ = p_head;
        }
    }

protected:
    void Expand(size_t pool_size = e_EXPAND_SIZE){
        try
        {
            unsigned mem_pool_size = sizeof(MemoryPool<T>);
            unsigned node_size = sizeof(T) * ArrSize_ > mem_pool_size ? sizeof(T) * ArrSize_ : mem_pool_size;
            node_size += sizeof(SizeType);
            SizeType* p_mem = reinterpret_cast<SizeType*>(new char[node_size]);
            assert(p_mem);
            *p_mem = ArrSize_;
            MemoryPool<T>* p_last_node = reinterpret_cast<MemoryPool<T>*>(p_mem+1);
            p_last_node->ArrSize_ = ArrSize_;
            p_last_node->listMem_ = nullptr;
            listMem_ = p_last_node;
            for (int i = 0; i < pool_size - 1; ++i) {
                p_mem = reinterpret_cast<SizeType*>(new char[node_size]);
                assert(p_mem);
                *p_mem = ArrSize_;
                MemoryPool<T>* p_node = reinterpret_cast<MemoryPool<T>*>(p_mem+1);
                p_node->ArrSize_ = ArrSize_;
                p_node->listMem_ = nullptr;
                p_last_node->listMem_ = p_node;
                p_last_node = p_last_node->listMem_;
            }
            p_last_node->ArrSize_ = ArrSize_;
            p_last_node->listMem_ = nullptr;
        }
        catch (const std::exception& e)
        {
            std::cout << "MemoryPool raise ex:" << e.what() << std::endl;
            assert(0);
            throw e;
        }
    }
    void Release() {
        try
        {
            MemoryPool<T>* p_head = nullptr;
            for (p_head = this->listMem_; p_head != nullptr; p_head = this->listMem_) {
                this->listMem_ = this->listMem_->listMem_;
                SizeType* p_mem = reinterpret_cast<SizeType*>(p_head);
                delete[] reinterpret_cast<char*>(p_mem - 1); p_mem = nullptr;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "MemoryPool raise ex:" << e.what() << std::endl;
            throw e;
        }        
    }

public:
    MemoryPool<T>* listMem_{ nullptr };
    SizeType ArrSize_{ 0 };
    std::mutex mtx_;
};