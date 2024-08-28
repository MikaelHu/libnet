#pragma once
//========================================================================
//[File Name]:ServerSSL.h
//[Description]: the definitions of Tcp  Server with support of ssl and SaSSL.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <numeric>
#include "../asio/asio/buffer.hpp"
#include "../core/SessionSSL.h"
#include "StreamBase.h"
#include "StreamWriter.h"


namespace Net {
	using namespace Core::SSL;

	namespace Stream {
		namespace SSL {
			struct SaSSLInfo_t {
				char sa[64]{ 0 };
				char dh[64]{ 0 };
				char psswd[64]{ 0 };
			};
			class  SaSSL {
			public:
				explicit SaSSL(const SaSSLInfo_t& sa_info) : sslCtx_(ssl::context::sslv23) {
					try
					{
						memcpy_s(&saSSLInfo_, sizeof(saSSLInfo_), &sa_info, sizeof(sa_info));

						sslCtx_.set_options(
							ssl::context::default_workarounds
							| ssl::context::no_sslv2
							| ssl::context::single_dh_use);
						sslCtx_.set_password_callback(std::bind(&SaSSL::GetPassword, this));
						sslCtx_.use_certificate_chain_file(saSSLInfo_.sa);
						sslCtx_.use_private_key_file(saSSLInfo_.sa, ssl::context::pem);
						sslCtx_.use_tmp_dh_file(saSSLInfo_.dh);
					}
					catch (const std::exception& e)
					{
						StreamWriter::Instance()->Write(std::cout, "[SaSSL] raise ex=%s", e.what());
						throw e;
					}
				}

			protected:
				std::string GetPassword() const { return saSSLInfo_.psswd; }

			protected:
				ssl::context sslCtx_;
				SaSSLInfo_t saSSLInfo_{ 0 };
			};

			class ServerTcpSSL : public ServerBase, SaSSL{
			public:
				explicit ServerTcpSSL(io_context& io_c, const std::string& ip, short port, \
					const SaSSLInfo_t& sa_info, unsigned timeout = 5) :
					ServerBase(io_c, ip, port, timeout), SaSSL(sa_info) { }

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
									std::shared_ptr<SessionTcpSSL> session = std::make_shared<SessionTcpSSL>(id, this->ioCtx_, \
										std::move(socket), this->sslCtx_, this->memStorage_, this->timeout_);
									//auto ret = session->Open("", -1); //todo
									session->SetCallBackError([this](int session_id, uint8_t ec) {
										StreamWriter::Instance()->Write(std::cout, "[Server ssl] err code=%d", (int)ec);
										this->CloseSession(session_id);
										});
									session->SetStatus(1);
									tcp::socket::send_buffer_size sbs(1024 * 32);
									tcp::socket::receive_buffer_size rbs(1024 * 32);
									std::error_code ec;
									session->SetOption(sbs, ec);
									session->SetOption(rbs, ec);

									std::error_code err_code;
									session->HandShake(ssl::stream_base::server, err_code);
									if (!err_code) {
										this->umSessions_.emplace(id, session);
										StreamWriter::Instance()->Write(std::cout, "[Server ssl] Current connect count:%d", this->umSessions_.size());
										this->DoReceive(id);
									}
								}
								else {
									StreamWriter::Instance()->Write(std::cout, "[Server ssl] Accept err message=%s", err_code.message().c_str());
								}

								std::this_thread::sleep_for(std::chrono::milliseconds(5));
							}
							catch (const std::exception& e)
							{
								StreamWriter::Instance()->Write(std::cout, "[Server ssl] Accept raise ex:%s", e.what());
								throw e;//todo
								break;
							}
						}
						});
				}
				virtual void AsyncAccept() override {
					this->acceptor_.async_accept([this](const asio::error_code& ec, tcp::socket socket) {
						try
						{
							if (ec) {
								StreamWriter::Instance()->Write(std::cout, "[Server ssl] async_accept err=%s", ec.message().c_str());
								this->HandleAcpErr(ec);
								this->AsyncAccept();
								return;
							}
							else {
								int id = SessionIdGen<int>::Instance()->GetId();
								std::shared_ptr<SessionTcpSSL> session = \
									std::make_shared<SessionTcpSSL>(id, this->ioCtx_, \
										std::move(socket), sslCtx_, this->memStorage_, this->timeout_);
								//auto ret = session->Open("", -1);//todo
								session->SetStatus(1);
								tcp::socket::send_buffer_size sbs(1024 * 32);
								tcp::socket::receive_buffer_size rbs(1024 * 32);
								std::error_code ec;
								session->SetOption(sbs, ec);
								session->SetOption(rbs, ec);

								session->SetCallBackError([this](int session_id, uint8_t ec) {
									StreamWriter::Instance()->Write(std::cout, "[Server ssl] err code=%d", (int)ec);
									this->CloseSession(session_id);
									});

								session->AsyncHandShake(ssl::stream_base::server, [this, id, session](const std::error_code& err_code) {
									if (!err_code) {
										this->umSessions_.emplace(id, session);
										StreamWriter::Instance()->Write(std::cout, "[Server ssl] Current connect count:%d", this->umSessions_.size());
										this->DoAsyncReceive(id);
									}
									else
										StreamWriter::Instance()->Write(std::cout, "[Server ssl] handshake err=%s", err_code.message().c_str());
									});
								this->AsyncAccept();
							}
						}
						catch (const std::exception& e)
						{
							StreamWriter::Instance()->Write(std::cout, "[Server ssl]Async recieve raise ex:%s", e.what());
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
									socket.lowest_layer().cancel();
							});
					}
				}
			};
		}
	}
}