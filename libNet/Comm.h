#pragma once
//========================================================================
//[File Name]:Comm.h
//[Description]: common defines for net.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "asio/asio.hpp"

using namespace asio::ip;

namespace Net {
#define SAFE_DELETE(p) {if(p){delete (p); (p) = nullptr;}}
#define SAFE_DELETE_ARRAY(p) {if(p){delete[] (p); (p) = nullptr;}}

	typedef std::function<void(int, char*, unsigned, uint8_t)>  CBAsyncRecv;
	typedef std::function<void(int, size_t, uint8_t)>	CBAsyncSend;
	typedef std::function<void(const udp::endpoint&, char*, unsigned, uint8_t)>  CBAsyncRecvUdp;
	typedef std::function<void(const udp::endpoint&, size_t, uint8_t)>	CBAsyncSendUdp;

	constexpr size_t MaxConnection = 65536;

	constexpr size_t MemStorageSize = 512;

	constexpr uint16_t PacketSizeTcp = 1024;
	constexpr uint16_t PacketSizeUdp = 1024;
	constexpr int MaxPacketSize = 65535;
	constexpr int MaxConnNum = 65536;
	constexpr int MaxRecvSize = 65535;

	//Error type definition:
	constexpr uint8_t NO_ERR = 0x00;
	constexpr uint8_t NET_EOF = 0x01;
	constexpr uint8_t NET_OTHER = 0x02;
	constexpr uint8_t NET_BAD_DATA = 0x10;
	constexpr uint8_t NET_BAD_HEAD = 0x11;
	constexpr uint8_t NET_BAD_BODY = 0x12;
}
