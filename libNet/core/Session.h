#pragma once
//========================================================================
//[File Name]:Session.h
//[Description]: a socket session definition.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "SessionBase.h"

namespace Net {
	namespace Core {
		class SessionTcp : public SessionBase<SOCKET_TYPE> {
		public:
			explicit SessionTcp(int sess_id, io_context& io_c, Socket_TCP&& socket, NMemoryStorage<char>& mem_storage, unsigned timeout, bool keeplive = false) :
				SessionBase(sess_id, io_c, std::move(socket), mem_storage, timeout, keeplive)
			{ Init(); }

		protected:
			void Init() {
				rwHandler_ = std::shared_ptr<RWHandlerTcp<SOCKET_TYPE>>(CreateRWHandler());
			}
			virtual RWHandlerTcp<SOCKET_TYPE>* CreateRWHandler() override {
#ifdef PACKET_STICK
				RWHandlerTcp<SOCKET_TYPE>* handler = new RWHandlerTcpS<SOCKET_TYPE>(ioCtx_, socket_, memStorage_);
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
						socket_.shutdown(tcp::socket::shutdown_send, ec);

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
						socket_.shutdown(tcp::socket::shutdown_receive, ec);
					}

					if (cbErr_)
						cbErr_(id_, ec);
					});
				return handler;
			}
			virtual void CloseSocket() override {
				asio::error_code ec;
				socket_.shutdown(tcp::socket::shutdown_both, ec);
				socket_.close();
			}
			virtual void OnTimer() override {
				if (timeout_ > 0) {
					auto self = this->shared_from_this();
					timer_.expires_after(std::chrono::seconds(timeout_));
					timer_.async_wait(
						[this, self](const std::error_code& ec)
						{
							if (ec != asio::error::operation_aborted)
								this->socket_.cancel();
						});
				}				
			}
		};

		class SessionUdp : public std::enable_shared_from_this<SessionUdp> {
		public:
			explicit SessionUdp(io_context& io_c, NMemoryStorage<char>& mem_storage, unsigned timeout) :
				ioCtx_(io_c), socket_(io_c/*, udp::endpoint(udp::v4(), 9900)*/), timer_(io_c), memStorage_(mem_storage), timeout_(timeout)
			{
				Init();
			}
			~SessionUdp() { Release(); };

			int Open(const std::string& ip, short port, bool b_reuse = false) {
				endpointLocal_ = udp::endpoint(asio::ip::make_address(ip), port);
				socket_.open(endpointLocal_.protocol());
				socket_.set_option(udp::socket::reuse_address(b_reuse));
				socket_.bind(endpointLocal_);

				return 0;
			}
			int Close() { CloseSocket(); return 0; }
			bool isClosed() { return !socket_.is_open(); }

			template<typename SocketOption_T>
			void SetOption(const SocketOption_T& option, std::error_code& ec) {
				socket_.set_option(option, ec);
			}
			template<typename SocketOption_T>
			void GetOption(SocketOption_T& option, std::error_code& ec) {
				socket_.get_option(option, ec);
			}

			udp::endpoint& GetNetPeer() { return endpointPeer_; }
			int Send(const udp::endpoint& endpoint, const char* data, unsigned len, uint8_t& err_code) {
				Buffer buff;
				if (len > Packet::e_Max_BodyLen)
					buff = Buffer(memStorage_.Alloc(MaxPacketSize), MaxPacketSize);
				else
					buff = Buffer(memStorage_.Alloc(len + Packet::e_HeadLen), len + Packet::e_HeadLen);

				Packet packet(buff);
				int nret{ 0 };
				while (len > 0) {
					packet.clear();
					const unsigned send_size = len > Packet::e_Max_BodyLen ? (MaxPacketSize - Packet::e_HeadLen) : len;
					if (packet.encodeData(data, send_size, err_code) < 0) {
						assert(packet.data());
						auto ret = memStorage_.Free(packet.data());
						if (ret < 0)
							assert(0);
						return -2;//todo return nret?
					}
					if (timeout_ > 0) {
						timer_.expires_after(std::chrono::seconds(timeout_));
						timer_.async_wait(
							[this](const std::error_code& ec)
							{
								if (ec != asio::error::operation_aborted)
									socket_.cancel();
							});
					}
					auto ret = rwHandler_->HandleWrite(endpoint, packet, err_code);
					if (timeout_ > 0)
						timer_.cancel();
					if (ret > 0) {
						nret += (ret - Packet::e_HeadLen);
						len -= (ret - Packet::e_HeadLen);
						data += (ret - Packet::e_HeadLen);
					}
					else {//todo return ret?
						assert(packet.data());
						auto ret1 = memStorage_.Free(packet.data());
						if (ret1 < 0)
							assert(0);
						return nret;
					}
				}
				assert(packet.data());
				auto ret1 = memStorage_.Free(packet.data());
				if (ret1 < 0)
					assert(0);
				return nret;
			}
			int Recieve(char** data, uint8_t& err_code) {
				if (timeout_ > 0) {
					timer_.expires_after(std::chrono::seconds(timeout_));
					timer_.async_wait(
						[this](const std::error_code& ec)
						{
							if (ec != asio::error::operation_aborted)
								socket_.cancel();
						});
				}
				Packet* packet = rwHandler_->HandleRead(endpointPeer_, err_code);
				if (timeout_ > 0)
					timer_.cancel();
				if (packet) {
					*data = packet->body();
					return packet->bodyLen();
				}
				return -2;
			}
			int AsyncSend(const udp::endpoint& endpoint, const char* data, unsigned len, std::function<void(const udp::endpoint&, size_t, uint8_t)> func = nullptr) {
				uint8_t err_code{ 0 };
				while (len > 0) {
					const unsigned send_size = len > Packet::e_Max_BodyLen ? (MaxPacketSize - Packet::e_HeadLen) : len;
					Buffer buff(memStorage_.Alloc(send_size + Packet::e_HeadLen), send_size + Packet::e_HeadLen);
					Packet packet(buff);
					if (packet.encodeData(data, send_size, err_code) < 0) {
						assert(packet.data());
						auto ret = memStorage_.Free(packet.data());
						if (ret < 0)
							assert(0);
						if (func)
							func(endpointPeer_, -2, err_code);
					}
					{
						std::lock_guard<std::mutex> lock(mtx_);
						sendQueue_.emplace_back(packet);//todo
					}
					len -= send_size;
					data += send_size;
				}

				if (!bAsyncW_) {
					bAsyncW_ = true;
					DoAsyncSend(endpoint, func);//todo
				}
				return 0;
			}
			int AsyncRecieve(CBAsyncReadUdp cb) {
				rwHandler_->HandleAsyncRead(endpointPeer_, cb);
				return 0;
			}
			template<typename F>
			void SetCallBackError(F f) { cbErr_ = f; }
			udp::socket& GetSocket() { return socket_; }

		protected:
			void Init() {
				rwHandler_ = std::shared_ptr<RWHandlerUdp>(CreateRWHandler());
			}
			void Release() {
				{
					std::lock_guard<std::mutex> lock(this->mtx_);
					if (!sendQueue_.empty()) {
						for (auto& var : sendQueue_) {
							if (var.data())
								memStorage_.Free(var.data());
						}
						sendQueue_.clear();
					}
				}
			}
			RWHandlerUdp* CreateRWHandler() {
				RWHandlerUdp* handler = new RWHandlerUdp(ioCtx_, socket_, memStorage_);

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
						socket_.shutdown(tcp::socket::shutdown_send, ec);

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
						socket_.shutdown(tcp::socket::shutdown_receive, ec);
					}

					if (cbErr_)
						cbErr_(endpointPeer_, ec);
					});
				return handler;
			}
			void CloseSocket() {
				asio::error_code ec;
				socket_.shutdown(tcp::socket::shutdown_both, ec);
				socket_.close();
			}
			void DoAsyncSend(const udp::endpoint& endpoint, std::function<void(const udp::endpoint&, size_t, uint8_t)> func) {
				Packet packet;
				bool b_get{ false };
				{
					std::lock_guard<std::mutex> lock(mtx_);
					if (!sendQueue_.empty()) {
						packet = sendQueue_.front();
						b_get = true;
					}
				}

				if (b_get) {
					auto self = this->shared_from_this();
					rwHandler_->HandleAsyncWrite(endpoint, packet, [self, func](const udp::endpoint& endpoint, std::size_t size, uint8_t ec) {
						Packet packet;
						{
							std::lock_guard<std::mutex> lock(self->mtx_);
							if (!self->sendQueue_.empty()) {
								packet = self->sendQueue_.front();
								self->sendQueue_.pop_front();
							}
						}
						if (packet.data()) {
							self->memStorage_.Free(packet.data());
						}
						if (func)
							func(endpoint, size, ec);
						self->DoAsyncSend(endpoint, func);
						});
				}
				else
					bAsyncW_ = false;
			}

		protected:
			io_context& ioCtx_;
			Socket_UDP socket_;
			udp::endpoint endpointLocal_;
			udp::endpoint endpointPeer_;
			NMemoryStorage<char>& memStorage_;
			std::shared_ptr<RWHandlerUdp> rwHandler_{ nullptr };
			steady_timer timer_;
			unsigned timeout_{ 0 };
			CBErrUdp cbErr_{ nullptr }; //params:session id, error_code
			std::mutex mtx_;
			std::atomic_bool bAsyncW_{ false };
			std::list<Packet> sendQueue_;
		};
	}
}