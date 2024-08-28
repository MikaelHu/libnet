//========================================================================
//[File Name]:test_CList.cpp
//[Description]: a test file for CList.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../com/CList.h"


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
void print(const char* msg, CLIST::Node<T>* list) {
	printf("%s", msg);
	auto size = CLIST::SizeOf(list);
	for (unsigned i = 0; i < size; i++)
		printf("%d\t", CLIST::At(list, i)->data);
	printf("\n");
}
template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
void print(const char* msg, CLIST::Node<T>* list) {
	printf("%s", msg);
	auto size = CLIST::SizeOf(list);
	for (unsigned i = 0; i < size; i++)
		printf("%f\t", CLIST::At(list, i)->data);
	printf("\n");
}
template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
void print(const char* msg, CLIST::Node<T>* list) {
	printf("%s", msg);
	auto size = CLIST::SizeOf(list);
	for (unsigned i = 0; i < size; i++)
		printf("%d\t", CLIST::At(list, i)->data());
	printf("\n");
}
void test_clist() {
	//CLIST::Node<TEST_t>* list = CLIST::Create({ 5, 2, 4, 3, 2, 8, 6, 7, 7, 10 });
	CLIST::Node<TEST_t>* list = CLIST::Create({ Data((TEST_t)5), Data((TEST_t)2), Data((TEST_t)4), \
		Data((TEST_t)3), Data((TEST_t)2), Data((TEST_t)8), Data((TEST_t)6), Data((TEST_t)2), Data((TEST_t)8), Data((TEST_t)6), Data((TEST_t)7),\
		Data((TEST_t)7), Data((TEST_t)10) });
	//CLIST::Node<TEST_t>* list = CLIST::Create((TEST_t)100);
	for (int i = 0; i < 10; i++)
		CLIST::Insert<TEST_t>(&list, (TEST_t)i, [i](const TEST_t& elem) {return elem < (TEST_t)i; });
		//CLIST::PushBack(&list, (TEST_t)i);
		//CLIST::PushHead(&list, (TEST_t)i);
	if (!CLIST::Empty(list)) {
		printf("T orig:");
		CLIST::Traversal<TEST_t>(list, [](const TEST_t& elem) { printf("%d\t", elem.data); });
		printf("\n");
		CLIST::QSort<TEST_t>(&list, [](const TEST_t& t1, const TEST_t& t2)->bool { return  t1 > t2; });
		print("Sort:", list);
		auto find = CLIST::FindFirstOf(list, (TEST_t)3);
		CLIST::Reverse(&list);
		print("Reverse:", list);
		auto back = CLIST::Back(list);
		printf("back:%d\n", back->data());
		/*printf("back:%d\n", back->data);
		printf("back:%.6f\n", back->data);*/
		auto front = CLIST::Front(list);
		printf("front:%d\n", front->data());
		/*printf("front:%d\n", front->data);
		printf("front:%.6f\n", front->data);*/
		auto er = CLIST::Erase(&list, 0);
		print("Erase 0:", list);
		auto er1 = CLIST::Erase(&list, 2);
		print("Erase 2:", list);
		auto er2 = CLIST::Erase(&list, back);
		print("Erase back:", list);
		auto find1 = CLIST::FindFirstOf(list, (TEST_t)4);
		printf("FindFirstOf 4:%d\n", find1->data());
		/*printf("FindFirstOf 4:%d\n", find1->data);
		printf("FindFirstOf 4:%.6f\n", find1->data);*/
		auto find2 = CLIST::FindFirstIf<TEST_t>(list, [](const TEST_t& elem) {return elem == (TEST_t)5; });
		printf("FindFirstIf 5:%d\n", find2->data());
		/*printf("FindFirstIf 5:%d\n", find2->data);
		printf("FindFirstIf 5:%.6f\n", find2->data);*/
		auto re = CLIST::Remove(&list, (TEST_t)7);
		print("Remove 7:", list);
		CLIST::Unique(&list);
		print("Unique:", list);		
		auto re1 = CLIST::RemoveIf<TEST_t>(&list, [](const TEST_t& elem) {return elem == (TEST_t)9; });
		print("RemoveIf 9:", list);
		CLIST::Clear(&list);
		print("Clear:", list);
	}
}

int main(int argc, char** argv) {
	test_clist();
	return 0;
}