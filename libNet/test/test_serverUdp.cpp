//========================================================================
//[File Name]:test_serverUdp.cpp
//[Description]: a test file for udp server.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../stream/Server.h"

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

	std::shared_ptr<ServerUdp> server(new ServerUdp(io_c, "127.0.0.1", 9900));
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