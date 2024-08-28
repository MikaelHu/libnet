#pragma once
//========================================================================
//[File Name]:ConnectorSSL.h
//[Description]: The impelemation of tcp connector with the support of ssl, 
// which allow openning lots of connects to tcp-ssl servers.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../core/SessionSSL.h"
#include "StreamBase.h"


namespace Net {
	using namespace Core::SSL;

	namespace Stream {
		namespace SSL {

			class ConnectorTcpSSL : public ConnectorBase, CaSSL {
			public:
				explicit ConnectorTcpSSL(io_context& io_c, const std::string& ip, short port, const std::string& ca, unsigned timeout = 5) : \
					ConnectorBase(io_c, ip, port, timeout), CaSSL(ca) { }

				virtual int Open(const std::string& ip, short port, bool keep_live = false) override {
					int id = SessionIdGen<int>::Instance()->GetId();
					tcp::endpoint endpoint(address::from_string(ip_), port_);
					std::shared_ptr<SessionTcpSSL> session(new SessionTcpSSL(id, this->ioCtx_, \
						tcp::socket(this->ioCtx_, endpoint), sslCtx_, this->memStorage_, this->timeout_, keep_live));
					session->SetCallBackError([this](int session_id, uint8_t ec) {
						StreamWriter::Instance()->Write(std::cout, "[Connector ssl] err code=%d", (int)ec);
						this->CloseSession(session_id);
						});
					SetVerify(session->GetSocket());

					this->OnTimer(session->GetSocket());
					tcp::endpoint server_addr(tcp::endpoint(address::from_string(ip), port));
					asio::error_code ec;
					session->GetSocket().lowest_layer().connect(server_addr, ec);
					this->OffTimer();

					if (ec) {
						return -1;
					}
					else {
						dynamic_cast<SessionTcpSSL*>(session.get())->HandShake(ssl::stream_base::client, ec);
						if (!ec) {
							session->SetStatus(1);
							tcp::socket::send_buffer_size sbs(1024 * 32);
							tcp::socket::receive_buffer_size rbs(1024 * 32);
							std::error_code ec;
							session->SetOption(sbs, ec);
							session->SetOption(rbs, ec);
							StreamWriter::Instance()->Write(std::cout, "[Connector ssl] Connect ok! session_id=%d", id);

							std::lock_guard<std::mutex> lock(this->mtx_);
							this->umSessions_.emplace(id, session);
							return id;
						}
						return -2;
					}
				}
				virtual void OpenA(const std::string& ip, short port, \
					std::function<void(int, std::error_code)> func, bool keep_live = false) override {
					this->bAsyncAc_ = true;
					tcp::endpoint endpoint(address::from_string(ip_), port_);
					int id = SessionIdGen<int>::Instance()->GetId();
					std::shared_ptr<SessionTcpSSL> session(new SessionTcpSSL(id, this->ioCtx_, \
						tcp::socket(this->ioCtx_, endpoint), sslCtx_, this->memStorage_, this->timeout_, keep_live));
					session->SetCallBackError([this](int session_id, uint8_t ec) {
						StreamWriter::Instance()->Write(std::cout, "[Connector ssl] err code=%d", (int)ec);
						this->CloseSession(session_id);
						});
					SetVerify(session->GetSocket());
					{
						std::lock_guard<std::mutex> lock(this->mtx_);
						this->umSessions_.emplace(id, session);
					}

					tcp::endpoint server_addr(tcp::endpoint(address::from_string(ip), port));
					session->GetSocket().lowest_layer().async_connect(server_addr, [this, id, func](const asio::error_code& ec) {
						if (ec) {
							this->umSessions_.erase(id);
							if (func != nullptr)
								func(-1, ec);
						}
						else {
							dynamic_cast<SessionTcpSSL*>(this->umSessions_.at(id).get())->AsyncHandShake(ssl::stream_base::client, \
								[this, id, func](const std::error_code& err_code) {
									if (!err_code) {
										this->umSessions_.at(id)->SetStatus(1);
										tcp::socket::send_buffer_size sbs(1024 * 32);
										tcp::socket::receive_buffer_size rbs(1024 * 32);
										std::error_code ec;
										this->umSessions_.at(id)->SetOption(sbs, ec);
										this->umSessions_.at(id)->SetOption(rbs, ec);

										StreamWriter::Instance()->Write(std::cout, "[Connector ssl] Connect ok! session_id=%d", id);
										if (func != nullptr)
											func(id, err_code);
									}
									else {
										this->umSessions_.erase(id);
										if (func != nullptr)
											func(-2, err_code);
									}
								});
						}
						});
					std::this_thread::sleep_for(std::chrono::seconds(this->timeout_));
				}

			protected:
				virtual void ReConn(int session_id) override {
					auto find = this->umSessions_.find(session_id);
					if (find != this->umSessions_.end()) {
						find->second->GetSocket().lowest_layer().async_connect(find->second->GetNetPeer(), [this, session_id](const asio::error_code& ec) {
							if (ec) {
								this->HandleConnErr(session_id, ec);
							}
							else {
								auto find = this->umSessions_.find(session_id);
								if (find != this->umSessions_.end()) {
									find->second->SetStatus(1);
									StreamWriter::Instance()->Write(std::cout, "[Connector ssl] Reconnect ok!, session_id=%d", session_id);
								}
							}
						});
					}
				}

				virtual void OnTimer(SOCKET_TYPE& socket) override {
					if (this->timeout_ > 0) {
						this->timer_.expires_after(std::chrono::seconds(this->timeout_));
						this->timer_.async_wait(
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