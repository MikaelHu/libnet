#pragma once
//========================================================================
//[File Name]:RequestResponse.h
//[Description]: the definitions of abstract Request and Response.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================

namespace Net {
    namespace Stream {
        class Request {
        public:
            Request() = default;
            virtual ~Request() = default;
        };
        class Response {
        public:
            Response() = default;
            virtual ~Response() = default;
        };
    }
}