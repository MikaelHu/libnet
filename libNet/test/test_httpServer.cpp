//========================================================================
//[File Name]:test_httpServer.cpp
//[Description]: a test file for http server.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <atomic>

//#define OPENSSL
#include "../http/HttpServer.h"

using namespace Http;

#ifdef OPENSSL
#pragma comment(lib, "./openssl/lib/libssl.lib")
#pragma comment(lib, "./openssl/lib/libcrypto.lib")
#endif


void test_httpServer() {
#ifdef OPENSSL
	HttpServer server("127.0.0.1", 443, ".");
#else
	HttpServer server("127.0.0.1", 80, ".");
#endif

	const char user[] = "The User is javie.��̲�ϵ��ο�Ͷ��������ɳ̲���޹���";
	const char guide[] = "Guide: guide, lead, steer, pilot, engineer mean to direct in a course or show the way to be followed. guide implies intimate knowledge of the way and of all its difficulties and dangers. guided the scouts \
through the cave lead implies showing the way and often keeping those that follow under control and in order. led his team to victory steer implies an ability to keep to a course and stresses the capacity of maneuvering correctly. \
steered the ship through a narrow channel pilot suggests guidance over a dangerous or complicated course. piloted the bill through the Senate engineer implies finding ways to avoid or overcome difficulties in achieving an end or \
carrying out a plan. engineered his son's election to the governorship";
	const char traveral[] = "Thisis for traveral test.hadshgasg fhauegasd �����İ����ᷢ����˹�ٸ�ɶ��gafgasdas da!!!";
	const char kooke[] = "\
		< !DOCTYPE html >\
		<html>\
		<head>\
		<meta charset = \"utf-8\">\
		<title>����̳�(runoob.com) < / title >\
		< / head>\
		<body>\
		<h1>�ҵĵ�һ������< / h1>\
		<p>�ҵĵ�һ�����䡣< / p>\
		< / body>\
		< / html>\
		";
	const char default_resp[] = "���ı�������ԣ�Ӣ�HyperText Markup Language����ƣ�HTML����һ�����ڴ�����ҳ�ı�׼������ԡ�\
		������ʹ�� HTML �������Լ��� WEB վ�㣬HTML ������������ϣ����������������\
		�ڱ��̳��У�����ѧϰ���ʹ�� HTML ������վ�㡣\
		HTML ������ѧϰ���������ܺܿ�ѧ������\
		";

	//std::atomic_int Count{0};
	server.OnRequest("GET /user", [user](std::string content, int session_id, unsigned error_code, OnRspCB callback) {
		StreamWriter::Instance()->Write(std::cout, "[test_httpServer] session id=%d, GET /user content:%s", session_id, content.c_str());
		callback(session_id, user);
		});
	server.OnRequest("GET /guide", [guide](std::string content, int session_id, unsigned error_code, OnRspCB callback) {
		StreamWriter::Instance()->Write(std::cout, "[test_httpServer] session id=%d, GET /user content:%s", session_id, content.c_str());
		callback(session_id, guide);
		});
	server.OnRequest("GET /traveral", [traveral](std::string content, int session_id, unsigned error_code, OnRspCB callback) {
		StreamWriter::Instance()->Write(std::cout, "[test_httpServer] session id=%d, GET /user content:%s", session_id, content.c_str());
		callback(session_id, traveral);
		});
	server.OnRequest("GET /kooke", [kooke](std::string content, int session_id, unsigned error_code, OnRspCB callback) {
		StreamWriter::Instance()->Write(std::cout, "[test_httpServer] session id=%d, GET /user content:%s", session_id, content.c_str());
		callback(session_id, kooke);
		});
	server.OnRequest("GET /default", [default_resp](std::string content, int session_id, unsigned error_code, OnRspCB callback) {
		StreamWriter::Instance()->Write(std::cout, "[test_httpServer] session id=%d, GET /user content:%s", session_id, content.c_str());
		callback(session_id, default_resp);
		});

	server.Start();

	StreamWriter::Instance()->Write(std::cout, "[test_httpServer] The main thread will sleep for 300s.");
	std::this_thread::sleep_for(std::chrono::seconds(60 * 5));
	server.Stop();
}

#include <fstream>

void test_httpServer_File() {
#ifdef OPENSSL
	HttpServer server("127.0.0.1", 443, ".");
#else
	HttpServer server("127.0.0.1", 80, ".");
#endif

	std::vector<std::thread*> v_workthd_;

	server.OnRequest("GET /file", [&server, &v_workthd_](std::string content, int session_id, unsigned error_code, OnRspCB callback) {
		StreamWriter::Instance()->Write(std::cout, "[test_httpServer] session id=%d, GET /file content:%s", session_id, content.c_str());
		callback(session_id, "");

		int file_pos{ 0 };
		int pos_s = content.find_first_of('=');
		if (pos_s != std::string::npos) {
			int pos_e = content.find_first_of(pos_s, '=');
			if (pos_e != std::string::npos) {
				std::string str_file_pos = content.substr(pos_s + 1, pos_e - pos_s - 1);
				file_pos = std::stoi(str_file_pos);
			}
		}

		std::thread* thd = new std::thread([session_id , file_pos, &server]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));			
			try
			{
				std::ifstream ifs("./vlc-3.0.1-win32.exe", std::ios::in | std::ios::binary);
				if (ifs) {
					// get length of file:
					ifs.seekg(0, ifs.end);
					int length = ifs.tellg();
					if(file_pos > 0)
						ifs.seekg(file_pos, ifs.beg);
					else
						ifs.seekg(0, ifs.beg);

					StreamWriter::Instance()->Write(std::cout, "[test_httpServer] File length:%d", length);

					constexpr int BUFFER_SIZE = 1024 * 48;
					std::string buffer(BUFFER_SIZE, '\0');
					int offset = 0;
					int readSize = std::min(BUFFER_SIZE, length - offset);
					int count = 0;
					while (readSize > 0 && ifs.read(&buffer[0], readSize)) {
						uint8_t ec = 0;
						bool b_end{ false };
						if (offset + readSize >= length)
							b_end = true;
						int ret = server.SendChunkFile(session_id, buffer.c_str(), readSize, b_end, ec);
						offset += readSize;
						readSize = std::min(BUFFER_SIZE, length - offset);
						StreamWriter::Instance()->Write(std::cout, "[test_httpServer] SessionID=%d, Send Byte=%d, count=%d, total send=%d", \
							session_id, ret, count, offset);
					}
				}
				if (ifs.is_open())
					ifs.close();
			}
			catch (const std::exception& e)
			{
				StreamWriter::Instance()->Write(std::cout, "[test_httpServer] send chunk file raise ex:%s", e.what());
			}			
		});
		v_workthd_.push_back(thd);
	});

	server.Start();

	StreamWriter::Instance()->Write(std::cout, "[test_httpServer] The main thread will sleep for 300s.");
	std::this_thread::sleep_for(std::chrono::seconds(60 * 5));

	for (auto itr = v_workthd_.begin(); itr != v_workthd_.end();) {
		if ((*itr)->joinable()) {
			(*itr)->join();
			std::thread* p_t = *itr;
			delete p_t;
			itr = v_workthd_.erase(itr);
		}
	}
	server.Stop();
}

int main(int argc, char* argv[]) {
	//test_httpServer();
	test_httpServer_File();

	return 0;
}