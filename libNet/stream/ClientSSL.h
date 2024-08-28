#pragma once
//========================================================================
//[File Name]:ClientSSL.h
//[Description]: The impelemation of tcp client with support of ssl.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../asio/asio.hpp"
#include "../core/SessionSSL.h"
#include "StreamBase.h"


using namespace asio;
using namespace asio::ip;

namespace Net {
	using namespace Core::SSL;

	namespace Stream {
		namespace SSL {
			class ClientSSL : public ClientBase<SOCKET_TYPE>, CaSSL {
			public:
				explicit ClientSSL(io_context& io_c, const std::string& ip, short port, const std::string& ca, unsigned timeout = 5) :
					ClientBase<SOCKET_TYPE>(io_c, ip, port, timeout), CaSSL(ca){ }

				virtual int Open(const std::string& ip, short port, bool keep_live = false) override {
					tcp::endpoint endpoint(address::from_string(ip_), port_);
					int id = SessionIdGen<int>::Instance()->GetId();
					session_ = new SessionTcpSSL(id, ioCtx_, tcp::socket(this->ioCtx_, endpoint), \
						sslCtx_, memStorage_, timeout_, keep_live);
					session_->SetCallBackError([this](int session_id, uint8_t ec) {
						StreamWriter::Instance()->Write(std::cout, "[Client ssl] err code=%d", (int)ec);
						this->CloseSession();
						});
					this->OnTimer(session_->GetSocket());
					tcp::endpoint server_addr(tcp::endpoint(address::from_string(ip), port));
					asio::error_code ec;
					session_->GetSocket().lowest_layer().connect(server_addr, ec);
					this->OffTimer();

					if (ec) {
						return -1;
					}
					else {
						dynamic_cast<SessionTcpSSL*>(session_)->HandShake(ssl::stream_base::client, ec);
						session_->SetStatus(1);
						StreamWriter::Instance()->Write(std::cout, "[Client ssl] Connect ok! session_id=%d", id);
						return id;
					}
				}
				virtual void OpenA(const std::string& ip, short port, \
					std::function<void(int, std::error_code)> func, bool keep_live = false) override {
					tcp::endpoint endpoint(address::from_string(ip_), port_);
					int id = SessionIdGen<int>::Instance()->GetId();
					session_ = new SessionTcpSSL(id, ioCtx_, tcp::socket(ioCtx_, endpoint), \
						sslCtx_, memStorage_, timeout_, keep_live);
					session_->SetCallBackError([this](int session_id, uint8_t ec) {
						StreamWriter::Instance()->Write(std::cout, "[Client ssl] err code=%d", (int)ec);
						this->CloseSession();
						});
					tcp::endpoint server_addr(tcp::endpoint(address::from_string(ip), port));
					session_->GetSocket().lowest_layer().async_connect(server_addr, [this, id, func, keep_live](const asio::error_code& ec) {
						if (ec) {
							if (func != nullptr)
								func(-1, ec);
						}
						else {
							dynamic_cast<SessionTcpSSL*>(session_)->AsyncHandShake(ssl::stream_base::client, \
								[this, id, func](const std::error_code& err_code) {
									if (!err_code) {
										session_->SetStatus(1);
										StreamWriter::Instance()->Write(std::cout, "[Client ssl] Connect ok! session_id=%d", id);

										if (func != nullptr)
											func(id, err_code);
									}
									else {
										if (func != nullptr)
											func(-2, err_code);
									}
								});
						}
					});
				}

			protected:
				virtual void ReConn() override {
					session_->GetSocket().lowest_layer().async_connect(session_->GetNetPeer(), [this](const asio::error_code& ec) {
						if (ec) {
							this->HandleConnErr(ec);
						}
						else {
							session_->SetStatus(1);
							StreamWriter::Instance()->Write(std::cout, "[Client ssl] Reconnect ok!");
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
									socket.lowest_layer().cancel();
							});
					}
				}
			};
		}
	}
}