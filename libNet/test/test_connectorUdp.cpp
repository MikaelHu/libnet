//========================================================================
//[File Name]:test_connectorUdp.cpp
//[Description]: a test file for udp connector.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../stream/Connector.h"
#include "Timer.h"
#include "StreamWriter.h"
#include "Dumphelper.h"


using namespace Net;
using namespace Net::Stream;

#define ConcurrConntion (100)
#define PackerPerConntion (2000)

void test_connector(int port, ConnectorUdp* conn) {
	std::string str = std::to_string(port);
	int i = 0;
	while (i < PackerPerConntion) {
		i++;
		uint8_t err_code{ 0 };

		/*conn->AsyncSend("127.0.0.1", 9900, str.data(), str.length() + 1, [port](const udp::endpoint& endpoint, size_t size, uint8_t ec) {
			static unsigned count = 0;
			auto thd_id = std::this_thread::get_id();
			StreamWriter::Instance()->Write(std::cout, "[Test Connector] thd_id=%d, port=%d\
 count=%d, sent_size=%d, ec=%d", thd_id, port, count++, size, (int)ec);
			});*/
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));

		static int count = 0;
		auto ret = conn->Send("127.0.0.1", 9900, str.data(), str.length() + 1, err_code);
		auto thd_id = std::this_thread::get_id();
		StreamWriter::Instance()->Write(std::cout, "[Test Connector] thd_id=%d, port=%d\
 count=%d, sent_size=%d, ec=%d", thd_id, port, ++count, ret, (int)err_code);
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

	int port_start = 30000;
	int port = port_start;
	std::unordered_map<int, ConnectorUdp*> um_conn;
	while (port < ConcurrConntion + port_start) {
		try
		{
			port++;
			ConnectorUdp* conn = new ConnectorUdp(io_c, "127.0.0.1", port);
			int ret = conn->Open();
			um_conn.emplace(port, conn);
		}
		catch (const std::exception& e)
		{
			std::cout << "[Test Connector_udp] raise ex:" << e.what() << std::endl;
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	Timer tm;
	std::vector<std::shared_ptr<std::thread>> v_thd;
	for (auto var : um_conn) {
		int port = var.first;
		ConnectorUdp* p = var.second;
		std::shared_ptr<std::thread> thd = std::make_shared<std::thread>([port, p] {
			test_connector(port, p);
			});
		v_thd.push_back(thd);
	}
	for (auto var : v_thd) {
		if (var->joinable())
			var->join();
	}

	auto seconds = tm.elapsed_seconds();
	if (seconds > 0) {
		auto pps = (ConcurrConntion * PackerPerConntion) / seconds;
		std::cout << "[Test Connector_udp] PPS=" << pps << std::endl;
	}

	std::cout << "Main thread will sleep for 300s!" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(60 * 5));
	for (auto var : um_conn) {
		var.second->Close();
	}

	io_c.stop();
	for (auto& var : v_threads) {
		if (var.joinable())
			var.join();
	}

	return 0;
}