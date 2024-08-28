#pragma once
//========================================================================
//[File Name]:List.h
//[Description]: An implemetation of linked list with C++11 with various Interfaces. 
// It supports not only standard data type, but also user-defined data struct, which 
// need to implement some interface, like: operator>,<,==, etc.It is easy to use.
//[Author]:Nico Hu
//[Date]:2020-07-25
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <type_traits>
#include <functional>
#include <initializer_list>


template<typename T>
class List {
    struct Node {
        T     data;
        Node* next;
    };
 public:
    typedef Node* PList_t;

public:
    explicit List() = default;
    explicit List(const T& val){
        elem_ = new Node;
        elem_->data = val;
        elem_->next = nullptr;
    }
    explicit List(std::initializer_list<T> init_list) {
        for (auto it = init_list.begin(); it != init_list.end(); it++) {
            if (elem_ == nullptr) {
                elem_ = new Node;
                elem_->data = *it;
                elem_->next = nullptr;
            }
            else
                PushBack(*it);
        }
    }
    ~List() {
        Clear();
    }

    int Clear() {
        PList_t curr = elem_;
        while (curr != nullptr) {
            PList_t next = curr->next;
            delete curr;
            curr = next;
        }
        elem_ = nullptr;
        return 0;
    }
    bool Empty() {
        return elem_ == nullptr;
    }
    unsigned SizeOf() {
        PList_t curr = elem_;
        unsigned size = 0;
        while (curr != nullptr) {
            size++;
            curr = curr->next;
        }
        return size;
    }
    int Insert(const T& val, std::function<bool(const T&)> func = nullptr) {
        PList_t node = new Node;
        node->data = val;
        node->next = nullptr;

        if (elem_ == nullptr || func == nullptr) {
            node->next = elem_;
            elem_ = node;
            return 0;
        }
        PList_t curr = elem_;
        PList_t pre = nullptr;
        while (curr != nullptr) {
            if (func(curr->data)) {
                if (pre == nullptr) {
                    node->next = elem_;
                    elem_ = node;
                }
                else {
                    pre->next = node;
                    node->next = curr;
                }
                break;
            }
            pre = curr;
            curr = curr->next;
        }
        if (curr == nullptr)
            pre->next = node;
        return 0;
    }
    int PushHead(const T& val) {
        PList_t node = new Node;
        node->data = val;
        node->next = elem_;
        elem_ = node;
        return 0;
    }
    int PushBack(const T& val) {
        PList_t node = new Node;
        node->data = val;
        node->next = nullptr;
        if (elem_ == nullptr) {
            elem_ = node;
            return 0;
        }
        PList_t tail = elem_;
        while (tail->next != nullptr)
            tail = tail->next;
        tail->next = node;
        return 0;
    }
    PList_t At(unsigned index) {
        PList_t curr = elem_;
        while (curr != nullptr && index > 0) {
            curr = curr->next;
            index--;
        }
        return curr == nullptr ? nullptr : curr;
    }
    PList_t Front() {
        return elem_;
    }
    PList_t Back() {
        if (elem_ == nullptr)
            return nullptr;
        PList_t curr = elem_;
        while (curr->next != nullptr)
            curr = curr->next;
        return curr;
    }
    template<typename T1 = unsigned, std::enable_if_t<std::is_integral<T1>::value, int> = 0>
    int Erase(T1 index) {
        if (elem_ == nullptr)
            return -1;
        PList_t curr = elem_;
        PList_t pre = nullptr;
        while (curr != nullptr && index > 0) {
            pre = curr;
            curr = curr->next;
            index--;
        }
        if (curr != nullptr) {
            if (pre == nullptr) {
                elem_ = curr->next;
                delete curr;

            }
            else {
                pre->next = curr->next;
                delete curr;
            }
            return 0;
        }
        return -1;
    }
    template<typename T1 = PList_t, std::enable_if_t<!std::is_integral<T1>::value && std::is_same<PList_t, T1>::value, int> = 0>
    int Erase(T1 node) {
        if (elem_ == nullptr || node == nullptr)
            return -1;
        PList_t curr = elem_;
        PList_t pre = nullptr;
        while (curr != nullptr && curr != node) {
            pre = curr;
            curr = curr->next;
        }
        if (curr == node) {
            if (pre == nullptr) {
                elem_ = curr->next;
                delete curr;
            }
            else {
                pre->next = curr->next;
                delete curr;
            }
            return 0;
        }
        return -1;
    }
    template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    int Remove(const T& val) {
        PList_t curr = elem_;
        PList_t pre = nullptr;
        while (curr != nullptr) {
            T t = curr->data - val;
            if (t > -1.0e-6 && t < 1.0e-6) {
                if (pre == nullptr) {
                    elem_ = curr->next;
                    delete curr;
                    curr = elem_;
                }
                else {
                    pre->next = curr->next;
                    delete curr;
                    curr = pre->next;
                }
                continue;
            }
            pre = curr;
            curr = curr->next;
        }
        return 0;
    }
    template<typename T, std::enable_if_t<!std::is_floating_point<T>::value, int> = 0>
    int Remove(const T& val) {
        PList_t curr = elem_;
        PList_t pre = nullptr;
        while (curr != nullptr) {
            if (curr->data == val) {
                if (pre == nullptr) {
                    elem_ = curr->next;
                    delete curr;
                    curr = elem_;
                }
                else {
                    pre->next = curr->next;
                    delete curr;
                    curr = pre->next;
                }
                continue;
            }
            pre = curr;
            curr = curr->next;
        }
        return 0;
    }
    int RemoveIf(std::function<bool(const T&)> func) {
        PList_t curr = elem_;
        PList_t pre = nullptr;
        while (curr != nullptr) {
            if (func(curr->data)) {
                if (pre == nullptr) {
                    elem_ = curr->next;
                    delete curr;
                    curr = elem_;
                }
                else {
                    pre->next = curr->next;
                    delete curr;
                    curr = pre->next;
                }
                continue;
            }
            pre = curr;
            curr = curr->next;
        }
        return 0;
    }
    int Sort(std::function<bool(const T&, const T&)> func) {
        unsigned size = SizeOf();
        while (size - 1 > 0) {
            PList_t curr = elem_;
            PList_t pre = nullptr;
            while (curr != nullptr && curr->next != nullptr) {
                if (func(curr->data, curr->next->data)) {
                    if (pre == nullptr) {
                        elem_ = curr->next;
                        curr->next = elem_->next;
                        elem_->next = curr;
                        pre = elem_;
                    }
                    else {
                        pre->next = curr->next;
                        curr->next = pre->next->next;
                        pre->next->next = curr;
                        pre = pre->next;
                    }
                    continue;
                }
                pre = curr;
                curr = curr->next;
            }
            size--;
        }
        return 0;
    }
    int QSort(std::function<bool(const T&, const T&)> func)
    {
        PList_t pbegin = elem_;
        PList_t pend = nullptr;
        QSortInner(pbegin, pend, func);
        return 0;
    }
    int Reverse() {
        PList_t a = nullptr, b = nullptr;
        while (elem_ != nullptr)
        {
            b = elem_->next;
            elem_->next = a;
            a = elem_;
            elem_ = b;
        }
        elem_ = a;
        return 0;
    }
    template<typename T1 = T, std::enable_if_t<std::is_floating_point<T1>::value && std::is_same<T, T1>::value, int> = 0>
    PList_t FindFirstOf(const T& val) {
        PList_t curr = elem_;
        while (curr != nullptr) {
            T t = curr->data - val;
            if (t > -1.0e-6 && t < 1.0e-6)
                break;
            curr = curr->next;
        }
        if (curr != nullptr)
            return curr;
        return nullptr;
    }
    template<typename T1 = T, std::enable_if_t<!std::is_floating_point<T1>::value && std::is_same<T, T1>::value, int> = 0>
    PList_t FindFirstOf(const T& val) {
        PList_t curr = elem_;
        while (curr != nullptr) {
            if (curr->data == val)
                break;
            curr = curr->next;
        }
        if (curr != nullptr)
            return curr;
        return nullptr;
    }
    PList_t FindFirstIf(std::function<bool(const T&)> func) {
        PList_t curr = elem_;
        while (curr != nullptr) {
            if (func(curr->data))
                break;
            curr = curr->next;
        }
        if (curr != nullptr)
            return curr;
        return nullptr;
    }
    template<typename T1, std::enable_if_t<std::is_floating_point<T1>::value&& std::is_same<T, T1>::value, int> = 0>
    int Unique() {
        QSort([](const T& t1, const T& t2)->bool { return t1 > t2; });
        PList_t curr = elem_;
        while (curr != nullptr && curr->next != nullptr) {
            T t = curr->data - curr->next->data;
            if (t > -1.0e-6 && t < 1.0e-6) {
                PList_t del = curr->next;
                curr->next = del->next;
                delete del;
                continue;
            }
            curr = curr->next;
        }
        return 0;
    }
    template<typename T1, std::enable_if_t<!std::is_floating_point<T1>::value && std::is_same<T, T1>::value, int> = 0>
    int Unique() {
        QSort([](const T& t1, const T& t2)->bool { return  t1 > t2; });
        PList_t curr = elem_;
        while (curr != nullptr && curr->next != nullptr) {
            if (curr->data == curr->next->data) {
                PList_t del = curr->next;
                curr->next = del->next;
                delete del;
                continue;
            }
            curr = curr->next;
        }
        return 0;
    }
    template<typename T1 = T, std::enable_if_t<std::is_floating_point<T1>::value && std::is_same<T, T1>::value, int> = 0>
    void Traversal() {
        PList_t curr = elem_;
        while (curr != nullptr) {
            printf("%.6f\t", curr->data);
            curr = curr->next;
        }
    }
    template<typename T1 = T, std::enable_if_t<std::is_integral<T1>::value&& std::is_same<T, T1>::value, int> = 0>
    void Traversal() {
        PList_t curr = elem_;
        while (curr != nullptr) {
            printf("%d\t", curr->data);
            curr = curr->next;
        }
    }
    template<typename T1 = T, std::enable_if_t<std::is_class<T1>::value && std::is_same<T, T1>::value, int> = 0>
    void Traversal(std::function<void(const T&)> func) {
        PList_t curr = elem_;
        while (curr != nullptr) {
            func(curr->data);
            curr = curr->next;
        }
    }

protected:
    void QSortInner(PList_t pbegin, PList_t pend, std::function<bool(const T&, const T&)> func)
    {
        if (pbegin == pend)
            return;
        PList_t mid = Partion(pbegin, pend, func);
        QSortInner(pbegin, mid, func);
        QSortInner(mid->next, pend, func);
    }
    PList_t Partion(PList_t pbegin, PList_t pend, std::function<bool(const T&, const T&)> func)
    {
        if (pbegin == pend || pbegin->next == pend)
            return pbegin;
        T key = pbegin->data;
        PList_t p = pbegin;
        PList_t q = p->next;
        while (q != pend)
        {
            if(func(q->data, key))
            {
                p = p->next;
                Swap(&(p->data) ,&(q ->data));
                //Swap(&p->data, &q->data);                  
            }
            q = q->next;
        }
        Swap(&(p->data), &(pbegin->data));
        return p;
    }
    void Swap(T* a, T* b)
    {
        T tmp = *a; *a = *b; *b = tmp;
    }

private:
    PList_t elem_{nullptr};
};