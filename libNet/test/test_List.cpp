//========================================================================
//[File Name]:test_List.cpp
//[Description]: a test file for List.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../com/List.h"


template<typename T>
struct Data {
	T data;

	Data() = default;
	Data(T d) :data(d) {}
	T operator()() { return data; }
	bool operator>(const Data& t) const { return this->data > t.data; }
	bool operator==(const Data& t) const { return this->data == t.data; }
	bool operator<(const Data& t) const { return this->data < t.data; }
};

typedef struct Data<int> TEST_t;
template<typename T, std::enable_if_t<std::is_integral<T>::value::value, int> = 0>
void print(const char* msg, List<T>* list) {
	printf("%s", msg);
	auto size = list->SizeOf();
	for (unsigned i = 0; i < size; i++)
		printf("%d\t", list->At(i)->data);
	printf("\n");
}
template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
void print(const char* msg, List<T>* list) {
	printf("%s", msg);
	auto size = list->SizeOf();
	for (unsigned i = 0; i < size; i++)
		printf("%f\t", list->At(i)->data);
	printf("\n");
}
template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
void print(const char* msg, List<T>* list) {
	printf("%s", msg);
	auto size = list->SizeOf();
	for (unsigned i = 0; i < size; i++)
		printf("%d\t", list->At(i)->data());
	printf("\n");
}
void test_list() {
	List<TEST_t> list({ Data((TEST_t)8),Data((TEST_t)9), Data((TEST_t)11), Data((TEST_t)5), Data((TEST_t)6), Data((TEST_t)7),(TEST_t)3, Data((TEST_t)10), (TEST_t)2, (TEST_t)5, (TEST_t)1, (TEST_t)4 });
	/*List<TEST_t> list({ Data((TEST_t)5), Data((TEST_t)2), Data((TEST_t)4), \
		Data((TEST_t)3), Data((TEST_t)2), Data((TEST_t)8), Data((TEST_t)6), Data((TEST_t)7),\
		Data((TEST_t)7), Data((TEST_t)10) });*/
	//List<TEST_t> list;
	List<TEST_t>::PList_t p = nullptr;
	for (int i = 0; i < 10; i++)
		//list.PushBack((TEST_t)i);
		//list.Insert((TEST_t)i, [i](const TEST_t& elem) {return elem > (TEST_t)i; });
		list.PushHead((TEST_t)i);
	if (!list.Empty()) {
		printf("T orig:");
		list.Traversal([](const TEST_t& elem) { printf("%d\t", elem.data); });
		printf("\n");
		list.QSort([](const TEST_t& t1, const TEST_t& t2)->bool { return  t1 > t2; });
		print("QSort:", &list);
		auto find = list.FindFirstOf((TEST_t)3);
		list.Reverse();
		print("Reverse:", &list);
		auto back = list.Back();
		printf("back:%d\n", back->data());
		/*printf("back:%d\n", back->data);
		printf("back:%.6f\n", back->data);*/
		auto front = list.Front();
		printf("front:%d\n", front->data());
		p = front;
		if (p->next)
			printf("front next:%d\n", front->next->data());
		/*printf("front:%d\n", front->data);
		printf("front:%.6f\n", front->data);*/
		auto er = list.Erase(0);
		print("Erase 0:", &list);
		auto er1 = list.Erase(2);
		print("Erase 2:", &list);
		auto er2 = list.Erase(back);
		print("Erase back:", &list);
		auto find1 = list.FindFirstOf((TEST_t)5);
		printf("FindFirstOf 5:%d\n", find1->data());
		/*printf("FindFirstOf 4:%d\n", find1->data);
		printf("FindFirstOf 4:%.6f\n", find1->data);*/
		auto find2 = list.FindFirstIf([](const TEST_t& elem) {return elem == (TEST_t)8; });
		printf("FindFirstIf 8:%d\n", find2->data());
		/*printf("FindFirstIf 5:%d\n", find2->data);
		printf("FindFirstIf 5:%.6f\n", find2->data);*/
		auto re = list.Remove((TEST_t)7);
		print("Remove 7:", &list);
		list.Unique<TEST_t>();
		print("Unique:", &list);
		auto re1 = list.RemoveIf([](const TEST_t& elem) {return elem == (TEST_t)9; });
		print("RemoveIf 9:", &list);
		list.Clear();
		print("Clear:", &list);
	}
}

int main(int argc, char** argv) {
	test_list();

	return 0;
}