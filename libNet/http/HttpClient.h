#pragma once
//========================================================================
//[File Name]:HttpClient.h
//[Description]: the implemetation of http client.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "asio.hpp"
#include <string>

#ifndef OPENSSL
#include "../stream/Connector.h"
#else
#include "../stream/ConnectorSSL.h"
using namespace Net::Stream::SSL;
#endif //OPENSSL

#include "../Comm.h"
#include "../com/ObjectPool.h"
#include "StreamWriter.h"
#include "HttpResponseHandler.h"
#include "HttpConnectionMng.h"


using namespace asio;
using namespace asio::ip;
using namespace Net;
using namespace Net::Stream;


namespace Http {

	class HttpClient {
	public:
		explicit HttpClient(bool keep_live = true) : ioCtx_(), worker_(ioCtx_), \
			keepLive_(keep_live){
			for (auto i = 0; i < workThdNum_; ++i) {
				v_workThd_.emplace_back(std::thread([&]() {
					ioCtx_.run();
					}));
			}
#ifdef OPENSSL
			std::string ca = "ca.pem";
			connector_ = new ConnectorTcpSSL(ioCtx_, "127.0.0.1", 0, ca);
#else
			connector_ = new ConnectorTcp(ioCtx_, "127.0.0.1", 0);
#endif
			assert(connector_);
			SendReqMng_ = new HttpSendRequestMng(connector_);
			assert(SendReqMng_);
			connMng_ = new HttpConnectionMng(ioCtx_);
			assert(connMng_);
			Init();
		}
		~HttpClient() {
			connMng_->Stop();
			ioCtx_.stop();

			for (auto& var : v_workThd_) {
				if (var.joinable())
					var.join();
			}
			SAFE_DELETE(connector_);
			SAFE_DELETE(SendReqMng_);
			SAFE_DELETE(connMng_);

			for (auto itr = um_CkFileMng_.begin(); itr != um_CkFileMng_.end();) {
				if (itr->second) {
					SAFE_DELETE(itr->second);
					itr = um_CkFileMng_.erase(itr);
				}
				else
					itr++;
			}
		}

		int Open(const char* ip, short port) {
			serverIp_ = ip;
			ServerPort_ = port;
			int id = connector_->Open(serverIp_, ServerPort_);
			return id;
		}
		void Close(int id) {
			connector_->Close(id);
		}
		//req:"GET /lx" 
		void SendReq(int id, const char* req, unsigned len, OnSendReqCB callback) {
			if (connector_->IsConnected(id) != 1)
				callback("", 1); // todo error code 1 for unconnected.
			SendReqMng_->SendReq(id, req, len, keepLive_, callback);
		}
		void AsyncSendReq(int id, const char* req, unsigned len, OnSendReqCB callback) {
			if (connector_->IsConnected(id) != 1)
				callback("", 1);			
			SendReqMng_->AsyncSendReq(id, req, len, keepLive_, callback);
		}
		int SendChunkFile(int id, const char* chunk, unsigned len, bool b_end, uint8_t& ec) {
			auto find = um_CkFileMng_.find(id);
			if (find != um_CkFileMng_.end()) {
				int ret = find->second->SendChunkFile(id, chunk, len, b_end, ec);
				if (b_end) {
					SAFE_DELETE(find->second);
					um_CkFileMng_.erase(find);
				}
				return ret;
			}
			else {
				HttpChunkFileMng* ck_file_mng = new HttpChunkFileMng(connector_);
				assert(ck_file_mng);
				int ret = ck_file_mng->SendChunkFile(id, chunk, len, b_end, ec);
				if (b_end)
					SAFE_DELETE(ck_file_mng);
				um_CkFileMng_.emplace(id, ck_file_mng);
				return ret;
			}
		}
		//todo: usage: to receive the chunk file (entitybody and trailer)
		template<typename F>  //void(char*, int) param: data, len
		int RecvChunkFile(int id, F callback, bool b_trailer, uint8_t& ec) {
			if (connector_->IsConnected(id) != 1)
				return -1;
			auto find = um_CkFileMng_.find(id);
			if (find != um_CkFileMng_.end()) {
				int ret = find->second->RecvChunkFile(id, callback, b_trailer, ec);
				SAFE_DELETE(find->second);
				um_CkFileMng_.erase(find);
				return ret;
			}
			else {
				HttpChunkFileMng* ck_file_mng = new HttpChunkFileMng(connector_);
				assert(ck_file_mng);
				int ret = ck_file_mng->RecvChunkFile(id, callback, b_trailer, ec);
				SAFE_DELETE(find->second);
				um_CkFileMng_.erase(find);
				return ret;
			}
		}

	protected:
		void Init() {
			connMng_->Run();
			CreateConnStrategy();

			connector_->SetEventCB([this](int id, const Event& event) {
				switch (event.type) {
				case Event::eEvent_t::e_connClosed:
				case Event::eEvent_t::e_netEof:
				case Event::eEvent_t::e_netErr:
				{
					UpdateConn(id, ConnState::e_Closed);
					break;
				}
				default:
					break;
				}
				});
		}
		void CreateConnStrategy() {
			connMng_->AddStrategy("IdleConn", [this](HttpConnectionMng::UM_ConnState& um_conn_state) {
				for (auto iter = um_conn_state.begin(); iter != um_conn_state.end();) {
					if (iter->second == ConnState::e_Idle) {
						this->Close(iter->first);
						iter = um_conn_state.erase(iter);
					}
					else if (iter->second == ConnState::e_Closed)
						iter = um_conn_state.erase(iter);
					else
						iter++;
				}
				}, true);
		}
		
		void UpdateConn(int id, ConnState state) {
			connMng_->UpdateConn(id, state);
		}

	protected:
		io_context ioCtx_;
		io_context::work worker_;
		unsigned workThdNum_{ 1 };
		std::vector<std::thread> v_workThd_;
		bool keepLive_{ false };
		std::string serverIp_;
		short ServerPort_{ 0 };
		ConnectorBase* connector_{ nullptr };
		HttpSendRequestMng* SendReqMng_{ nullptr };
		HttpConnectionMng* connMng_{ nullptr };
		std::unordered_map<int, HttpChunkFileMng*> um_CkFileMng_;
	};
}