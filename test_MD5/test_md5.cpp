#include <stdio.h>
#include <string>
#include "MD5.h"
#include "CMD5.h"

using namespace Http::Alg;

void test_md5_1_2() {
	MD5 m;
	MD5::uchar8 str1[] = "ABCDEFGHIJKLMN";
	MD5::uchar8 str2[] = "OPQRSTUVWXYZabcdefghijk";
	MD5::uchar8 str3[] = "lmnopqrstuvwxyz";
	m.UpdateMd5(str1, sizeof(str1) - 1);
	m.UpdateMd5(str2, sizeof(str2) - 1);
	m.UpdateMd5(str3, sizeof(str3) - 1);
	m.Finalize();
	std::string ret = m.ToString();
	std::cout << ret << std::endl;
}
void test_md5_1() {
	try
	{
		MD5 m;
		MD5::char8 str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		m.ComputMd5(str, sizeof(str) - 1);
		std::string ret = m.ToString();
		std::cout << ret << std::endl;
	}
	catch (const std::exception& e)
	{
		printf("test_md5 raise ex:%s", e.what());
	}
}

void test_md5() {
	CMD5 md5;
	char str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	md5.GenerateMD5((unsigned char*)str, strlen(str));
	std::string result = md5.ToString();
	printf("%s\n", result.c_str());
}

int main(int agrc, char* argv[]) {
	test_md5_1_2();
	test_md5_1();
	test_md5();
	  
	return 0;
}