//========================================================================
//[File Name]:test_httpClient.cpp
//[Description]: a test file for http client.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <thread>
#include <vector>
#include <atomic>

//#define OPENSSL
#include "../http/HttpClient.h"
#include "../com/Timer.h"

using namespace Http;

#ifdef OPENSSL
#pragma comment(lib, "./openssl/lib/libssl.lib")
#pragma comment(lib, "./openssl/lib/libcrypto.lib")
#endif

constexpr int ConcurrConntion = 16;
constexpr int PackerPerConntion = 100;


void do_send(int id, int count, HttpClient* client, const char* request) {
	client->SendReq(id, request, strlen(request) + 1, [id, count, request](std::string content, uint8_t ec) {
		StreamWriter::Instance()->Write(std::cout, "[test_httpClient] Conn Id=%d, Count=%d, Request=%s, resp content:%s, ec=%d", id, count, request, content.c_str(), (int)ec);
	});
}
void test_httpClient(HttpClient* client, const char* c_req[], int id) {
	static std::atomic_int count{0};
	for(int k = 0; k < PackerPerConntion; k++)
		for (int i = 0; i < 6/*sizeof(c_req)*/; i++) {
			do_send(id, ++count, client, c_req[i]);
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
}

void do_download_file(int id, HttpClient* client, const char* request) {
	std::string req = request;
	auto pos = req.find_last_of('/');
	if (pos != std::string::npos) {
		std::string file_name = req.substr(pos + 1);
		std::string save_name = std::to_string(id) +"_"+ file_name;

		try
		{
			int file_length{ 0 };
			std::ifstream ifs(save_name, std::ios::in | std::ios::binary);
			if (ifs) {
				ifs.seekg(0, ifs.end);
				file_length = ifs.tellg();
				ifs.close();
			}
			std::ofstream ofs(save_name, std::ios::out | std::ios::binary | std::ios::app);			

			client->SendReq(id, request, strlen(request) + 1, [id, request, &ofs](std::string content, uint8_t ec) {
				StreamWriter::Instance()->Write(std::cout, "[test_httpClient] Conn Id=%d, Request=%s, content len=%d, ec=%d", \
					id, request, content.length(), (int)ec);

				if ((int)ec == 0xFF) {
					StreamWriter::Instance()->Write(std::cout, "[test_httpClient] MD5 verify success.");
				}
				else if ((int)ec == 0xFE) {
					StreamWriter::Instance()->Write(std::cout, "[test_httpClient] MD5 verify failed.");
				}

				if (ofs) {
					static int count = 0;
					if (!content.empty()) {
						count++;
						ofs.write(content.c_str(), content.length());
						if(count%100 == 0)
							ofs.flush();
						StreamWriter::Instance()->Write(std::cout, "[test_httpClient] chunked file count=%d.", count);
					}
				}
				else {
					StreamWriter::Instance()->Write(std::cout, "[test_httpClient] ofs is closed!");
				}
			});

			if(ofs.is_open())
				ofs.close();
		}
		catch (const std::exception& e)
		{
			StreamWriter::Instance()->Write(std::cout, "[test_httpClient] Recv chunk file raise ex=%s", e.what());
		}		
	}
}

void test_httpClient_File(HttpClient* client, const char* c_req[], int id) {
	for (int i = 0; i < 1/*sizeof(c_req)*/; i++) {
		do_download_file(id, client, c_req[i]);
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

int main(int argc, char* argv[]) {
	const char* CRequest[] = {
	"GET /user",
	"GET /guide",
	"GET /traveral",
	"GET /kooke",
	"GET /default",
	"GET /bad_request"
	};
	const char* CRequestFile[] = {
	"GET /file"
	};

	HttpClient client;
	std::vector<int> v_id;
#ifdef OPENSSL
	for (int i = 0; i < ConcurrConntion; i++) {
		int id = client.Open("127.0.0.1", 443);
		if (id > 0)
			v_id.emplace_back(id);
	}	
#else
	for (int i = 0; i < ConcurrConntion; i++) {
		int id = client.Open("127.0.0.1", 80);
		if (id > 0)
			v_id.emplace_back(id);
	}
#endif

	Timer tm;
	std::vector<std::thread*> v_thd;
	for (int i = 0; i < v_id.size(); i++) {
		std::thread* thd = new std::thread([i, &CRequestFile, &client, &v_id]() {
			//test_httpClient(&client, CRequest, v_id[i]);
			test_httpClient_File(&client, CRequestFile, v_id[i]);
		});
		v_thd.emplace_back(thd);
	}

	for (auto var : v_thd) {
		if (var && var->joinable())
			var->join();
	}
	v_thd.clear();
	for(auto var : v_id)
		client.Close(var);
	v_id.clear();

	auto seconds = tm.elapsed_seconds();
	if (seconds > 0) {
		std::cout << "[test_httpClient] Elapsed-Time(s) =" << seconds << std::endl;
		auto pps = (ConcurrConntion * PackerPerConntion * sizeof(CRequest)) / seconds;
		std::cout << "[test_httpClient] PPS=" << pps << std::endl;
	}

	StreamWriter::Instance()->Write(std::cout, "[test_httpClient] The main thread will sleep for 300s.");
	std::this_thread::sleep_for(std::chrono::seconds(60 * 5));

	return 0;
}