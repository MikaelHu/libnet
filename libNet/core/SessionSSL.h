#pragma once
//========================================================================
//[File Name]:SessionSSL.h
//[Description]: the socket session class with support of ssl.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../asio/asio/ssl.hpp"
#include "SessionBase.h"


namespace Net {
	namespace Core {
		namespace SSL {

			class SessionTcpSSL : public SessionBase<SOCKET_TYPE> {
			public:
				explicit SessionTcpSSL(int sess_id, io_context& io_c, Socket_TCP&& socket, ssl::context& ssl_c, NMemoryStorage<char>& mem_storage, unsigned timeout, bool keeplive = false) :
					SessionBase(sess_id, io_c, std::move(socket), ssl_c, mem_storage, timeout, keeplive)
				{
					Init();
				}

				void HandShake(ssl::stream_base::handshake_type type, std::error_code& err_code) {
					socket_.handshake(type, err_code);
				}
				template<typename F>
				void AsyncHandShake(ssl::stream_base::handshake_type type, F f)
				{
					auto self = this->shared_from_this();
					socket_.async_handshake(type, [this, self, f](const std::error_code& err_code) { f(err_code); });
				}

			protected:
				void Init() {
					rwHandler_ = std::shared_ptr<RWHandlerTcp<SOCKET_TYPE>>(CreateRWHandler());
				}
				virtual RWHandlerTcp<SOCKET_TYPE>* CreateRWHandler() override {
#ifdef PACKET_STICK
					RWHandler_Tcp<SOCKET_TYPE>* handler = new RWHandlerTcpS<SOCKET_TYPE>(ioCtx_, socket_, memStorage_);
#else
					RWHandlerTcp<SOCKET_TYPE>* handler = new RWHandlerTcpNS<SOCKET_TYPE>(ioCtx_, socket_, memStorage_);
#endif //PACKET_STICK
					handler->SetCallBackError([this](E_ERR_T err_t, uint8_t ec) {
						switch (ec) {
						case NET_EOF:
							StreamWriter::Instance()->Write(std::cout, "[Session] Net err, eof!");
							break;
						case NET_OTHER:
							StreamWriter::Instance()->Write(std::cout, "[Session] Net err, other!");
							break;
						case NET_BAD_HEAD:
							StreamWriter::Instance()->Write(std::cout, "[Session] Data err, bad head!");
							break;
						case NET_BAD_BODY:
							StreamWriter::Instance()->Write(std::cout, "[Session] Data err, bad body!");
							break;
						case NO_ERR:
							StreamWriter::Instance()->Write(std::cout, "[Session] Non err!");
							break;
						default:
							break;
						}
						if (err_t == E_ERR_T::e_Err_W) {
							asio::error_code ec;
							socket_.lowest_layer().shutdown(tcp::socket::shutdown_send, ec);

							std::lock_guard<std::mutex> lock(this->mtx_);
							for (auto& var : sendQueue_) {
								if (var.data()) {
									memStorage_.Free(var.data());
								}
							}
							sendQueue_.clear();
						}
						else if (err_t == E_ERR_T::e_Err_R) {
							asio::error_code ec;
							socket_.lowest_layer().shutdown(tcp::socket::shutdown_receive, ec);
						}

						if (cbErr_)
							cbErr_(id_, ec);
						});
					return handler;
				}

				virtual void CloseSocket() override {
					asio::error_code ec;
					socket_.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
					socket_.lowest_layer().close();
				}
				virtual void OnTimer() override {
					if (timeout_ > 0) {
						auto self = this->shared_from_this();
						timer_.expires_after(std::chrono::seconds(timeout_));
						timer_.async_wait(
							[this, self](const std::error_code& ec)
							{
								if (ec != asio::error::operation_aborted)
									this->socket_.lowest_layer().cancel();
							});
					}
				}
			};
		}
	}
}