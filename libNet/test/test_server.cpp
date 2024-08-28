//========================================================================
//[File Name]:test_server.cpp
//[Description]: a test file for tcp server.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================

//#define OPENSSL
#ifndef OPENSSL
#include "../stream/Server.h"
#else
#include "../stream/ServerSSL.h"
using namespace Net::Stream::SSL;
#endif //OPENSSL

#ifdef OPENSSL
#pragma comment(lib, "../openssl/lib/libssl.lib")
#pragma comment(lib, "../openssl/lib/libcrypto.lib")
#endif

using namespace Net;
using namespace Net::Stream;

const auto N = std::thread::hardware_concurrency();
void test_server() {
	io_context io_c;	
	asio::io_context::work worker(io_c);
	std::vector<std::thread> v_threads;
	for (auto i = 0; i < N; ++i) {
		v_threads.emplace_back(std::thread([&]() {
			io_c.run();
			}));
	}

#ifndef OPENSSL
	std::shared_ptr<ServerTcp> server(new ServerTcp(io_c, "127.0.0.1", 9900));
#else
	SaSSLInfo_t sa_info = {"server.pem", "dh2048.pem", "test"};
	std::shared_ptr<ServerTcpSSL> server(new ServerTcpSSL(io_c, "127.0.0.1", 9900, sa_info));
#endif //OPENSSL

	server->StartA();

	std::cout << "Main thread will for 3600 seconds...\n";
	std::this_thread::sleep_for(std::chrono::seconds(3600));
	std::cout << "Main thread wake up...\n";
	server->Stop();
	io_c.stop();
	
	for (auto& var : v_threads) {
		if (var.joinable())
			var.join();
	}
}
int main(int argc, char** argv) {
	test_server();

	return 0;
}