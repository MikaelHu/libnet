#pragma once
//========================================================================
//[File Name]:Packet.h
//[Description]: a definition of stream packet.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <cstddef>
#include <string>

#include "../Comm.h"


namespace Net {
//#define PACKET_STICK
	namespace Core {

		struct Buffer {
		public:
			explicit Buffer() = default;
			explicit Buffer(char* buff, std::size_t len) : data_(buff), len_(len) {
				assert(data_);
				assert(len_ > 0);
				memset(data_, 0, sizeof(char) * len_);
			}

			Buffer(const Buffer& ref) : data_(ref.data_), len_(ref.len_) {}
			Buffer& operator=(const Buffer&) = default;
			Buffer(Buffer&& ref) noexcept : data_(ref.data_), len_(ref.len_) {
				ref.data_ = nullptr;
				ref.len_ = 0;
			}
			Buffer& operator=(Buffer&& ref) noexcept {
				if (this != &ref) {
					this->data_ = ref.data_;
					ref.data_ = nullptr;
					this->len_ = ref.len_;
					ref.len_ = 0;
				}
				return *this;
			}
			void clear() {
				memset(data_, 0, sizeof(char) * len_);
			}
			void reset() {
				data_ = nullptr;
				len_ = 0;
			}
		public:
			char* data_{ nullptr };
			std::size_t len_{ 0 };
		};

		class Packet {
		public:
#ifdef PACKET_STICK
			enum { e_HeadLen = 2 };
#else
			enum { e_HeadLen = 0 };
#endif // PACKET_STICK
			enum { e_Max_BodyLen = MaxPacketSize - e_HeadLen };

		public:
			explicit Packet() = default;
			explicit Packet(const Buffer& buff) : buff_(buff) {
				assert(buff_.data_);
				assert(buff_.len_ < e_Max_BodyLen);
			}

			Packet(const Packet&) = default;
			Packet& operator=(const Packet&) = default;
			Packet(Packet&& ref) noexcept : bodyLen_(ref.bodyLen_), buff_(std::move(ref.buff_)) { }
			Packet& operator=(Packet&& ref) noexcept {
				if (this != &ref) {
					bodyLen_ = ref.bodyLen_;
					buff_ = std::move(ref.buff_);
				}
				return *this;
			}

			const char* data() const {
				return buff_.data_;
			}
			char* data() {
				return buff_.data_;
			}
			std::size_t length() const {
				return e_HeadLen + bodyLen_;
			}
			std::size_t bufflength() const {
				return buff_.len_;
			}
			const char* body() const {
				return buff_.data_ + e_HeadLen;
			}
			char* body() {
				return buff_.data_ + e_HeadLen;
			}
			std::size_t bodyLen() const {
				return bodyLen_;
			}
			int setBodyLen(size_t len, uint8_t& ec) {
				if (len > buff_.len_ - e_HeadLen) {
					ec = NET_BAD_BODY;
					return -1;
				}
				bodyLen_ = len;
				return 0;
			}
			int decodeData(uint8_t& ec) {
				return decodeHeader(ec);
			}
			int decodeHeader(uint8_t& ec) {
				if (e_HeadLen > 0) {
					char header[e_HeadLen + 1] = "";
					strncat_s(header, buff_.data_, e_HeadLen);
					sscanf(header, "%x", &bodyLen_);
					if (bodyLen_ > e_Max_BodyLen) {
						bodyLen_ = 0;
						ec = NET_BAD_HEAD;
						return -1;
					}
					return 0;
				}
				else
					return -2;
			}
			int encodeData(const char* data, unsigned len, uint8_t& err_code) {
				if (setBodyLen(len, err_code) < 0)
					return -1;

				if (e_HeadLen > 0)
					encodeHeader();
				memcpy_s(buff_.data_ + e_HeadLen, buff_.len_ - e_HeadLen, data, len);
				return 0;
			}
			void clear() {
				buff_.clear();
				bodyLen_ = 0;
			}
			void reset() {
				buff_.reset();
				bodyLen_ = 0;
			}

		protected:
			void encodeHeader() {
				char header[e_HeadLen + 1] = "";
				sprintf_s(header, "%x", bodyLen_);
				memcpy_s(buff_.data_, buff_.len_, header, e_HeadLen);
			}

		protected:
			Buffer buff_;
			std::size_t bodyLen_{ 0 };
		};
	}
}