#pragma once
//========================================================================
//[File Name]:Connector.h
//[Description]: The impelemation of tcp connector, which allow openning lots 
// of connects to tcp servers.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../core/Session.h"
#include "StreamBase.h"


using namespace asio;
using namespace asio::ip;

namespace Net {
	namespace Stream {

		class ConnectorTcp : public ConnectorBase {
		public:
			explicit ConnectorTcp(io_context& io_c, const std::string& ip, short port, unsigned timeout = 5) : 
				ConnectorBase(io_c, ip, port, timeout) { }

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
					std::shared_ptr<SessionTcp> session = std::make_shared<SessionTcp>(id, this->ioCtx_, \
						std::move(socket), this->memStorage_, this->timeout_, keep_live);
					session->SetCallBackError([this](int session_id, uint8_t ec) {
						StreamWriter::Instance()->Write(std::cout, "[Connector] err code=%d", (int)ec);
						this->CloseSession(session_id);
						});
					session->SetStatus(1);
					tcp::socket::send_buffer_size sbs(1024 * 32);
					tcp::socket::receive_buffer_size rbs(1024 * 32);
					std::error_code ec;
					session->SetOption(sbs, ec);
					session->SetOption(rbs, ec);

					StreamWriter::Instance()->Write(std::cout, "[Connector] Connect ok! session_id=%d", id);

					std::lock_guard<std::mutex> lock(this->mtx_);
					this->umSessions_.emplace(id, session);
					return id;
				}
			}
			virtual void OpenA(const std::string& ip, short port, \
				std::function<void(int, std::error_code)> func, bool keep_live = false) override {
				this->bAsyncAc_ = true;
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
						std::shared_ptr<SessionTcp> session = std::make_shared<SessionTcp>(id, this->ioCtx_, \
							std::move(socket), this->memStorage_, this->timeout_, keep_live);
						session->SetCallBackError([this](int session_id, uint8_t ec) {
							StreamWriter::Instance()->Write(std::cout, "[Connector] err code=%d", (int)ec);
							this->CloseSession(session_id);
							});

						{
							std::lock_guard<std::mutex> lock(this->mtx_);
							this->umSessions_.emplace(id, session);
						}
						session->SetStatus(1);
						tcp::socket::send_buffer_size sbs(1024 * 32);
						tcp::socket::receive_buffer_size rbs(1024 * 32);
						std::error_code ec;
						session->SetOption(sbs, ec);
						session->SetOption(rbs, ec);

						StreamWriter::Instance()->Write(std::cout, "[Connector] Connect ok! session_id=%d", id);

						if (func != nullptr)
							func(id, ec);
					}
				});
			}

		protected:
			virtual void ReConn(int session_id) override {
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end()) {
					find->second->GetSocket().async_connect(find->second->GetNetPeer(), [this, session_id](const asio::error_code& ec) {
						if (ec) {
							this->HandleConnErr(session_id, ec);
						}
						else {
							auto find = this->umSessions_.find(session_id);
							if (find != this->umSessions_.end()) {
								find->second->SetStatus(1);
								StreamWriter::Instance()->Write(std::cout, "[Connector] Reconnect ok!, session_id=%d", session_id);
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
								socket.cancel();
						});
				}
			}
		};

		class ConnectorUdp : public std::enable_shared_from_this<ConnectorUdp> {
		public:
			explicit ConnectorUdp(io_context& io_c, const std::string& ip, short port, unsigned timeout = 5) :
				memStorage_({ 32, 64, 256, 512, 1024 }),
				ioCtx_(io_c), ip_(ip), port_(port), timeout_(timeout) { }

			int Open() {
				StreamWriter::Instance()->Write(std::cout, "[Connector] open connector port=%d!", port_);
				session_ = std::make_shared<SessionUdp>(ioCtx_, memStorage_, timeout_);
				session_->Open(ip_, port_);
				session_->SetCallBackError([this](udp::endpoint endpoint, uint8_t ec) {
					StreamWriter::Instance()->Write(std::cout, "[Connector] err code=%d", (int)ec);
					});
				return 0;
			}
			int Close() {
				session_->Close();
				return 0;
			}
			int Send(const std::string& ip, short port, const char* data, unsigned len, uint8_t& ec) {
				return session_->Send(udp::endpoint(ip::address_v4::from_string(ip), port), data, len, ec);
			}
			int AsyncSend(const std::string& ip, short port, const char* data, unsigned len, CBAsyncSendUdp cb = nullptr) {
				return session_->AsyncSend(udp::endpoint(ip::address_v4::from_string(ip), port), data, len, cb);
			}
			int Recieve(char** data, uint8_t& ec) {
				return session_->Recieve(data, ec);
			}
			int AsyncRecieve(CBAsyncRecvUdp cb = nullptr) {
				return session_->AsyncRecieve([cb](const udp::endpoint& endpoint, Packet* packet, uint8_t ec) {
					if (packet == nullptr) {
						StreamWriter::Instance()->Write(std::cout, "[Connector] Async recieve err, code=%d", (int)ec);
						if (cb)
							cb(endpoint, nullptr, 0, ec);
					}
					else {
						if (cb)
							cb(endpoint, packet->body(), packet->bodyLen(), ec);
					}
					});
			}

		protected:
			void HandleRWErr(const udp::endpoint& endpoint, uint8_t ec) {
				StreamWriter::Instance()->Write(std::cout, "[Connector] RW err, ip=%s, port=%d, ec=%d", \
					endpoint.address().to_string().c_str(), endpoint.port(), (int)ec);
			}

		protected:
			std::string ip_;
			short port_{ 0 };
			io_context& ioCtx_;
			unsigned timeout_{ 0 };
			NMemoryStorage<char> memStorage_;
			std::shared_ptr<SessionUdp> session_{ nullptr };
		};
	}
}