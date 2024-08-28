#pragma once
//========================================================================
//[File Name]:SessionBase.h
//[Description]: the base class of socket session.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <unordered_map>

#include "../Comm.h"
#include "RWHandler.h"
#include "NMemoryStorage.h"
#include "StreamWriter.h"
#include "NonCopyable.h"

#ifdef OPENSSL
#include "../asio/asio/ssl.hpp"
#endif //OPENSSL

using namespace asio;

namespace Net {
	namespace Core {
		using Socket_TCP = tcp::socket;
		using Socket_UDP = udp::socket;

#ifndef OPENSSL
		typedef Socket_TCP SOCKET_TYPE;
#else
		using Socket_TCP_SSL = ssl::stream<tcp::socket>;
		typedef Socket_TCP_SSL SOCKET_TYPE;
#endif //OPENSSL

		template<typename SocketType>
		class SessionBase : public std::enable_shared_from_this<SessionBase<SocketType>> {
		public:
#ifndef OPENSSL
			explicit SessionBase(int sess_id, io_context& io_c, Socket_TCP&& socket, NMemoryStorage<char>& mem_storage, unsigned timeout, bool keeplive) :
				id_(sess_id), ioCtx_(io_c), socket_(std::move(socket)), timer_(io_c), memStorage_(mem_storage), timeout_(timeout), keeplive_(keeplive)
			{ }
#else
			explicit SessionBase(int sess_id, io_context& io_c, Socket_TCP&& socket, ssl::context& ssl_c, NMemoryStorage<char>& mem_storage, unsigned timeout, bool keeplive) :
				id_(sess_id), ioCtx_(io_c), socket_(std::move(socket), ssl_c), timer_(io_c), memStorage_(mem_storage), timeout_(timeout), keeplive_(keeplive)
			{ }
#endif //OPENSSL
			~SessionBase() { Release(); };

			int Open(const std::string& ip, short port, bool b_reuse = false) {
				endpointLocal_ = tcp::endpoint(asio::ip::make_address(ip), port);
				socket_.open(endpointLocal_.protocol());
				socket_.set_option(tcp::socket::reuse_address(b_reuse));
				socket_.bind(endpointLocal_);
				return 0;
			}
			int Close() {
				CloseSocket(); return 0;
			}
			template<typename SocketOption_T>
			void SetOption(const SocketOption_T& option, std::error_code& ec) {
				socket_.set_option(option, ec);
			}
			template<typename SocketOption_T>
			void GetOption(SocketOption_T& option, std::error_code& ec) {
				socket_.get_option(option, ec);
			}
			bool isClosed() { return !socket_.is_open(); }
			void SetId(int id) { id_ = id; }
			int GetId() const { return id_; }
			void SetStatus(int status) { State_ = status; }
			int GetStatus() { return State_; }
			bool GetKeeplive() { return keeplive_; }
			void SetKeeplive(bool keeplive) { keeplive_ = keeplive; }
			tcp::endpoint& GetNetPeer() { return endpointPeer_; }

			int Send(const char* data, unsigned len, uint8_t& err_code) {
				if (!State_)
					return -1;

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
					OnTimer();
					auto ret = rwHandler_->HandleWrite(packet, err_code);
					OffTimer();

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
				if (!State_) {
					err_code = 1;
					return -1;
				}

				OnTimer();
				Packet* packet = rwHandler_->HandleRead(err_code);
				OffTimer();
				if (packet) {
					*data = packet->data();
					return packet->bodyLen();
				}
				return -2;
			}
			int AsyncSend(const char* data, unsigned len, std::function<void(size_t, uint8_t)> func = nullptr) {
				if (!State_)
					return -1;
				else {
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
								func(-2, err_code);
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
						DoAsyncSend(func);//todo
					}
					return 0;
				}
			}
			int AsyncRecieve(CBAsyncRead cb) {
				if (!State_)
					return -1;
				rwHandler_->HandleAsyncRead(cb);
				return 0;
			}
			template<typename F>
			void SetCallBackError(F f) { cbErr_ = f; }
			SocketType& GetSocket() { return socket_; }

		protected:
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
			virtual RWHandlerTcp<SocketType>* CreateRWHandler() = 0;
			virtual void CloseSocket() = 0;
			void DoAsyncSend(std::function<void(size_t, uint8_t)> func) {
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
					rwHandler_->HandleAsyncWrite(packet, [self, func](std::size_t size, uint8_t ec) {
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
							func(size, ec);
						self->DoAsyncSend(func);
						});
				}
				else
					bAsyncW_ = false;
			}

			virtual void OnTimer() = 0;
			void OffTimer() {
				if (timeout_ > 0)
					timer_.cancel();
			}

		protected:
			int id_{ -1 };
			tcp::endpoint endpointLocal_;
			tcp::endpoint endpointPeer_;
			NMemoryStorage<char>& memStorage_;
			io_context& ioCtx_;
			SocketType	 socket_;
			std::shared_ptr<RWHandlerTcp<SocketType>> rwHandler_{ nullptr };
			int State_{ 0 };  //0: unconnected, 1: connected, ...
			bool keeplive_{ false };
			steady_timer timer_;
			unsigned timeout_{ 0 };
			CBErrTcp cbErr_{ nullptr }; //params:session id, error_code
			std::mutex mtx_;
			std::atomic_bool bAsyncW_{ false };
			std::list<Packet> sendQueue_;
		};
	}
}