#pragma once
//========================================================================
//[File Name]:MultiCast.h
//[Description]: The impelemation of socket multicast(sender and receiver).
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <iostream>
#include <sstream>
#include <string>

#include "../asio/asio.hpp"
#include "../core/Session.h"
#include "StreamWriter.h"


using namespace asio::ip;
namespace Net {  
    using namespace Core;

    namespace MultiCast {
        class Sender
        {
        public:
            explicit Sender(asio::io_context& io_c, const char* ip, short port, unsigned timeout = 5)
                : ioCtx_(io_c), timer_(io_c),  memStorage_({ 32, 64, 256 }), timeout_(timeout) { 
                StreamWriter::Instance()->Write(std::cout, "[Sender] open sender port=%d!", port);
                session_ = std::make_shared<SessionUdp>(ioCtx_, memStorage_, timeout_);
                session_->Open(ip, port);
                session_->SetCallBackError([this](udp::endpoint endpoint, uint8_t ec) {
                    StreamWriter::Instance()->Write(std::cout, "[Sender] Peer ip=%s, port=%d, err code=%d", \
                        endpoint.address().to_string().c_str(), endpoint.port(), (int)ec);
                    });
            }
            ~Sender() {
                session_->Close();
            }

            int Send(const std::string& ip, short port, const char* data, unsigned len, uint8_t& ec) {
                return session_->Send(udp::endpoint(ip::address_v4::from_string(ip), port), data, len, ec);
            }
            int AsyncSend(const std::string& ip, short port, const char* data, unsigned len, CBAsyncSendUdp cb = nullptr) {
                return session_->AsyncSend(udp::endpoint(ip::address_v4::from_string(ip), port), data, len, cb);
            }
            udp::endpoint& GetNetPeer() {
                return session_->GetNetPeer();
            }

        protected:
            void HandleRWErr(const udp::endpoint& endpoint, uint8_t ec) {
                StreamWriter::Instance()->Write(std::cout, "[Sender] RW err, ip=%s, port=%d, ec=%d", \
                    endpoint.address().to_string().c_str(), endpoint.port(), (int)ec);
            }
            void OnTimer(){
                if (timeout_ > 0) {
                    timer_.expires_after(std::chrono::seconds(timeout_));
                    timer_.async_wait(
                        [this](const std::error_code& ec)
                        {
                            if (ec != asio::error::operation_aborted)
                                this->session_->GetSocket().cancel();
                        });
                }
            }
            void OffTimer() {
                if (timeout_ > 0)
                    timer_.cancel();
            }

        protected:
            io_context& ioCtx_;
            unsigned timeout_{ 0 };
            asio::steady_timer timer_;
            NMemoryStorage<char> memStorage_;
            std::shared_ptr<SessionUdp> session_{ nullptr };
        };

        class Receiver
        {
        public:
            explicit Receiver(asio::io_context& io_c,
                const char* listen_address,
                const char* multicast_address,
                short multicast_port, unsigned timeout = 5)
                : ioCtx_(io_c), memStorage_(/*MemStorageSize, */{ 32, 64, 256 }),
                timeout_(timeout)
            {
                StreamWriter::Instance()->Write(std::cout, "[Receiver] open Receiver multicast port=%d!", multicast_port);
                session_ = std::make_shared<SessionUdp>(ioCtx_, memStorage_, timeout_);
                session_->Open(listen_address, multicast_port, true);
                session_->GetSocket().set_option(asio::ip::multicast::join_group(asio::ip::make_address(multicast_address)));
                session_->SetCallBackError([this](udp::endpoint endpoint, uint8_t ec) {
                    StreamWriter::Instance()->Write(std::cout, "[Receiver] err code=%d", (int)ec);
                    });
            }
            ~Receiver() {
                session_->Close();
            }

            int Recieve(char** data, uint8_t& ec) {
                return session_->Recieve(data, ec);
            }
            int AsyncRecieve(CBAsyncRecvUdp cb = nullptr) {
                return session_->AsyncRecieve([cb](const udp::endpoint& endpoint, Packet* packet, uint8_t ec) {
                    if (packet == nullptr) {
                        StreamWriter::Instance()->Write(std::cout, "[Receiver] Async recieve err, code=%d", (int)ec);
                        if (cb)
                            cb(endpoint, nullptr, 0, ec);
                    }
                    else {
                        if (cb)
                            cb(endpoint, packet->body(), packet->bodyLen(), ec);
                    }
                    });
            }
            udp::endpoint& GetNetPeer() {
                return session_->GetNetPeer();
            }

        protected:
            void HandleRWErr(const udp::endpoint& endpoint, uint8_t ec) {
                StreamWriter::Instance()->Write(std::cout, "[Receiver] RW err, ip=%s, port=%d, ec=%d", \
                    endpoint.address().to_string().c_str(), endpoint.port(), (int)ec);
            }

        private:
            io_context& ioCtx_;
            unsigned timeout_{ 0 };
            NMemoryStorage<char> memStorage_;
            std::shared_ptr<SessionUdp> session_{ nullptr };
        };
    }
}