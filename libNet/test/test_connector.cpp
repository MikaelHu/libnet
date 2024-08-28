//========================================================================
//[File Name]:test_connector.cpp
//[Description]: a test file for tcp connector.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <atomic>

//#define OPENSSL
#ifndef OPENSSL
#include "../stream/Connector.h"
#else
#include "../stream/ConnectorSSL.h"
using namespace Net::Stream::SSL;
#endif //OPENSSL

#include "Timer.h"
#include "StreamWriter.h"
#include "Dumphelper.h"

using namespace Net;
using namespace Net::Stream;

#define ConcurrConntion (500)
#define PackerPerConntion (600)

#ifdef OPENSSL
#pragma comment(lib, "./openssl/lib/libssl.lib")
#pragma comment(lib, "./openssl/lib/libcrypto.lib")
#endif

void test_connector(ConnectorBase* conn, int session_id) {
	if (!conn->IsConnected(session_id)) {
		std::cout << "[Test Connector] Connect to Server failed! session_id=" << session_id << std::endl;
		return;
	}

	std::string str = std::to_string(session_id);
	int i = 0;
	while (i < PackerPerConntion) {
		i++;
		uint8_t err_code{ 0 };

		/*conn->AsyncSend(session_id, str.data(), str.length() + 1, [session_id](size_t size, uint8_t ec) {
			static unsigned count = 0;
			auto thd_id = std::this_thread::get_id();
			StreamWriter::Instance()->Write(std::cout, "[Test Connector] thd_id = %d, session id = %d, Packet num =%d, send size=%d", thd_id, session_id, ++count, size);
			});*/
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));

		static std::atomic_int num = 0;
		auto ret = conn->Send(session_id, str.data(), str.length() + 1, err_code);
		//auto thd_id = std::this_thread::get_id();
		StreamWriter::Instance()->Write(std::cout, "[Test Connector] session id = %d,\
 Packet num =%d, sent_size=%d, ec=%d", /*thd_id, */session_id, ++num, ret, (int)err_code);
	}
	//conn->Close(session_id);
}
const auto N = std::thread::hardware_concurrency();
int main(int argc, char** argv) {
	SetUnhandledExceptionFilter(ExceptionFilter);

	asio::io_context io_c;
	asio::io_context::work worker(io_c);
	std::vector<std::thread> v_threads;
	for (auto i = 0; i < N; ++i) {
		v_threads.emplace_back(std::thread([&]() {
			io_c.run();
			}));
	}

#ifndef OPENSSL
	ConnectorTcp conn(io_c, "127.0.0.1", 0);
#else
	ConnectorTcpSSL conn(io_c, "127.0.0.1", 0, "ca.pem");
#endif //OPENSSL

	int num{0};
	std::vector<int> v_conn;
	int port_start = 30000;
	while (num < ConcurrConntion) {
		try
		{
			num++;
			int session_id = conn.Open("127.0.0.1", 9900, true);
			printf("[Test Connector] session_id=%d\n", session_id);
			v_conn.push_back(session_id);
		}
		catch (const std::exception& e)
		{
			std::cout << "[Test Connector] raise ex:" << e.what() << std::endl;
		}		
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Timer tm;
	std::vector<std::thread*> v_thd;
	for(auto var : v_conn){
		std::thread* thd = new std::thread([var, &conn] {
			test_connector(&conn, var);
			});
		v_thd.emplace_back(thd);
	}
	for (auto var : v_thd) {
		if (var->joinable())
			var->join();
	}

	auto seconds = tm.elapsed_seconds();
	if (seconds > 0) {
		auto pps = (ConcurrConntion * PackerPerConntion) / seconds;
		std::cout << "[Test Connector] PPS=" << pps << std::endl;
	}
	
	std::cout << "Main thread will sleep for 300s!" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(60 * 5));
	for (auto var : v_conn) {
		conn.Close(var);
	}

	io_c.stop();
	for (auto& var : v_threads) {
		if (var.joinable())
			var.join();
	}
	return 0;
}