#pragma once
//========================================================================
//[File Name]:HttpRequestHandler.h
//[Description]: The definition of http request handler .
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <string>
#include <unordered_map>
#include "../stream/ReqRespHandler.h"
#include "HttpReqResp.h"
#include "HttpDefines.h"


namespace Http {
    
	class HttpRequestHandler : public RequestHandler
	{
	public:
		explicit HttpRequestHandler(const std::string& doc_root): RequestHandler(), docRoot_(doc_root){}
		virtual ~HttpRequestHandler() = default;

        void AddReqHandler(const char* req, OnRecvReqCB callback) {
            auto find = umReqHandler_.find(req);
            if (find == umReqHandler_.end()) {
                umReqHandler_.emplace(req, callback);
            }
        }
		virtual void HandleRequest(int id, const Request& req, Response& rep) override {
            HttpRequest& request = dynamic_cast<HttpRequest&>(const_cast<Request&>(req));            

            char tag[256]{ 0 };
            sprintf(tag, "%s %s", request.method_.c_str(), request.url_.c_str());
            auto find = umReqHandler_.find(tag);
            if (find != umReqHandler_.end()) {
                uint8_t error_code = 0;
                if (!HttpTools::GetHeaderValue(request, "Content-MD5").empty())
                    error_code = 0xFF;
                find->second(request.entityBody_, id, error_code, [this, request, &rep](int session_id, std::string content) {
                    HttpResponse& response = dynamic_cast<HttpResponse&>(rep);
                    response.status_ = StatusType::e_ok;
                    response.entityBody_ = content/*StockResps::to_string(StatusType::e_ok)*/;
                    response.v_headers_.resize(5);
                    response.v_headers_[0].name = "Content-Length";
                    response.v_headers_[0].value = std::to_string(response.entityBody_.size());
                    response.v_headers_[1].name = "Content-Type";
                    response.v_headers_[1].value = "text/plain";
                    response.v_headers_[2].name = "TE";
                    response.v_headers_[2].value = "trailers, chunked";
                    response.v_headers_[3].name = "Transfer-Encoding";
                    response.v_headers_[3].value = "chunked";
                    response.v_headers_[4].name = "Trailer";
                    response.v_headers_[4].value = "Content-MD5";

                    int ret = HttpTools::CheckConnection(request);
                    if (ret == 1) {
                        HttpHeader_t header;
                        header.name = "Connection";
                        header.value = "keep-alive";
                        response.v_headers_.emplace_back(header);
                    }
                    else if (ret == 0){
                        HttpHeader_t header;
                        header.name = "Connection";
                        header.value = "close";
                        response.v_headers_.emplace_back(header);
                    }
                    return;
                });
            }
            else {
                HttpResponse& response = dynamic_cast<HttpResponse&>(rep);
                response = HttpResponse::defaultResp(StatusType::e_bad_request);

                int ret = HttpTools::CheckConnection(request);
                if (ret == 1) {
                    HttpHeader_t header;
                    header.name = "Connection";
                    header.value = "keep-alive";
                    response.v_headers_.emplace_back(header);
                }
                else if (ret == 0) {
                    HttpHeader_t header;
                    header.name = "Connection";
                    header.value = "close";
                    response.v_headers_.emplace_back(header);
                }
            }
		}

	protected:
		HttpRequestHandler(const HttpRequestHandler&) = delete;
		HttpRequestHandler& operator=(const HttpRequestHandler&) = delete;

	protected:
		std::string docRoot_;
        std::unordered_map<std::string, OnRecvReqCB> umReqHandler_;
	};
}
