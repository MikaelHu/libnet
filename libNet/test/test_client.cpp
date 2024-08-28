//========================================================================
//[File Name]:test_client.cpp
//[Description]: a test file for tcp client.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <iostream>
#include <string>

//#define OPENSSL
#ifdef OPENSSL
#include "./stream/ClientSSL.h"
using namespace Net::Stream::SSL;
#else
#include "../stream/Client.h"
#endif //OPENSSL

#ifdef OPENSSL
#pragma comment(lib, "./openssl/lib/libssl.lib")
#pragma comment(lib, "./openssl/lib/libcrypto.lib")
#endif

using namespace Net;
using namespace Net::Stream;

int main(int argc, char** argv) {
	asio::io_context io_c;
	asio::io_context::work worker(io_c);
	std::thread t([&io_c]() {io_c.run(); });

#ifdef OPENSSL
	ClientSSL clt(io_c, "127.0.0.1", 9901, "ca.pem");
#else
	Client clt(io_c, "127.0.0.1", 9901);
#endif
	int ret = clt.Open("127.0.0.1", 9900);

	if (ret <= 0) {
		std::cout << "[Test Client] Client connect to Server failed!" << std::endl;
		return -1;
	}

	std::string str;
	while (std::cin >> str) {
		uint8_t err_code = 0;
		clt.Send(str.data(), str.length() + 1, err_code);
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "Main thread wake up...\n";
	clt.Close();
	io_c.stop(); 
	t.join();

	return 0;
}