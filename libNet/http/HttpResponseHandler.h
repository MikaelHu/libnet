#pragma once
//========================================================================
//[File Name]:HttpResponseHandler.h
//[Description]: The definition of http response handler .
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <string>
#include <fstream>
#include "../stream/ReqRespHandler.h"
#include "HttpReqResp.h"


namespace Http {
    class HttpResponseHandler : public ResponseHandler
    {
    public:
        explicit HttpResponseHandler(const std::string& doc_root) : ResponseHandler(){}
        virtual ~HttpResponseHandler() = default;

        virtual void HandleResponse(const Response& rep) override {            
        }

    protected:
        HttpResponseHandler(const HttpResponseHandler&) = delete;
        HttpResponseHandler& operator=(const HttpResponseHandler&) = delete;
    };
}
