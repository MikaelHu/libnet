#pragma once
//========================================================================
//[File Name]:ReqRespHandler.h
//[Description]: the definitions of abstract request handler and response handler.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "RequestResponse.h"


namespace Net {
	namespace Stream {
		class RequestHandler
		{
		public:
			explicit RequestHandler() { }
			virtual ~RequestHandler() = default;
			virtual void HandleRequest(int id, const Request& req, Response& rep) = 0;

		protected:
			RequestHandler(const RequestHandler&) = delete;
			RequestHandler& operator=(const RequestHandler&) = delete;
		};
		class ResponseHandler
		{
		public:
			explicit ResponseHandler() { }
			virtual ~ResponseHandler() = default;
			virtual void HandleResponse(const Response& rep) = 0;

		protected:
			ResponseHandler(const ResponseHandler&) = delete;
			ResponseHandler& operator=(const ResponseHandler&) = delete;
		};
	}
}
