#pragma once
//========================================================================
//[File Name]:HttpTools.h
//[Description]: some useful tools for http.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <string>
#include <fstream>
#include "HttpDefines.h"
#include "../com/StringTools.h"


namespace Http {

    class HttpTools {
    public:
        template<typename T>
        static int CheckMethod(const T& in) {
            const char* method = in.method_.c_str();
            size_t len = in.method_.length();
            for (int i = 0; i < len; i++) {
                if (!StringTools::IsChar(*(method + i)) || StringTools::IsCtl(*(method + i)) || StringTools::IsTspecial(*(method + i)))
                    return -1;
                for (int i = 0; i < sizeof(Http::METHOD); i++) {
                    if (strcmp(method, Http::METHOD[i]) == 0)
                        return 0;
                }
            }
            return -1;
        }
        template<typename T>
        static int CheckUrl(const T& in) {
            const char* url = in.url_.c_str();
            size_t len = in.url_.length();
            for (int i = 0; i < len; i++) {
                if (StringTools::IsCtl(*(url + i)))
                    return -1;
            }
            return 0;
        }
        template<typename T>
        static int CheckVersion(const T& in) {//todo
            const char* version = in.version_.c_str();
            size_t len = in.version_.length();
            if (StringTools::FindFirstOf(version, 0, "HTTP/", 5) == 0)
                return 0;
            return -1;
        }
        template<typename T>
        static std::string GetHeaderValue(const T& in, const std::string& header) {
            auto find = std::find_if(in.v_headers_.begin(), in.v_headers_.end(), [header](const HttpHeader_t& item)->bool {
                return item.name == header;
                });
            if (find != in.v_headers_.end()) {
                return find->value;
            }
            return "";
        }
        template<typename T>
        static int CheckConnection(const T& in) {
            auto find = std::find_if(in.v_headers_.begin(), in.v_headers_.end(), [](const HttpHeader_t& header)->bool {
                return header.name == std::string("Connection");
                });
            if (find != in.v_headers_.end()) {
                if (find->value == "close")
                    return 0;
                else if (find->value == "keep-alive")
                    return 1;
            }
            return -1;
        }
        template<typename T>
        static int GetEntityBodyLen(const T& in) {
            auto find = std::find_if(in.v_headers_.begin(), in.v_headers_.end(), [](const HttpHeader_t& header)->bool {
                return header.name == std::string("Content-Length");
                });
            if (find != in.v_headers_.end()) {
                size_t body_len = std::stoi(find->value);
                return body_len;
            }
            return -1;
        }
        template<typename T>
        static int CheckEntityBody(const T& in) {
            int len = GetEntityBodyLen(in);
            if (len < 0)
                return -2;
            size_t len_a = in.entityBody_.length();
            if (len == len_a)
                return 0;
            else if (len > len_a)
                return len_a;
            else //len < len_a
                return -1;
        }
        static bool DecodeUrl(const std::string& in, std::string& out) {
            out.clear();
            out.reserve(in.size());
            for (std::size_t i = 0; i < in.size(); ++i)
            {
                if (in[i] == '%')
                {
                    if (i + 3 <= in.size())
                    {
                        int value = 0;
                        std::istringstream is(in.substr(i + 1, 2));
                        if (is >> std::hex >> value)
                        {
                            out += static_cast<char>(value);
                            i += 2;
                        }
                        else
                            return false;
                    }
                    else
                        return false;
                }
                else if (in[i] == '+')
                    out += ' ';
                else
                    out += in[i];
            }
            return true;
        }
    };
}