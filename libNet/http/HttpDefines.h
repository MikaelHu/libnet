#pragma once
//========================================================================
//[File Name]:HttpDefines.h
//[Description]: some definitions for http
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <string>
#include <sstream>
#include "../asio/asio.hpp"
#include "../com/StringTools.h"
#include "MD5.h"


namespace Http {
    using OnSendReqCB = std::function<void(std::string, uint8_t)>;
    using OnTranEnCB = std::function<void(std::string, uint8_t)>;
    using OnRspCB = std::function<void(int, std::string)>;          //session_id, content
    using OnRecvReqCB = std::function<void(std::string, int, uint8_t, OnRspCB)>; //content, session_id, error_code, callback

    enum class StatusType
    {
        e_undef = 0,
        e_ok = 200,
        e_created = 201,
        e_accepted = 202,
        e_no_content = 204,
        e_multiple_choices = 300,
        e_moved_permanently = 301,
        e_moved_temporarily = 302,
        e_not_modified = 304,
        e_bad_request = 400,
        e_unauthorized = 401,
        e_forbidden = 403,
        e_not_found = 404,
        e_internal_server_error = 500,
        e_not_implemented = 501,
        e_bad_gateway = 502,
        e_service_unavailable = 503
    };

    namespace StatusStrings {
        const std::string undef =
            "HTTP/1.1 0 Undef\r\n";
        const std::string ok =
            "HTTP/1.1 200 OK\r\n";
        const std::string created =
            "HTTP/1.1 201 Created\r\n";
        const std::string accepted =
            "HTTP/1.1 202 Accepted\r\n";
        const std::string no_content =
            "HTTP/1.1 204 No Content\r\n";
        const std::string multiple_choices =
            "HTTP/1.1 300 Multiple Choices\r\n";
        const std::string moved_permanently =
            "HTTP/1.1 301 Moved Permanently\r\n";
        const std::string moved_temporarily =
            "HTTP/1.1 302 Moved Temporarily\r\n";
        const std::string not_modified =
            "HTTP/1.1 304 Not Modified\r\n";
        const std::string bad_request =
            "HTTP/1.1 400 Bad Request\r\n";
        const std::string unauthorized =
            "HTTP/1.1 401 Unauthorized\r\n";
        const std::string forbidden =
            "HTTP/1.1 403 Forbidden\r\n";
        const std::string not_found =
            "HTTP/1.1 404 Not Found\r\n";
        const std::string internal_server_error =
            "HTTP/1.1 500 Internal Server Error\r\n";
        const std::string not_implemented =
            "HTTP/1.1 501 Not Implemented\r\n";
        const std::string bad_gateway =
            "HTTP/1.1 502 Bad Gateway\r\n";
        const std::string service_unavailable =
            "HTTP/1.1 503 Service Unavailable\r\n";

        std::string to_string(StatusType status) {
            switch (status)
            {
            case StatusType::e_undef:
                return const_cast<std::string&>(undef);
            case StatusType::e_ok:
                return const_cast<std::string&>(ok);
            case StatusType::e_created:
                return const_cast<std::string&>(created);
            case StatusType::e_accepted:
                return const_cast<std::string&>(accepted);
            case StatusType::e_no_content:
                return const_cast<std::string&>(no_content);
            case StatusType::e_multiple_choices:
                return const_cast<std::string&>(multiple_choices);
            case StatusType::e_moved_permanently:
                return const_cast<std::string&>(moved_permanently);
            case StatusType::e_moved_temporarily:
                return const_cast<std::string&>(moved_temporarily);
            case StatusType::e_not_modified:
                return const_cast<std::string&>(not_modified);
            case StatusType::e_bad_request:
                return const_cast<std::string&>(bad_request);
            case StatusType::e_unauthorized:
                return const_cast<std::string&>(unauthorized);
            case StatusType::e_forbidden:
                return const_cast<std::string&>(forbidden);
            case StatusType::e_not_found:
                return const_cast<std::string&>(not_found);
            case StatusType::e_internal_server_error:
                return const_cast<std::string&>(internal_server_error);
            case StatusType::e_not_implemented:
                return const_cast<std::string&>(not_implemented);
            case StatusType::e_bad_gateway:
                return const_cast<std::string&>(bad_gateway);
            case StatusType::e_service_unavailable:
                return const_cast<std::string&>(service_unavailable);
            default:
                return const_cast<std::string&>(internal_server_error);
            }
        }
        asio::const_buffer to_buffer(StatusType status)
        {
            switch (status)
            {
            case StatusType::e_ok:
                return asio::buffer(ok);
            case StatusType::e_created:
                return asio::buffer(created);
            case StatusType::e_accepted:
                return asio::buffer(accepted);
            case StatusType::e_no_content:
                return asio::buffer(no_content);
            case StatusType::e_multiple_choices:
                return asio::buffer(multiple_choices);
            case StatusType::e_moved_permanently:
                return asio::buffer(moved_permanently);
            case StatusType::e_moved_temporarily:
                return asio::buffer(moved_temporarily);
            case StatusType::e_not_modified:
                return asio::buffer(not_modified);
            case StatusType::e_bad_request:
                return asio::buffer(bad_request);
            case StatusType::e_unauthorized:
                return asio::buffer(unauthorized);
            case StatusType::e_forbidden:
                return asio::buffer(forbidden);
            case StatusType::e_not_found:
                return asio::buffer(not_found);
            case StatusType::e_internal_server_error:
                return asio::buffer(internal_server_error);
            case StatusType::e_not_implemented:
                return asio::buffer(not_implemented);
            case StatusType::e_bad_gateway:
                return asio::buffer(bad_gateway);
            case StatusType::e_service_unavailable:
                return asio::buffer(service_unavailable);
            default:
                return asio::buffer(internal_server_error);
            }
        }
    }
    namespace MiscStrings {
        const char name_value_separator[] = { ':', ' ' };
        const char crlf[] = { '\r', '\n' };
    }
    namespace StockResps {
        const char ok[] = "";
        const char created[] =
            "<html>"
            "<head><title>Created</title></head>"
            "<body><h1>201 Created</h1></body>"
            "</html>";
        const char accepted[] =
            "<html>"
            "<head><title>Accepted</title></head>"
            "<body><h1>202 Accepted</h1></body>"
            "</html>";
        const char no_content[] =
            "<html>"
            "<head><title>No Content</title></head>"
            "<body><h1>204 Content</h1></body>"
            "</html>";
        const char multiple_choices[] =
            "<html>"
            "<head><title>Multiple Choices</title></head>"
            "<body><h1>300 Multiple Choices</h1></body>"
            "</html>";
        const char moved_permanently[] =
            "<html>"
            "<head><title>Moved Permanently</title></head>"
            "<body><h1>301 Moved Permanently</h1></body>"
            "</html>";
        const char moved_temporarily[] =
            "<html>"
            "<head><title>Moved Temporarily</title></head>"
            "<body><h1>302 Moved Temporarily</h1></body>"
            "</html>";
        const char not_modified[] =
            "<html>"
            "<head><title>Not Modified</title></head>"
            "<body><h1>304 Not Modified</h1></body>"
            "</html>";
        const char bad_request[] =
            "<html>"
            "<head><title>Bad Request</title></head>"
            "<body><h1>400 Bad Request</h1></body>"
            "</html>";
        const char unauthorized[] =
            "<html>"
            "<head><title>Unauthorized</title></head>"
            "<body><h1>401 Unauthorized</h1></body>"
            "</html>";
        const char forbidden[] =
            "<html>"
            "<head><title>Forbidden</title></head>"
            "<body><h1>403 Forbidden</h1></body>"
            "</html>";
        const char not_found[] =
            "<html>"
            "<head><title>Not Found</title></head>"
            "<body><h1>404 Not Found</h1></body>"
            "</html>";
        const char internal_server_error[] =
            "<html>"
            "<head><title>Internal Server Error</title></head>"
            "<body><h1>500 Internal Server Error</h1></body>"
            "</html>";
        const char not_implemented[] =
            "<html>"
            "<head><title>Not Implemented</title></head>"
            "<body><h1>501 Not Implemented</h1></body>"
            "</html>";
        const char bad_gateway[] =
            "<html>"
            "<head><title>Bad Gateway</title></head>"
            "<body><h1>502 Bad Gateway</h1></body>"
            "</html>";
        const char service_unavailable[] =
            "<html>"
            "<head><title>Service Unavailable</title></head>"
            "<body><h1>503 Service Unavailable</h1></body>"
            "</html>";

        std::string to_string(StatusType status)
        {
            switch (status)
            {
            case StatusType::e_ok:
                return ok;
            case StatusType::e_created:
                return created;
            case StatusType::e_accepted:
                return accepted;
            case StatusType::e_no_content:
                return no_content;
            case StatusType::e_multiple_choices:
                return multiple_choices;
            case StatusType::e_moved_permanently:
                return moved_permanently;
            case StatusType::e_moved_temporarily:
                return moved_temporarily;
            case StatusType::e_not_modified:
                return not_modified;
            case StatusType::e_bad_request:
                return bad_request;
            case StatusType::e_unauthorized:
                return unauthorized;
            case StatusType::e_forbidden:
                return forbidden;
            case StatusType::e_not_found:
                return not_found;
            case StatusType::e_internal_server_error:
                return internal_server_error;
            case StatusType::e_not_implemented:
                return not_implemented;
            case StatusType::e_bad_gateway:
                return bad_gateway;
            case StatusType::e_service_unavailable:
                return service_unavailable;
            default:
                return internal_server_error;
            }
        }
    }
    namespace MimeTypes {
        struct mapping
        {
            const char* extension;
            const char* mime_type;
        } mappings[] =
        {
          { "gif", "image/gif" },
          { "htm", "text/html" },
          { "html", "text/html" },
          { "jpg", "image/jpeg" },
          { "png", "image/png" }
        };
        /// Convert a file extension into a MIME type.
        std::string extension_to_type(const std::string& extension) {
            for (mapping m : mappings)
                if (m.extension == extension)
                    return m.mime_type;
            return "text/plain";
        }
    }

    const char* METHOD[] = {
        "GET", "HEAD", "PUT", "POST", "TRACE", "OPTIONS", "DELETE", "LOCK", \
        "MKCOL", "COPY", "MOVE"
    };

    struct HttpHeader_t{
        std::string name;
        std::string value;

        bool Empty() const {
            return name.empty();
        }
        std::string ToString() {
            std::ostringstream os;
            os << name << ": " << value;
            return os.str();
        }
        int FromString(const std::string& header) {
            if (!header.empty()) {
                std::size_t pos = header.find_first_of(':');
                if (pos != std::string::npos) {
                    std::string str_name = header.substr(0, pos);
                    std::string str_value = header.substr(pos + 1, header.length() - pos - 1);
                    name = StringTools::Trim(str_name.c_str());
                    value = StringTools::Trim(str_value.c_str());
                    return 0;
                }
                return -2;
            }
            return -1;
        }
    };
    /*struct EntityBody {
        char* body_{ nullptr };
        size_t len_{ 0 };
        void Reset() {
            if (body_) {
                delete[] body_; body_ = nullptr;
            }
            len_ = 0;
        }
    };*/

    struct KeepAlive_t {
        std::string name;   //"keep-alive"
        int max{0};         //the server remained connecting capability.
        int timeOut{0};     //unity: second.

        HttpHeader_t ToHttpHeader() {
            HttpHeader_t header;
            header.name = name;
            std::stringstream ss;
            ss << "max=" << max << ", " << "timeOut=" << timeOut;
            ss >> header.value;
            return header;
        }
        int FromHttpHeader(const HttpHeader_t& header) {
            if (!header.Empty()) {
                name = header.name;
                if (DecodeKeepAlive(header.value, *this))
                    return 0;
                return -2;
            }
            return -1;
        }
        static bool DecodeKeepAlive(const std::string& in, KeepAlive_t& keep_alive) {
            if (in.empty())
                return false;
            std::size_t pos = std::string::npos;
            std::size_t pos1 = std::string::npos;
            std::size_t off_set = 0;
            do {
                pos = in.find_first_of(',', off_set);
                if (pos != std::string::npos)
                {
                    pos1 = in.find_first_of('=', off_set);
                    if (pos1 != std::string::npos) {
                        std::string str_name = in.substr(off_set, pos1 - off_set);
                        std::string str_value = in.substr(pos1 + 1, pos - pos1 - 1);
                        str_name = StringTools::Trim(str_name.c_str());
                        str_value = StringTools::Trim(str_value.c_str());

                        if (str_name == std::string("max"))
                            keep_alive.max = std::stoi(str_value);
                        else if (str_name == std::string("timeOut"))
                            keep_alive.timeOut = std::stoi(str_value);
                        off_set = pos + 1;
                    }
                    else
                        return false;
                }
                else {
                    pos1 = in.find_first_of('=', off_set);
                    if (pos1 != std::string::npos) {
                        std::string str_name = in.substr(off_set, pos1 - off_set);
                        std::string str_value = in.substr(pos1 + 1, in.length() - pos1 - 1);
                        str_name = StringTools::Trim(str_name.c_str());
                        str_value = StringTools::Trim(str_value.c_str());

                        if (str_name == std::string("max"))
                            keep_alive.max = std::stoi(str_value);
                        else if (str_name == std::string("timeOut"))
                            keep_alive.timeOut = std::stoi(str_value);
                    }
                    else
                        return false;
                }
            } while (pos != std::string::npos);
            return true;
        }
    };
    struct HttpVersion_t {
        std::string name; //"HTTP"
        short major{ 0 };
        short minor{ 0 };

        std::string ToString() {
            std::ostringstream os;
            os << name << "/" << major << "." << minor;
            return os.str();
        }
        int FromString(const std::string& ver) {
            if (!ver.empty()) {
                std::size_t pos = ver.find_first_of('/');
                if (pos != std::string::npos) {
                    std::string str_name = ver.substr(0, pos);
                    std::size_t pos1 = ver.find_first_of('.', pos);
                    if (pos1 != std::string::npos) {
                        std::string maj = ver.substr(pos + 1, pos1);
                        std::string min = ver.substr(pos1 + 1, ver.length() - pos1 - 1);
                        name = StringTools::Trim(str_name.c_str());
                        major = std::stoi(StringTools::Trim(maj.c_str()));
                        minor = std::stoi(StringTools::Trim(min.c_str()));
                        return 0;
                    }
                }
                return -2;
            }
            return -1;
        }
    };
}