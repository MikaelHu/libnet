#pragma once
//========================================================================
//[File Name]:Server.h
//[Description]: the definitions of Tcp and Udp Servers.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <numeric>
#include "../asio/asio/buffer.hpp"
#include "../core/Session.h"
#include "StreamBase.h"
#include "StreamWriter.h"


using namespace asio;
using namespace asio::ip;

namespace Net {
	namespace Stream {

		class ServerTcp : public ServerBase {
		public:
			explicit ServerTcp(io_context& io_c, const std::string& ip, short port, \
				unsigned timeout = 5) : ServerBase(io_c, ip, port, timeout) { }

		protected:
			virtual void Accept() override {
				this->ServThdptr_ = std::make_shared<std::thread>([&] {
					while (!this->bExit_) {
						try
						{
							tcp::socket socket(this->ioCtx_);
							asio::error_code err_code;
							this->acceptor_.accept(socket, err_code);
							if (!err_code) {
								auto id = SessionIdGen<int>::Instance()->GetId();
								std::shared_ptr<SessionTcp> session = std::make_shared<SessionTcp>(id, this->ioCtx_, \
									std::move(socket), this->memStorage_, this->timeout_);
								//auto ret = session->Open("", -1); //todo
								session->SetCallBackError([this](int session_id, uint8_t ec) {
									StreamWriter::Instance()->Write(std::cout, "[Server] err code=%d", (int)ec);
									this->CloseSession(session_id);
									});
								session->SetStatus(1);
								tcp::socket::send_buffer_size sbs(1024 * 32);
								tcp::socket::receive_buffer_size rbs(1024 * 32);
								std::error_code ec;
								session->SetOption(sbs, ec);
								session->SetOption(rbs, ec);

								this->umSessions_.emplace(id, session);
								StreamWriter::Instance()->Write(std::cout, "[Server] Current connect count:%d", this->umSessions_.size());
								this->DoReceive(id);
							}
							else {
								StreamWriter::Instance()->Write(std::cout, "[Server] Accept err message=%s", err_code.message().c_str());
							}

							std::this_thread::sleep_for(std::chrono::milliseconds(5));
						}
						catch (const std::exception& e)
						{
							StreamWriter::Instance()->Write(std::cout, "[Server] Accept raise ex:%s", e.what());
							throw e; break; //todo							
						}
					}
				});
			}
			virtual void AsyncAccept() override {
				this->acceptor_.async_accept([this](const asio::error_code& ec, tcp::socket socket) {
					try
					{
						if (ec) {
							StreamWriter::Instance()->Write(std::cout, "[Server] async_accept err=%s", ec.message().c_str());
							this->HandleAcpErr(ec);
							this->AsyncAccept();
							return;
						}
						else {
							int id = SessionIdGen<int>::Instance()->GetId();
							std::shared_ptr<SessionTcp> session = \
								std::make_shared<SessionTcp>(id, this->ioCtx_, \
									std::move(socket), this->memStorage_, this->timeout_);
							//auto ret = session->Open("", -1);//todo
							session->SetStatus(1);
							auto self = std::dynamic_pointer_cast<ServerTcp>(NetBase::shared_from_this());
							session->SetCallBackError([this, self](int session_id, uint8_t ec) {
								StreamWriter::Instance()->Write(std::cout, "[Server] err code=%d", (int)ec);
								this->CloseSession(session_id);
								});
							tcp::socket::send_buffer_size sbs(1024 * 32);
							tcp::socket::receive_buffer_size rbs(1024 * 32);
							std::error_code ec;
							session->SetOption(sbs, ec);
							session->SetOption(rbs, ec);

							this->umSessions_.emplace(id, session);
							StreamWriter::Instance()->Write(std::cout, "[Server] Current connect count:%d", this->umSessions_.size());
							this->DoAsyncReceive(id);
							this->AsyncAccept();
						}
					}
					catch (const std::exception& e)
					{
						StreamWriter::Instance()->Write(std::cout, "[Server]Async recieve raise ex:%s", e.what());
						throw e;
					}
				});
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

		using CBReceiveUdp = std::function<void(std::string ip, short port, const char*, size_t, uint8_t)>;

		class ServerUdp : public std::enable_shared_from_this<ServerUdp> {
		public:
			explicit ServerUdp(io_context& io_c, const std::string& ip, short port, unsigned timeout = 5) :
				memStorage_({ 32, 64, 256, 512, 1024 }),
				ioCtx_(io_c), ip_(ip), port_(port), timeout_(timeout) { }

			void Start() {
				bAsyncAc_ = false;
				bExit_ = false;
				StreamWriter::Instance()->Write(std::cout, "[Server] Start listening!");

				session_ = std::make_shared<SessionUdp>(ioCtx_, memStorage_, timeout_);
				session_->Open(ip_, port_);
				session_->SetCallBackError([this](udp::endpoint endpoint, uint8_t ec) {
					StreamWriter::Instance()->Write(std::cout, "[Server] err code=%d", (int)ec);
					});

				DoReceive();
			}
			void StartA() {
				bAsyncAc_ = true;
				bExit_ = false;
				StreamWriter::Instance()->Write(std::cout, "[Server] StartA listening!");

				session_ = std::make_shared<SessionUdp>(ioCtx_, memStorage_, timeout_);
				session_->Open("127.0.0.1", port_);
				session_->SetCallBackError([this](udp::endpoint endpoint, uint8_t ec) {
					StreamWriter::Instance()->Write(std::cout, "[Server] err code=%d", (int)ec);
					});
				ServThdptr_ = std::make_shared<std::thread>([this] {
					DoAsyncReceive();
					});

				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			void Stop() {
				session_->Close();
				bExit_ = true;
				if (ServThdptr_->joinable()) {
					ServThdptr_->join();
				}
			}

			template<typename F>
			void SetReceiveCB(F f) {
				cbReceive_ = f;
			}

		protected:
			void DoReceive() {
				ServThdptr_ = std::make_shared<std::thread>([this] {
					while (!bExit_) {
						uint8_t err_code{ 0 };
						char* data{ nullptr };
						auto size = session_->Recieve(&data, err_code);
						if (size < 0) {
							if (err_code == 1 || err_code == 2) { //todo
								StreamWriter::Instance()->Write(std::cout, "[Server] Recieve with err=%d", (int)err_code);
								break;
							}
							else {
								//todo to process the packet error
								StreamWriter::Instance()->Write(std::cout, "[Server] Recieve with err=%d", (int)err_code);
							}
							if (this->cbReceive_)
								this->cbReceive_("", 0, "", 0, err_code);
						}
						else {
							static int num = 0;
							auto thd_id = std::this_thread::get_id();
							auto endpoint = session_->GetNetPeer();
							StreamWriter::Instance()->Write(std::cout, "[Server] thead id=%d, peer ip=%s, port=%d£¬packet count=%d, Packet:%s", \
								thd_id, endpoint.address().to_string().c_str(), endpoint.port(), num++, data);

							if (this->cbReceive_)
								this->cbReceive_(endpoint.address().to_string(), endpoint.port(), data, size, err_code);
						}
					}
					});
			}
			void DoAsyncReceive() {
				auto self = shared_from_this();
				session_->AsyncRecieve([self](const udp::endpoint& endpoint, Packet* packet, uint8_t ec) {
					static int num = 0;
					auto thd_id = std::this_thread::get_id();
					if (!ec) {
						std::string str_packet = std::string(packet->body(), packet->bodyLen());
						StreamWriter::Instance()->Write(std::cout, "[Server] thead id=%d, peer ip=%s, port=%d£¬packet count=%d, Packet:%s", \
							thd_id, endpoint.address().to_string().c_str(), endpoint.port(), num++, str_packet.c_str());

						if (self->cbReceive_)
							self->cbReceive_(endpoint.address().to_string(), endpoint.port(), packet->body(), packet->bodyLen(), ec);
					}
					else {
						if (self->cbReceive_)
							self->cbReceive_(endpoint.address().to_string(), endpoint.port(), "", 0, ec);
						StreamWriter::Instance()->Write(std::cout, "[Server] Async recieve with err =%d", (int)ec);
					}
					self->DoAsyncReceive();
					});
			}

		protected:
			bool bAsyncAc_{ false };
			bool bExit_{ false };
			std::string ip_;
			short port_{ 0 };
			io_context& ioCtx_;
			unsigned timeout_{ 0 };
			NMemoryStorage<char> memStorage_;
			std::shared_ptr<SessionUdp> session_{ nullptr };
			std::shared_ptr<std::thread> ServThdptr_{ nullptr };
			CBReceiveUdp cbReceive_{ nullptr };
		};
	}
}