#pragma once
//========================================================================
//[File Name]:HttpServer.h
//[Description]: the implemetation of http server.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "asio.hpp"
#include <string>
#include <mutex>

#ifndef OPENSSL
#include "../stream/Server.h"
#else
#include "../stream/ServerSSL.h"
using namespace Net::Stream::SSL;
#endif //OPENSSL

#include "../com/ObjectPool.h"
#include "StreamWriter.h"
#include "HttpRequestHandler.h"
#include "HttpConnectionMng.h"
#include "MD5.h"


using namespace asio;
using namespace asio::ip;
using namespace Net;
using namespace Net::Stream;

namespace Http {

    class HttpServer
    {
        using SPReqParser = std::shared_ptr<HttpRequestParser>;
        struct ReqRem {
            int id{-1};
            char* data{nullptr};
            unsigned len{0};
            unsigned buf_len{ 0 };
            SPReqParser parser{nullptr};

            ~ReqRem() { SAFE_DELETE_ARRAY(data); }
            void reset() {
                id = -1; len = 0; buf_len = 0;
                SAFE_DELETE_ARRAY(data);
            }
        };
    public:
        explicit HttpServer(const char* ip, short port, const std::string& doc_root,
             unsigned thd_num = std::thread::hardware_concurrency()) :
            ioCtx_(), workThdNum_(thd_num), worker_(ioCtx_){
            requestHandler_ = new HttpRequestHandler(doc_root);
            assert(requestHandler_);
            objPool_.Init(32);
#ifdef OPENSSL
            SaSSLInfo_t sa_info = {"server.pem", "dh2048.pem", "test"};
            server_ = new ServerTcpSSL(ioCtx_, ip, port, sa_info);
#else
            server_ = new ServerTcp(ioCtx_, ip, port);
#endif //OPENSSL
            assert(server_);
            connMng_ = new HttpConnectionMng(ioCtx_);
            assert(connMng_);
            Init();
        }
        ~HttpServer() {
            SAFE_DELETE(requestHandler_);
            SAFE_DELETE(connMng_);
            SAFE_DELETE(server_);

            for (auto itr = um_CkFileMng_.begin(); itr != um_CkFileMng_.end();) {
                if (itr->second) {
                    SAFE_DELETE(itr->second);
                    itr = um_CkFileMng_.erase(itr);
                }
                else
                    itr++;
            }
        }

        void Start() {
            for (auto i = 0; i < workThdNum_; ++i) {
                v_workThd_.emplace_back(std::thread([&]() {
                    ioCtx_.run();
                    }));
            }
            if (server_)
                server_->StartA();
            connMng_->Run();
        }
        void Stop() {
            connMng_->Stop();
            if (server_)
                server_->Stop();
            
            ioCtx_.stop();
            for (auto& var : v_workThd_) {
                if (var.joinable())
                    var.join();
            }
        }
        void Close(int id) { server_->Close(id); }
        void OnRequest(const char* req, OnRecvReqCB callback) {
            requestHandler_->AddReqHandler(req, callback);
        }

        int SendChunkFile(int id, const char* chunk, unsigned len, bool b_end, uint8_t& ec) {
            if (server_->IsConnected(id) != 1)
                return -1;
            auto find = um_CkFileMng_.find(id);
            if (find != um_CkFileMng_.end()) {
                int ret = find->second->SendChunkFile(id, chunk, len, b_end, ec);
                if (b_end) {
                    SAFE_DELETE(find->second);
                    um_CkFileMng_.erase(find);
                }
                return ret;
            }
            else {
                HttpChunkFileMng* ck_file_mng = new HttpChunkFileMng(server_);
                assert(ck_file_mng);            
                int ret = ck_file_mng->SendChunkFile(id, chunk, len, b_end, ec);
                if (b_end)
                    SAFE_DELETE(ck_file_mng);
                um_CkFileMng_.emplace(id, ck_file_mng);
                return ret;
            }
        }
        //todo: usage: to receive the chunk file (entitybody and trailer)
        template<typename F>  //void(char*, int) param: data, len
        int RecvChunkFile(int id, F callback, bool b_trailer, uint8_t& ec) {
            if (server_->IsConnected(id) != 1)
                return -1;
            auto find = um_CkFileMng_.find(id);
            if (find != um_CkFileMng_.end()) {
                int ret = find->second->RecvChunkFile(id, callback, b_trailer, ec);
                SAFE_DELETE(find->second);
                um_CkFileMng_.erase(find);
                return ret;
            }
            else {
                HttpChunkFileMng* ck_file_mng = new HttpChunkFileMng(server_);
                assert(ck_file_mng);
                int ret = ck_file_mng->RecvChunkFile(id, callback, b_trailer, ec);
                SAFE_DELETE(find->second);
                um_CkFileMng_.erase(find);
                return ret;
            }
        }

    protected:
        void Init() {
            CreateConnStrategy();

            server_->SetEventCB([this](int id, const Event& event) {
                switch (event.type) {
                case Event::eEvent_t::e_connClosed:
                case Event::eEvent_t::e_netEof:
                case Event::eEvent_t::e_netErr:
                {
                    UpdateConn(id, ConnState::e_Closed);
                    break;
                }
                default:
                    break;
                }
                });

            server_->SetReceiveCB([this](int id, const char* data, size_t len, uint8_t ec) {
                UpdateConn(id, ConnState::e_Active);

                if (data) {
                    auto find = um_ReqRem_.find(id);
                    if (find != um_ReqRem_.end()) {
                        if (find->second.len + len > find->second.buf_len) {
                            int mem_len = find->second.len + len;
                            char* data_r = new char[mem_len];
                            memcpy_s(data_r, mem_len, find->second.data, find->second.len);
                            memcpy_s(data_r + find->second.len, len, data, len);
                            delete[] find->second.data; find->second.data = data_r;
                            find->second.len = mem_len;
                            find->second.buf_len = mem_len;
                        }
                        else {
                            memcpy_s(find->second.data + find->second.len, find->second.buf_len - find->second.len, data, len);
                            find->second.len += len;
                        }
                        int ret = DoReceive(id, find->second.parser, find->second.data, find->second.len);
                        if (ret != 1) um_ReqRem_.erase(id);
                    }
                    else {
                        auto parser = objPool_.Get<>();
                        if (parser == nullptr) {
                            parser = std::make_shared<HttpRequestParser>();
                            assert(parser);
                        }
                        parser->Reset();
                        int ret = DoReceive(id, parser, data, len);
                    }
                }
                else {
                    //todo
                    StreamWriter::Instance()->Write(std::cout, "[HttpServer] session id=%d, receive data failed, ec=%d", id, (int)ec);
                }
            });
        }
        
        int DoReceive(int id, SPReqParser parser, const char* data, int len) {
            int ret = parser->Parse(data, len);
            if (ret == -1) {
                HttpRequest& request = parser->GetResult();
                if (!HttpTools::GetHeaderValue(request, "Transfer-Encoding").empty()) {
                    std::string md5_l = um_CkFileMng_[id]->GetMD5();
                    std::string md5_r = HttpTools::GetHeaderValue(request, "Content-MD5");
                    if (md5_l == md5_r)
                        request.entityBody_ = StatusStrings::to_string(StatusType::e_ok);
                    else
                        request.entityBody_ = StatusStrings::to_string(StatusType::e_undef); //todo md5 verify failed status code?
                }

                HttpResponse resp;
                requestHandler_->HandleRequest(id, request, resp);
                //resp
                uint8_t ec = 0;
                std::string data = resp.ToString();
                int ret = this->server_->Send(id, data.c_str(), data.length(), ec);//todo if failed?
                return 0;
            }
            else if (ret >= 0) {
                int data_len = len - ret;
                
                auto find = um_ReqRem_.find(id);
                if (find != um_ReqRem_.end()) {
                    if (data_len > 0) {
                        memcpy_s(find->second.data, data_len, data + ret, data_len);
                        find->second.len = data_len;
                    }
                    else
                        find->second.len = 0;
                }
                else {
                    ReqRem req_rem;
                    req_rem.id = id;
                    if (data_len > 0) {
                        req_rem.data = new char[data_len];
                        memcpy_s(req_rem.data, data_len, data + ret, data_len);
                    }
                    req_rem.len = data_len;
                    req_rem.parser = parser;
                    um_ReqRem_.emplace(id, req_rem);
                }
                //check if it is chunk file encoding transfered?
                std::string head_value = HttpTools::GetHeaderValue(parser->GetResult(), "Transfer-Encoding");
                if (head_value == std::string("chunked")) {
                    HttpResponse resp;
                    HttpRequest& request = parser->GetResult();
                    um_CkFileMng_[id]->UpdateMD5(request.entityBody_.data(), request.entityBody_.length());
                    requestHandler_->HandleRequest(id, request, resp);
                }
                return 1;
            }
            else if (ret == -3) {
                //resp bad request.
                HttpResponse resp = HttpResponse::defaultResp(StatusType::e_bad_request);
                uint8_t ec = 0;
                std::string data = resp.ToString();
                int ret = this->server_->Send(id, data.c_str(), data.length(), ec);//todo if failed?
                return -1;
            }
            else
                return -2;
        }

        void CreateConnStrategy() {
            connMng_->AddStrategy("IdleConn", [this](HttpConnectionMng::UM_ConnState& um_conn_state) {
                for (auto iter = um_conn_state.begin(); iter != um_conn_state.end();) {
                    if (iter->second == ConnState::e_Idle) {
                        this->Close(iter->first);
                        iter = um_conn_state.erase(iter);
                    }
                    else if (iter->second == ConnState::e_Closed)
                        iter = um_conn_state.erase(iter);
                    else
                        iter++;
                }
                }, true);
            connMng_->AddStrategy("MaxConn", [this](HttpConnectionMng::UM_ConnState& um_conn_state) {
                for (auto iter = um_conn_state.begin(); iter != um_conn_state.end();) {
                    if (iter->second == ConnState::e_Idle) {
                        this->Close(iter->first);
                        iter = um_conn_state.erase(iter);
                    }
                    else if (iter->second == ConnState::e_Closed)
                        iter = um_conn_state.erase(iter);
                    else
                        iter++;
                }
                });
        }

        void UpdateConn(int id, ConnState state) {
            connMng_->UpdateConn(id, state);
        }

    protected:
        io_context ioCtx_;
        io_context::work worker_;
        int workThdNum_{ 1 };
        std::vector<std::thread> v_workThd_;
        ServerBase* server_{ nullptr };
        // The signal_set is used to register for process termination notifications.
        //signal_set signals_;
        HttpRequestHandler* requestHandler_{ nullptr };
        DesignPattern::ObjectPool<HttpRequestParser> objPool_;
        std::unordered_map<int, ReqRem> um_ReqRem_;
        HttpConnectionMng* connMng_{ nullptr };
        std::unordered_map<int, HttpChunkFileMng*> um_CkFileMng_;
    };
}