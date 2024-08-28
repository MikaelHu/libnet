#pragma once
//========================================================================
//[File Name]:RWHandler.h
//[Description]: an implemetaion of read-write handler for socket.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <functional>
#include <iostream>

#include "../asio/asio.hpp"
#include "Packet.h"
#include "StreamWriter.h"
#include "NMemoryStorage.h"

using namespace asio;
using namespace asio::ip;

namespace Net {
	namespace Core {
		enum class E_ERR_T {
			e_Err_R = 0,
			e_Err_W
		};

		typedef std::function<void(std::size_t, uint8_t)> CBAsyncWrite;
		typedef std::function<void(Packet*, uint8_t)>  CBAsyncRead;
		typedef std::function<void(int, uint8_t)> CBErrTcp;
		typedef std::function<void(udp::endpoint endpoint, uint8_t)> CBErrUdp;
		typedef std::function<void(E_ERR_T, uint8_t)> CBErrRW;

		template<typename SocketType>
		class RWHandlerTcp : public std::enable_shared_from_this<RWHandlerTcp<SocketType>> {
		public:
			explicit RWHandlerTcp(io_context& io_c, SocketType& socket, NMemoryStorage<char>& mem_storage) :
				socket_(socket), memStorage_(mem_storage), readPacket_(), readHPacket_() { }
			virtual ~RWHandlerTcp() {
				if (readHPacket_.data()) {
					memStorage_.Free(readHPacket_.data());
				}
				if (readPacket_.data()) {
					memStorage_.Free(readPacket_.data());
				}
			}

			virtual Packet* HandleRead(uint8_t& err_code) = 0;
			virtual void HandleAsyncRead(CBAsyncRead cb = nullptr) = 0;

			int HandleWrite(Packet& packet, uint8_t& err_code) {
				asio::error_code ec;
				auto size = asio::write(socket_, asio::buffer(packet.data(), packet.length()), ec);
				if (ec == asio::stream_errc::eof) {
					HandleErr(E_ERR_T::e_Err_R, ec); // Connection closed cleanly by peer.
					err_code = NET_EOF;
					return -1;
				}
				else if (ec) {
					HandleErr(E_ERR_T::e_Err_W, ec);
					err_code = NET_OTHER;
					return -1;
				}
				else {
					return size;
				}
			}
			void HandleAsyncWrite(Packet& packet, CBAsyncWrite cb = nullptr) {
				AsyncWrite(packet, cb);
			}
			template<typename F>
			void SetCallBackError(F f) {
				cbErr_ = f;
			}

		protected:
			void HandleErr(E_ERR_T err_t, const asio::error_code& ec) {
				StreamWriter::Instance()->Write(std::cout, "[RWHandler] Err, reason:%s", ec.message().c_str());
				if (cbErr_) {
					if (ec == asio::stream_errc::eof)
						cbErr_(err_t, NET_EOF);
					else if (ec)
						cbErr_(err_t, NET_OTHER);
				}
			}
			int ReadBody() {
				std::error_code ec;
				size_t len = readHPacket_.bodyLen();
				if (len == 0) {//todo
					StreamWriter::Instance()->Write(std::cout, "[RWHandler] Read Body len=%d", len);
					return -3;
				}
				Buffer buff(memStorage_.Alloc(Packet::e_HeadLen + len), Packet::e_HeadLen + len);
				readPacket_ = Packet(buff);
				auto size = asio::read(socket_, asio::buffer(readPacket_.body(), len), transfer_exactly(len), ec);
				if (ec) {
					if (readPacket_.data()) {
						int ret = memStorage_.Free(readPacket_.data());
						readPacket_.reset();
					}
					HandleErr(E_ERR_T::e_Err_R, ec); // Some other error.				
					return -1;
				}
				return size;
			}
			void AsyncReadBody(CBAsyncRead cb) {
				size_t len = readHPacket_.bodyLen();
				if (len == 0) {//todo
					StreamWriter::Instance()->Write(std::cout, "[RWHandler] Async read Body len=%d", len);
					return;
				}
				Buffer buff(memStorage_.Alloc(Packet::e_HeadLen + len), Packet::e_HeadLen + len);
				readPacket_ = Packet(buff);
				auto self = this->shared_from_this();
				asio::async_read(socket_, asio::buffer(readPacket_.body(), len), transfer_exactly(len), \
					[self, cb](const asio::error_code& ec, std::size_t size) {
						if (ec) {
							if (self->readPacket_.data()) {
								int ret = self->memStorage_.Free(self->readPacket_.data());
								self->readPacket_.reset();
							}
							self->HandleErr(E_ERR_T::e_Err_R, ec);
							return;
						}
						if (cb)
							cb(&self->readPacket_, NO_ERR);
						return;
					});
				return;
			}
			void AsyncWrite(Packet& packet, CBAsyncWrite cb) {
				auto self = this->shared_from_this();
				asio::async_write(socket_, asio::buffer(packet.data(), packet.length()), \
					[self, cb](std::error_code ec, std::size_t size) {
						try
						{
							if (ec)
								self->HandleErr(E_ERR_T::e_Err_W, ec);
							else
								if (cb)
									cb(size, NO_ERR);
						}
						catch (const std::exception& e)
						{
							std::cout << "[RWHandler] Async write rasie ex:" << e.what() << std::endl;
							throw e;
						}
					});
			}

		protected:
			SocketType& socket_;
			Packet readHPacket_;
			Packet readPacket_;
			CBErrRW cbErr_{ nullptr };
			NMemoryStorage<char>& memStorage_;
			//std::array<char, MAX_IP_PACK_SIZE> buff_{};
		};

		template<typename SocketType>
		class RWHandlerTcpNS : public RWHandlerTcp<SocketType> {
		public:
			explicit RWHandlerTcpNS(io_context& io_c, SocketType& socket, NMemoryStorage<char>& mem_storage) :
				RWHandlerTcp<SocketType>(io_c, socket, mem_storage) {
				this->readPacket_ = Packet(Buffer(this->memStorage_.Alloc(PacketSizeTcp), PacketSizeTcp));
			}

			virtual Packet* HandleRead(uint8_t& err_code) override {
				this->readPacket_.clear();
				std::error_code ec;
				auto size = this->socket_.read_some(asio::buffer(this->readPacket_.data(), this->readPacket_.bufflength()), ec);
				if (ec) {
					this->HandleErr(E_ERR_T::e_Err_R, ec); // Some other error.
					err_code = NET_OTHER;
					return nullptr;
				}
				this->readPacket_.setBodyLen(size, err_code);
				return &this->readPacket_;
			}
			virtual void HandleAsyncRead(CBAsyncRead cb = nullptr) override {
				this->readPacket_.clear();
				auto self = std::dynamic_pointer_cast<RWHandlerTcpNS>(this->shared_from_this());
				this->socket_.async_read_some(asio::buffer(this->readPacket_.data(), this->readPacket_.bufflength()), \
					[self, cb](const asio::error_code& ec, std::size_t size) {
						if (ec) {
							if (self->readPacket_.data()) {
								int ret = self->memStorage_.Free(self->readPacket_.data());
								self->readPacket_.reset();
							}
							self->HandleErr(E_ERR_T::e_Err_R, ec);
							return;
						}
						uint8_t err_code = NO_ERR;
						self->readPacket_.setBodyLen(size, err_code);
						if (cb)
							cb(&self->readPacket_, err_code);
						return;
					});
				return;
			}
		};
		template<typename SocketType>
		class RWHandlerTcpS : public RWHandlerTcp<SocketType> {
		public:
			explicit RWHandlerTcpS(io_context& io_c, tcp::socket& socket, NMemoryStorage<char>& mem_storage) :
				RWHandlerTcp<SocketType>(io_c, socket, mem_storage) {
				this->readHPacket_ = Packet(Buffer(this->memStorage_.Alloc(Packet::e_HeadLen), Packet::e_HeadLen));
			}

			virtual Packet* HandleRead(uint8_t& err_code) override {
				this->readHPacket_.clear();
				if (this->readPacket_.data()) {
					this->memStorage_.Free(this->readPacket_.data());
					this->readPacket_.reset();
				}

				std::error_code ec;
				auto size = asio::read(this->socket_, buffer(this->readHPacket_.data(), Packet::e_HeadLen), transfer_exactly(Packet::e_HeadLen), ec);
				if (ec == asio::stream_errc::eof) {
					this->HandleErr(E_ERR_T::e_Err_R, ec); // Connection closed cleanly by peer.
					err_code = NET_EOF;
					return nullptr;
				}
				else if (ec) {
					this->HandleErr(E_ERR_T::e_Err_R, ec); // Some other error.
					err_code = NET_OTHER;
					return nullptr;
				}
				if (this->readHPacket_.decodeHeader(err_code) < 0) {
					return nullptr;
				}
				auto ret = this->ReadBody();
				if (ret < 0) {
					err_code = NET_BAD_BODY;
					return nullptr;
				}
				return &this->readPacket_;
			}
			virtual void HandleAsyncRead(CBAsyncRead cb = nullptr) override {
				this->readHPacket_.clear();
				if (this->readPacket_.data()) {
					this->memStorage_.Free(this->readPacket_.data());
					this->readPacket_.reset();
				}

				auto self = std::dynamic_pointer_cast<RWHandlerTcpS>(this->shared_from_this());
				asio::async_read(this->socket_, asio::buffer(this->readHPacket_.data(), Packet::e_HeadLen), transfer_exactly(Packet::e_HeadLen), \
					[self, cb](const asio::error_code& ec, std::size_t size) {
						if (ec) {
							self->HandleErr(E_ERR_T::e_Err_R, ec);
							return;
						}
						uint8_t err_code{ NO_ERR };
						if (self->readHPacket_.decodeHeader(err_code) < 0) {
							if (cb)
								cb(nullptr, err_code);
							return;
						}
						self->AsyncReadBody(cb);
					});
				return;
			}
		};

		typedef std::function<void(const udp::endpoint& endpoint, std::size_t, uint8_t)> CBAsyncWriteUdp;
		typedef std::function<void(const udp::endpoint& endpoint, Packet*, uint8_t)>  CBAsyncReadUdp;

		class RWHandlerUdp : public std::enable_shared_from_this<RWHandlerUdp> {
		public:
			explicit RWHandlerUdp(io_context& io_c, udp::socket& socket, NMemoryStorage<char>& mem_storage) :
				socket_(socket), memStorage_(mem_storage), readHPacket_() {
				readHPacket_ = Packet(Buffer(memStorage_.Alloc(PacketSizeUdp), PacketSizeUdp));
			}
			virtual ~RWHandlerUdp() {
				if (readHPacket_.data()) {
					memStorage_.Free(readHPacket_.data());
				}
			}

			Packet* HandleRead(udp::endpoint& endpoint, uint8_t& err_code) {
				readHPacket_.clear();

				asio::error_code ec;
				size_t size = socket_.receive_from(asio::buffer(readHPacket_.data(), readHPacket_.bufflength()), endpoint, 0, ec);
				if (ec) {
					err_code = NET_OTHER;
					return nullptr;
				}
				else {
					int ret = readHPacket_.decodeHeader(err_code);
					if (ret == -1)
						return nullptr;
					else if (ret == -2)
						readHPacket_.setBodyLen(size, err_code);

					return &readHPacket_;
				}

			}
			void HandleAsyncRead(udp::endpoint& endpoint, CBAsyncReadUdp cb = nullptr) {
				readHPacket_.clear();

				auto self = shared_from_this();
				socket_.async_receive_from(
					asio::buffer(readHPacket_.data(), readHPacket_.bufflength()), endpoint,
					[self, endpoint, cb](std::error_code ec, std::size_t size)
					{
						if (ec) {
							self->HandleErr(E_ERR_T::e_Err_R, ec);
							return;
						}
						uint8_t err_code{ NO_ERR };
						int ret = self->readHPacket_.decodeHeader(err_code);
						if (ret == -1) {
							if (cb)
								cb(endpoint, nullptr, err_code);
						}
						else if (ret == -2)
							self->readHPacket_.setBodyLen(size, err_code);
						if (cb)
							cb(endpoint, &self->readHPacket_, err_code);

					});
				return;
			}

			int HandleWrite(const udp::endpoint& endpoint, Packet& packet, uint8_t& err_code) {
				asio::error_code ec;
				size_t size = socket_.send_to(asio::buffer(packet.data(), packet.length()), endpoint, 0, ec);
				if (ec) {
					err_code = NET_OTHER;
					return -1;
				}
				return size;
			}
			void HandleAsyncWrite(const udp::endpoint& endpoint, Packet& packet, CBAsyncWriteUdp cb = nullptr) {
				auto self = shared_from_this();
				socket_.async_send_to(
					asio::buffer(packet.data(), packet.length()), endpoint,
					[self, endpoint, cb](std::error_code ec, std::size_t size)
					{
						try
						{
							if (ec)
								self->HandleErr(E_ERR_T::e_Err_W, ec);
							else
								if (cb)
									cb(endpoint, size, NO_ERR);
						}
						catch (const std::exception& e)
						{
							StreamWriter::Instance()->Write(std::cout, "[RWHandler] async write rasie ex:%s", e.what());
							throw e;
						}
					});
				return;
			}
			template<typename F>
			void SetCallBackError(F f) {
				cbErr_ = f;
			}

		protected:
			void HandleErr(E_ERR_T err_t, const asio::error_code& ec) {
				StreamWriter::Instance()->Write(std::cout, "[RWHandler] Err, reason:%s", ec.message().c_str());
				if (cbErr_) {
					if (ec == asio::stream_errc::eof)
						cbErr_(err_t, NET_EOF);
					else if (ec)
						cbErr_(err_t, NET_OTHER);
				}
			}

		protected:
			udp::socket& socket_;
			Packet readHPacket_;
			Packet readPacket_;
			CBErrRW cbErr_{ nullptr };
			NMemoryStorage<char>& memStorage_;
		};
	}
}