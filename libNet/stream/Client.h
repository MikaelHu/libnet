#pragma once
//========================================================================
//[File Name]:Client.h
//[Description]: The impelemation of tcp client.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../asio/asio.hpp"
#include "../core/Session.h"
#include "StreamBase.h"


using namespace asio;
using namespace asio::ip;

namespace Net {
	namespace Stream {
		class Client : public ClientBase<SOCKET_TYPE> {
		public:
			explicit Client(io_context& io_c, const std::string& ip, short port, unsigned timeout = 5) :
				ClientBase<SOCKET_TYPE>(io_c, ip, port, timeout)
			{ }

			virtual int Open(const std::string& ip, short port, bool keep_live = false) override {
				tcp::endpoint endpoint(address::from_string(ip_), port_);
				tcp::socket socket(this->ioCtx_, endpoint);
				this->OnTimer(socket);
				tcp::endpoint server_addr(tcp::endpoint(address::from_string(ip), port));
				asio::error_code ec;
				socket.connect(server_addr, ec);
				this->OffTimer();

				if (ec) {
					return -1;
				}
				else {
					int id = SessionIdGen<int>::Instance()->GetId();
					session_ = new SessionTcp(id, ioCtx_, \
						std::move(socket), memStorage_, timeout_, keep_live);
					session_->SetCallBackError([this](int session_id, uint8_t ec) {
						StreamWriter::Instance()->Write(std::cout, "[Client] err code=%d", (int)ec);
						this->CloseSession();
						});
					session_->SetStatus(1);
					StreamWriter::Instance()->Write(std::cout, "[Client] Connect ok! session_id=%d", id);

					return id;
				}
			}
			virtual void OpenA(const std::string& ip, short port, \
				std::function<void(int, std::error_code)> func, bool keep_live = false) override {
				tcp::endpoint endpoint(address::from_string(ip_), port_);
				tcp::socket socket(this->ioCtx_, endpoint);
				tcp::endpoint server_addr(tcp::endpoint(address::from_string(ip), port));
				socket.async_connect(server_addr, [this, &socket, func, keep_live](const asio::error_code& ec) {
					if (ec) {
						if (func != nullptr)
							func(-1, ec);
					}
					else {
						int id = SessionIdGen<int>::Instance()->GetId();
						session_ = new SessionTcp(id, ioCtx_, \
							std::move(socket), memStorage_, timeout_, keep_live);
						session_->SetCallBackError([this](int session_id, uint8_t ec) {
							StreamWriter::Instance()->Write(std::cout, "[Client] err code=%d", (int)ec);
							this->CloseSession();
							});
						session_->SetStatus(1);
						StreamWriter::Instance()->Write(std::cout, "[Client] Connect ok! session_id=%d", id);

						if (func != nullptr)
							func(id, ec);
					}
				});
			}

			
		protected:
			virtual void ReConn() override {
				session_->GetSocket().async_connect(session_->GetNetPeer(), [this](const asio::error_code& ec) {
					if (ec) {
						this->HandleConnErr(ec);
					}
					else {
							session_->SetStatus(1);
							StreamWriter::Instance()->Write(std::cout, "[Client] Reconnect ok!");
					}
				});
			}
			virtual void OnTimer(SOCKET_TYPE& socket) override {
				if (timeout_ > 0) {
					timer_.expires_after(std::chrono::seconds(this->timeout_));
					timer_.async_wait(
						[this, &socket](const std::error_code& ec)
						{
							if (ec != asio::error::operation_aborted)
								socket.cancel();
						});
				}
			}
		};
	}
}