#pragma once
//========================================================================
//[File Name]:CList.h
//[Description]: An implemetation of linked list with various Interfaces. 
// All interfaces are in the formation of c functions(employed some c++11 features).
// It supports not only standard data type, but also user-defined data struct, which 
// need to implement some interface, like: operator>,<,==, etc.It is easy to use.
//[Author]:Nico Hu
//[Date]:2020-07-25
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <type_traits>
#include <functional>
#include <initializer_list>

namespace CLIST {
    template<typename T>
    struct Node {
        T        data;
        Node<T>* next;
    };

    template<typename T>
    Node<T>* Create(const T& val) {
        Node<T>* head = new Node<T>;
        head->data = val;
        head->next = nullptr;
        return head;
    }
    template<typename T>
    Node<T>* Create(std::initializer_list<T> init_list) {
        Node<T>* head = nullptr;
        for (auto it = init_list.begin(); it != init_list.end(); it++) {
            if (head == nullptr)
                head = Create(*it);
            else
                PushBack(&head, *it);
        }
        return head;
    }
    template<typename T>
    int Destroy(Node<T>** head) {
        Node<T>* curr = *head;
        while (curr != nullptr) {
            Node<T>* next = curr->next;
            delete curr;
            curr = next;
        }
        *head = nullptr;
        return 0;
    }
    template<typename T>
    int Clear(Node<T>** head) {
        return Destroy(head);
    }
    template<typename T>
    bool Empty(Node<T>* head) {
        return head == nullptr;
    }
    template<typename T>
    unsigned SizeOf(Node<T>* head) {
        Node<T>* curr = head;
        unsigned size = 0;
        while (curr != nullptr) {
            size++;
            curr = curr->next;
        }
        return size;
    }
    template<typename T>
    int Insert(Node<T>** head, const T& val, std::function<bool(const T&)> func = nullptr) {
        Node<T>* node = new Node<T>;
        node->data = val;
        node->next = nullptr;

        if (*head == nullptr || func == nullptr) {
            node->next = *head;
            *head = node;
            return 0;
        }
        Node<T>* curr = *head;
        Node<T>* pre = nullptr;
        while (curr != nullptr) {
            if (func(curr->data)) {
                if (pre == nullptr) {
                    node->next = *head;
                    *head = node;
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
        if (curr == nullptr) {
            pre->next = node;
        }
        return 0;
    }
    template<typename T>
    int PushHead(Node<T>** head, const T& val) {
        Node<T>* node = new Node<T>;
        node->data = val;
        node->next = *head;
        *head = node;
        return 0;
    }
    template<typename T>
    int PushBack(Node<T>** head, const T& val) {
        Node<T>* node = new Node<T>;
        node->data = val;
        node->next = nullptr;
        if (*head == nullptr) {
            *head = node;
            return 0;
        }
        Node<T>* tail = *head;
        while (tail->next != nullptr)
            tail = tail->next;
        tail->next = node;
        return 0;
    }
    template<typename T>
    Node<T>* At(Node<T>* head, unsigned index) {
        Node<T>* curr = head;
        while (curr != nullptr && index > 0) {
            curr = curr->next;
            index--;
        }
        return curr == nullptr ? nullptr : curr;
    }
    template<typename T>
    Node<T>* Front(Node<T>* head) {
        return head;
    }
    template<typename T>
    Node<T>* Back(Node<T>* head) {
        if (head == nullptr)
            return nullptr;
        Node<T>* curr = head;
        while (curr->next != nullptr)
            curr = curr->next;
        return curr;
    }
    template<typename T>
    int Erase(Node<T>** head, unsigned index) {
        if (*head == nullptr)
            return -1;
        Node<T>* curr = *head;
        Node<T>* pre = nullptr;
        while (curr != nullptr && index > 0) {
            pre = curr;
            curr = curr->next;
            index--;
        }
        if (curr != nullptr) {
            if (pre == nullptr) {
                *head = curr->next;
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
    template<typename T>
    int Erase(Node<T>** head, Node<T>* node) {
        if (*head == nullptr || node == nullptr)
            return -1;
        Node<T>* curr = *head;
        Node<T>* pre = nullptr;
        while (curr != nullptr && curr != node) {
            pre = curr;
            curr = curr->next;
        }
        if (curr == node) {
            if (pre == nullptr) {
                *head = curr->next;
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
    int Remove(Node<T>** head, const T& val) {
        Node<T>* curr = *head;
        Node<T>* pre = nullptr;
        while (curr != nullptr) {
            T t = curr->data - val;
            if (t > -1.0e-6 && t < 1.0e-6) {
                if (pre == nullptr) {
                    *head = curr->next;
                    delete curr;
                    curr = *head;
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
    int Remove(Node<T>** head, const T& val) {
        Node<T>* curr = *head;
        Node<T>* pre = nullptr;
        while (curr != nullptr) {
            if (curr->data == val) {
                if (pre == nullptr) {
                    *head = curr->next;
                    delete curr;
                    curr = *head;
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
    template<typename T>
    int RemoveIf(Node<T>** head, std::function<bool(const T&)> func) {
        Node<T>* curr = *head;
        Node<T>* pre = nullptr;
        while (curr != nullptr) {
            if (func(curr->data)) {
                if (pre == nullptr) {
                    *head = curr->next;
                    delete curr;
                    curr = *head;
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
    template<typename T>
    int Sort(Node<T>** head, std::function<bool(const T&, const T&)> func) {      
        unsigned size = SizeOf(*head);
        while (size - 1 > 0) {
            Node<T>* curr = *head;
            Node<T>* pre = nullptr;
            while (curr != nullptr && curr->next != nullptr) {
                if (func(curr->data, curr->next->data)) {
                    if (pre == nullptr) {
                        *head = curr->next;
                        curr->next = (*head)->next;
                        (*head)->next = curr;
                        pre = *head;
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
    template<typename T>
    void QSortInner(Node<T>* pbegin, Node<T>* pend, std::function<bool(const T&, const T&)> func);
    template<typename T>
    Node<T>* Partion(Node<T>* pbegin, Node<T>* pend, std::function<bool(const T&, const T&)> func);
    template<typename T>
    void Swap(T* a, T* b);
    template<typename T>
    int QSort(Node<T>** head, std::function<bool(const T&, const T&)> func)
    {
        Node<T>* pbegin = *head;
        Node<T>* pend = nullptr;
        QSortInner<T>(pbegin, pend, func);
        return 0;
    }
    template<typename T>
    void QSortInner(Node<T>* pbegin, Node<T>* pend, std::function<bool(const T&, const T&)> func)
    {
        if (pbegin == pend)
            return;
        Node<T>* mid = Partion(pbegin, pend, func);
        QSortInner<T>(pbegin, mid, func);
        QSortInner<T>(mid->next, pend, func);
    }
    template<typename T>
    Node<T>* Partion(Node<T>* pbegin, Node<T>* pend, std::function<bool(const T&, const T&)> func)
    {
        if (pbegin == pend || pbegin->next == pend)
            return pbegin;
        T key = pbegin->data;
        Node<T>* p = pbegin;
        Node<T>* q = p->next;
        while (q != pend)
        {
            if (func(q->data, key))
            {
                p = p->next;
                Swap(&(p->data), &(q->data));
                //Swap(&p->data, &q->data);                  
            }
            q = q->next;
        }
        Swap(&(p->data), &(pbegin->data));
        return p;
    }
    template<typename T>
    void Swap(T* a, T* b)
    {
        T tmp = *a; *a = *b; *b = tmp;
    }
    template<typename T>
    int Reverse(Node<T>** head) {
        Node<T>* a = nullptr, * b = nullptr;
        while (*head != nullptr)
        {
            b = (*head)->next;
            (*head)->next = a;
            a = (*head);
            (*head) = b;
        }
        *head = a;
        return 0;
    }
    template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    Node<T>* FindFirstOf(Node<T>* head, const T& val) {
        Node<T>* curr = head;
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
    template<typename T, std::enable_if_t<!std::is_floating_point<T>::value, int> = 0>
    Node<T>* FindFirstOf(Node<T>* head, const T& val) {
        Node<T>* curr = head;
        while (curr != nullptr) {
            if (curr->data == val)
                break;
            curr = curr->next;
        }
        if (curr != nullptr)
            return curr;
        return nullptr;
    }
    template<typename T>
    Node<T>* FindFirstIf(Node<T>* head, std::function<bool(const T&)> func) {
        Node<T>* curr = head;
        while (curr != nullptr) {
            if (func(curr->data))
                break;
            curr = curr->next;
        }
        if (curr != nullptr)
            return curr;
        return nullptr;
    }
    template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    int Unique(Node<T>** head) {
        QSort<T>(head, [](const T& t1, const T& t2)->bool { return t1 > t2; });
        Node<T>* curr = *head;
        while (curr != nullptr && curr->next != nullptr) {
            T t = curr->data - curr->next->data;
            if (t > -1.0e-6 && t < 1.0e-6) {
                Node<T>* del = curr->next;
                curr->next = del->next;
                delete del;
                continue;
            }
            curr = curr->next;
        }
        return 0;
    }
    template<typename T, std::enable_if_t<!std::is_floating_point<T>::value, int> = 0>
    int Unique(Node<T>** head) {
        QSort<T>(head, [](const T& t1, const T& t2)->bool { return  t1 > t2; });
        Node<T>* curr = *head;
        while (curr != nullptr && curr->next != nullptr) {
            if (curr->data == curr->next->data) {
                Node<T>* del = curr->next;
                curr->next = del->next;
                delete del;
                continue;
            }
            curr = curr->next;
        }
        return 0;
    }
    template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    void Traversal(Node<T>* head) {
        while (head != nullptr) {
            printf("%.6f\t", head->data);
            head = head->next;
        }
    }
    template<typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
    void Traversal(Node<T>* head) {
        while (head != nullptr) {
            printf("%d\t", head->data);
            head = head->next;
        }
    }
    template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
    void Traversal(Node<T>* head, std::function<void(const T&)> func) {
        while (head != nullptr) {
            func(head->data);
            head = head->next;
        }
    }
}