#pragma once
//========================================================================
//[File Name]:HttpReqResp.h
//[Description]: http request and response definition.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <string>
#include <vector>

#include "../asio/asio.hpp"
#include "../stream/RequestResponse.h"
#include "../stream/StreamBase.h"
#include "../com/ObjectPool.h"
#include "HttpDefines.h"
#include "HttpHsm.h"
#include "HttpTools.h"


using namespace Net;
using namespace Net::Stream;

namespace Http {
    class HttpResponse : public Response
	{
	public:
        std::string httpVer_;
        StatusType status_{ StatusType::e_undef };
        std::string reasonPhrase_;
		std::vector<HttpHeader_t> v_headers_;
        std::string entityBody_;

	public:
		std::string ToString() {
            std::ostringstream ostr;
            ostr << StatusStrings::to_string(status_);            
            for (std::size_t i = 0; i < v_headers_.size(); ++i)
            {
                HttpHeader_t& h = v_headers_[i];
                ostr << h.name;
                ostr << MiscStrings::name_value_separator;
                ostr << h.value;
                ostr << MiscStrings::crlf;
            }
            ostr << MiscStrings::crlf;
            ostr << entityBody_;
            return ostr.str();
        }
		/// Get a default reply by status.
		static HttpResponse defaultResp(StatusType status) {
            HttpResponse rep;
            rep.status_ = status;
            rep.entityBody_ = StockResps::to_string(status);
            rep.v_headers_.resize(2);
            rep.v_headers_[0].name = "Content-Length";
            rep.v_headers_[0].value = std::to_string(rep.entityBody_.size());
            rep.v_headers_[1].name = "Content-Type";
            rep.v_headers_[1].value = "text/html";
            return rep;
        }

        void Reset() {
            httpVer_.clear(); status_ = StatusType::e_undef; 
            reasonPhrase_.clear();
            v_headers_.clear();
            entityBody_.clear();
            //entityBody_.Reset();
        }
	};

	class HttpRequest : public Request {
	public:
		std::string method_;
		std::string url_;
        std::string httpVer_;
		std::vector<HttpHeader_t> v_headers_;
        std::string entityBody_;
        
    public:
        std::string ToString() {
            std::ostringstream ostr;
            ostr << method_ << " ";
            ostr << url_ << " ";
            ostr << httpVer_ << MiscStrings::crlf;
            for (std::size_t i = 0; i < v_headers_.size(); ++i)
            {
                HttpHeader_t& h = v_headers_[i];
                ostr << h.name;
                ostr << MiscStrings::name_value_separator;
                ostr << h.value;
                ostr << MiscStrings::crlf;
            }
            ostr << MiscStrings::crlf;
            ostr << entityBody_;
            return ostr.str();
        }
        void Reset() {
            method_.clear(); url_.clear(); httpVer_.clear();
            v_headers_.clear();
            entityBody_.clear();
            //entityBody_.Reset();
        }
	};

    class HttpRequestParser {
    public:
        explicit HttpRequestParser(){
            reqParserHsm_ = new HttpReqParserHsm<HttpRequest>();
            assert(reqParserHsm_);
        }
        ~HttpRequestParser() {
            if (reqParserHsm_) {
                delete reqParserHsm_; reqParserHsm_ = nullptr;
            }
        }
        void Reset() {
            reqParserHsm_->Reset();
        }       
        int Parse(const char* data, unsigned len) {
            return reqParserHsm_->Parse(data, len);
        }
        bool IsParserEntitybody() { return reqParserHsm_->IsParserEntitybody(); }
        int GetEntityBodyLen() { return HttpTools::GetEntityBodyLen(reqParserHsm_->GetResult()); }
        HttpRequest& GetResult() { return reqParserHsm_->GetResult(); }
  
    protected:
        HttpReqParserHsm<HttpRequest>* reqParserHsm_{nullptr};
    };

    class HttpResponseParser {
    public:
        explicit HttpResponseParser() {
            respParserHsm_ = new HttpRespParserHsm<HttpResponse>();
            assert(respParserHsm_);
        }
        ~HttpResponseParser() {
            if (respParserHsm_) {
                delete respParserHsm_; respParserHsm_ = nullptr;
            }
        }
        void Reset() {
            respParserHsm_->Reset();
        }
        int Parse(const char* data, unsigned len) {
            return respParserHsm_->Parse(data, len);
        }
        bool IsParserEntitybody() { return respParserHsm_->IsParserEntitybody(); }
        int GetEntityBodyLen() { return HttpTools::GetEntityBodyLen(respParserHsm_->GetResult()); }
        HttpResponse& GetResult() { return respParserHsm_->GetResult(); }

    protected:
        HttpRespParserHsm<HttpResponse>* respParserHsm_{ nullptr };
    };

	class Md5Mng {
	public:
		template<typename T>
		std::string GetMD5(T& req_resp) {
			md5_.Reset();
			int len = req_resp.entityBody_.length();
			const char* p_body = req_resp.entityBody_.c_str();
			int i{ 0 };
			int chunk_size{ 256 };
			do {
				if (i + chunk_size < len) {
					md5_.UpdateMd5(p_body + i, chunk_size);
					i += chunk_size;
				}
				else {
					md5_.UpdateMd5(p_body + i, len - i);
					md5_.Finalize();
					break;
				}
			} while (1);
			return md5_.ToString();
		}

		void UpdateMD5(const char* chunk, unsigned len) {
			int i{ 0 };
			int chunk_size{ 256 };
			do {
				if (i + chunk_size < len) {
					md5_.UpdateMd5(chunk + i, chunk_size);
					i += chunk_size;
				}
				else {
					md5_.UpdateMd5(chunk + i, len - i);
					break;
				}
			} while (1);
		}

		std::string GetMD5() {
			md5_.Finalize();
			std::string ret = md5_.ToString();
			md5_.Reset();
			return ret;
		}

	protected:
		Alg::MD5 md5_;
	};

	class HttpSendRequestMng {
		using SPRespParser = std::shared_ptr<HttpResponseParser>;
		struct RespRem {
			int id{ -1 };
			char* data{ nullptr };
			unsigned len{ 0 };
			unsigned buf_len{ 0 };
			SPRespParser parser;

			~RespRem() { SAFE_DELETE_ARRAY(data); }
			void reset() {
				id = -1; len = 0; buf_len = 0;
				SAFE_DELETE_ARRAY(data);
			}
		};

	public:
		explicit HttpSendRequestMng(NetBase<SOCKET_TYPE>* net_base) : netBase_(net_base) {
			objPool_.Init(16);
		}
		~HttpSendRequestMng() {
			for (auto itr = um_Md5Mng_.begin(); itr != um_Md5Mng_.end();) {
				if (itr->second) {
					SAFE_DELETE(itr->second);
					itr = um_Md5Mng_.erase(itr);
				}
				else
					itr++;
			}
		}

		void SendReq(int id, const char* req, unsigned len, bool keep_live, OnSendReqCB callback) {
			DoSendReq(id, req, len, keep_live, callback);
		}
		void AsyncSendReq(int id, const char* req, unsigned len, bool keep_live, OnSendReqCB callback) {
			DoAsyncSendReq(id, req, len, keep_live, callback);
		}

	protected:
		void DoSendReq(int id, const char* req, unsigned len, bool keep_live, OnSendReqCB callback) {
			HttpRequest request = CreateRequest(req, len, keep_live);

			uint8_t ec = 0;
			std::string data = request.ToString();
			int nret = netBase_->Send(id, data.c_str(), data.length(), ec);
			if (nret > 0)
				DoRecvResp(id, callback);
			else
				callback("", ec);
		}
		void DoAsyncSendReq(int id, const char* req, unsigned len, bool keep_live, OnSendReqCB callback) {
			HttpRequest request = CreateRequest(req, len, keep_live);

			std::string data = request.ToString();
			int nret = netBase_->AsyncSend(id, data.c_str(), data.length(), [this, callback](int id, size_t byte_send, uint8_t ec) {
				if (byte_send > 0)
					DoAsyncRecvResp(id, callback);//todo if the packet is divided to pieces,and there are several function calback.
				else
					callback("", ec);
				});
			if (nret < 0)
				callback("", 1);
		}
		HttpRequest CreateRequest(const char* req, unsigned len, bool keeplive) {
			HttpRequest request;
			int pos = StringTools::FindFirstOf(req, len, ' ');
			if (pos > 0) {
				char method[8]{ 0 };
				StringTools::SubStr(req, 0, pos, method);
				request.method_ = method;
				char url[64]{ 0 };
				StringTools::SubStr(req, pos + 1, len - pos, url);
				request.url_ = url;
			}
			request.httpVer_ = "HTTP/1.1";
			if (request.method_ == std::string("PUT")) {
				request.v_headers_.resize(3);
				request.v_headers_[0].name = "Content-Type";
				request.v_headers_[0].value = "text/plain";
				request.v_headers_[1].name = "Transfer-Encoding";
				request.v_headers_[1].value = "chunked";
				request.v_headers_[2].name = "Trailer";
				request.v_headers_[2].value = "Content-MD5";
			}
			else {
				request.v_headers_.resize(2);
				request.v_headers_[0].name = "Content-Length";
				request.v_headers_[0].value = "0";
				request.v_headers_[1].name = "Content-Type";
				request.v_headers_[1].value = "text/plain";
			}
			if (keeplive) {
				HttpHeader_t header;
				header.name = "Connection";
				header.value = "keep-alive";
				request.v_headers_.emplace_back(header);
			}
			else {
				HttpHeader_t header;
				header.name = "Connection";
				header.value = "close";
				request.v_headers_.emplace_back(header);
			}
			return request;
		}
		void DoRecvResp(int id, OnSendReqCB callback) {
			int nret{ 0 };
			do {
				char* r_data{ nullptr };
				uint8_t err_code = 0;
				nret = netBase_->Recieve(id, &r_data, err_code);
				if (nret > 0) {
					auto find = um_RespRem_.find(id);
					if (find != um_RespRem_.end()) {
						if (find->second.len + nret > find->second.buf_len) {
							int mem_len = find->second.len + nret;
							char* data_r = new char[mem_len];
							memcpy_s(data_r, mem_len, find->second.data, find->second.len);
							memcpy_s(data_r + find->second.len, nret, r_data, nret);
							SAFE_DELETE_ARRAY(find->second.data); find->second.data = data_r;
							find->second.len = mem_len;
							find->second.buf_len = mem_len;
						}
						else {
							memcpy_s(find->second.data + find->second.len, find->second.buf_len - find->second.len, r_data, nret);
							find->second.len += nret;
						}
						if (DoParse(id, find->second.parser, find->second.data, find->second.len, callback) != 1)
							break;
					}
					else {
						SPRespParser parser{ nullptr };
						parser = objPool_.Get<>();
						if (parser == nullptr) {
							parser = std::make_shared<HttpResponseParser>();
							assert(parser);
						}
						parser->Reset();
						if (DoParse(id, parser, r_data, nret, callback) != 1)
							break;
					}
				}
				else {
					callback("", err_code);
					break;
				}
			} while (nret > 0);
			um_RespRem_.erase(id);
		}
		void DoAsyncRecvResp(int id, OnSendReqCB callback) {
			int ret = netBase_->AsyncRecieve(id, [this, callback](int id, char* data, unsigned byte_recv, uint8_t ec) {
				auto find = um_RespRem_.find(id);
				if (find != um_RespRem_.end()) {
					if (find->second.len + byte_recv > find->second.buf_len) {
						int mem_len = find->second.len + byte_recv;
						char* data_r = new char[mem_len];
						memcpy_s(data_r, mem_len, find->second.data, find->second.len);
						memcpy_s(data_r + find->second.len, byte_recv, data, byte_recv);
						SAFE_DELETE_ARRAY(find->second.data); find->second.data = data_r;
						find->second.len = mem_len;
						find->second.buf_len = mem_len;
					}
					else {
						memcpy_s(find->second.data + find->second.len, find->second.buf_len - find->second.len, data, byte_recv);
						find->second.len += byte_recv;
					}
					int ret = DoParse(id, find->second.parser, find->second.data, find->second.len, callback);
					if (ret == 1)
						DoAsyncRecvResp(id, callback);
					else
						um_RespRem_.erase(id);
				}
				else {
					SPRespParser parser{ nullptr };
					parser = objPool_.Get<>();
					if (parser == nullptr) {
						parser = std::make_shared<HttpResponseParser>();
						assert(parser);
					}
					parser->Reset();
					if (DoParse(id, parser, data, byte_recv, callback) == 1)
						DoAsyncRecvResp(id, callback);
				}
				});
			if (ret < 0)
				callback("", 1);

		}
		int DoParse(int id, SPRespParser parser, char* data, int len, OnSendReqCB callback) {
			int ret = parser->Parse(data, len);
			if (ret == -1) {
				HttpResponse& resp = parser->GetResult();
				if (!HttpTools::GetHeaderValue(resp, "Transfer-Encoding").empty()) {
					std::string md5_l;
					auto find = um_Md5Mng_.find(id);
					if (find != um_Md5Mng_.end()) {
						find->second->UpdateMD5(resp.entityBody_.data(), resp.entityBody_.length());
						md5_l = find->second->GetMD5();
						SAFE_DELETE(find->second);
						um_Md5Mng_.erase(find);
					}
					else {
						Md5Mng md5;
						md5.UpdateMD5(resp.entityBody_.data(), resp.entityBody_.length());
						md5_l = md5.GetMD5();
					}
					
					std::string md5_r = HttpTools::GetHeaderValue(resp, "Content-MD5");
					StreamWriter::Instance()->Write(std::cout, "[HttpReqResp] local Content-MD5=%s", md5_l.c_str());
					StreamWriter::Instance()->Write(std::cout, "[HttpReqResp] recv Content-MD5=%s", md5_r.c_str());
					if (md5_l == md5_r) {
						std::string md5_verify_result = StatusStrings::to_string(StatusType::e_ok);
						callback(resp.entityBody_, 0xFF);
					}
					else {
						std::string md5_verify_result = StatusStrings::to_string(StatusType::e_undef);
						callback(resp.entityBody_, 0xFE);
					}
				}

				CheckConnection(id, resp);
				parser->Reset();
				return 0;
			}
			else if (ret >= 0) {
				int data_len = len - ret;
				
				auto find = um_RespRem_.find(id);
				if (find != um_RespRem_.end()) {
					if (data_len > 0) {
						memcpy_s(find->second.data, find->second.buf_len, data + ret, data_len);
						find->second.len = data_len;
					}
					else
						find->second.len = 0;
				}
				else {
					RespRem resp_rem;
					resp_rem.id = id;
					if (data_len > 0) {
						resp_rem.data = new char[data_len];
						memset(resp_rem.data, 0, data_len);
						memcpy_s(resp_rem.data, data_len, data + ret, data_len);
					}
					resp_rem.len = data_len;
					resp_rem.parser = parser;
					um_RespRem_.emplace(id, resp_rem);
				}
				//check if it is chunk file encoding transfered?
				std::string head_value = HttpTools::GetHeaderValue(parser->GetResult(), "Transfer-Encoding");
				if (head_value == std::string("chunked")) {
					HttpResponse& resp = parser->GetResult();
					if (!resp.entityBody_.empty()) {
						auto find = um_Md5Mng_.find(id); 
						if(find != um_Md5Mng_.end()){
							find->second->UpdateMD5(resp.entityBody_.data(), resp.entityBody_.length());
							callback(resp.entityBody_, 0);
						}
						else {
							Md5Mng* p_md5 = new Md5Mng();
							assert(p_md5);
							p_md5->UpdateMD5(resp.entityBody_.data(), resp.entityBody_.length());
							callback(resp.entityBody_, 0);
							um_Md5Mng_.emplace(id, p_md5);
						}
					}
				}
				return 1;
			}
			else {
				callback("", 1);
				//todo check connection???

				parser->Reset();
				return -1;
			}
		}
		void CheckConnection(int id, const HttpResponse& resp) {
			int ret = HttpTools::CheckConnection(resp);
			if (ret == 0)//close connection
				netBase_->Close(id);
			else if (ret == 1) {
				//todo keep-alive nothing to do?

			}
			else {
				StreamWriter::Instance()->Write(std::cout, "[HttpClient] The resp from httpserver does not contain any connection infos!");
			}
		}

	protected:
		NetBase<SOCKET_TYPE>* netBase_{ nullptr };
		DesignPattern::ObjectPool<HttpResponseParser> objPool_;
		std::unordered_map<int, RespRem> um_RespRem_;
		std::unordered_map<int, Md5Mng*> um_Md5Mng_;
	};

	class HttpChunkFileMng : public Md5Mng {
		struct ChunkRem {
			int id{ -1 };
			char* data{ nullptr };
			unsigned len{ 0 };
			unsigned buf_len{ 0 };

			~ChunkRem() { SAFE_DELETE_ARRAY(data); }
			void reset() {
				id = -1; len = 0; buf_len = 0;
				SAFE_DELETE_ARRAY(data);
			}
		};
		typedef std::function<void(char*, int)> ChunkRecvCB;

	public:
		HttpChunkFileMng(NetBase<SOCKET_TYPE>* net_base) : netBase_(net_base), 
			memStorage_{ 1024 * 4, 1024 * 8, 1024 * 16, 1024 * 32 }{
		}

		int SendChunkFile(int id, const char* chunk, unsigned len, bool b_end, uint8_t& ec) {
			return TransferEncoding(id, chunk, len, b_end, ec);
		}
		int RecvChunkFile(int id, ChunkRecvCB callback, bool b_trailer, uint8_t& ec) {
			return TransferDecoding(id, callback, b_trailer, ec);
		}
	protected:
		int TransferEncoding(int id, const char* chunk, unsigned len, bool b_end, uint8_t& ec) {
			if (chunk) {
				int PREFIX = 12, pos = 0;
				int buff_size = std::min(MaxPacketSize - PREFIX, (int)len) + PREFIX;
				char* chunk_buf = memStorage_.Alloc(buff_size);
				assert(chunk_buf);
				do {
					unsigned o_len{ 0 };
					memset(chunk_buf, 0, buff_size);
					int encode_size = std::min(buff_size - PREFIX, (int)len - pos);
					EncodingChunk(chunk + pos, encode_size, chunk_buf, buff_size, o_len);
					int nret = netBase_->Send(id, chunk_buf, o_len, ec);
					if (nret < 0) {
						memStorage_.Free(chunk_buf);
						return pos;
					}
					pos += encode_size;
				} while (pos < len);
				memStorage_.Free(chunk_buf);			
			}

			if (b_end) {
				std::string str_md5 = GetMD5();
				std::string content_md5 = "Content-MD5";
				std::ostringstream ostr;
				ostr << "0" << MiscStrings::crlf;
				ostr << content_md5 << ":";
				ostr << str_md5 << MiscStrings::crlf;
				std::string trailer_md5 = ostr.str();
				int nret = netBase_->Send(id, trailer_md5.c_str(), trailer_md5.length(), ec);
				if (nret < 0)
					return -1;  //todo return -1 means send trailer md5 failed.
				else
					StreamWriter::Instance()->Write(std::cout, "[test_httpServer] Content-MD5:%s", str_md5.c_str());
			}
			return len;
		}

		int TransferDecoding(int id, ChunkRecvCB callback, bool b_trailer, uint8_t& ec) {
			assert(callback);
			int r_len{ 0 };
			int in_ret{ -1 };
			do {
				char* data{ nullptr };
				r_len = netBase_->Recieve(id, &data, ec);
				if (r_len > 0) {
					auto find = um_ChunkRem_.find(id);
					if (find != um_ChunkRem_.end()) {
						int n_data_len = find->second.len + r_len;
						if (find->second.buf_len < n_data_len) {
							char* p_mem = new char[n_data_len];
							memcpy_s(p_mem, n_data_len, find->second.data, find->second.len);
							memcpy_s(p_mem + find->second.len, r_len, data, r_len);
							SAFE_DELETE_ARRAY(find->second.data); find->second.data = p_mem;
							find->second.buf_len = n_data_len;
							find->second.len = n_data_len;
						}
						else {
							memcpy_s(find->second.data + find->second.len, find->second.buf_len - find->second.len, data, r_len);
							find->second.len += r_len;
						}
						int ret_d = DecodingChunk(id, find->second.data, find->second.len, callback);
						if (ret_d == 1) continue;
						else { in_ret = ret_d; break; }
					}
					else {
						int ret_d = DecodingChunk(id, data, r_len, callback);
						if (ret_d == 1) continue;
						else { in_ret = ret_d; break; }
					}
				}
				else { in_ret = -3; break; }//todo -3 means receive error.
			} while (1);
			um_ChunkRem_.erase(id);

			return in_ret;
		}

		void EncodingChunk(const char* chunk, unsigned len, char* out, unsigned size, unsigned& o_len) {
			UpdateMD5(chunk, len);

			char code_len[8]{ 0 };
			sprintf(code_len, "%02x", len);

			/*std::stringstream ostr;
			ostr << code_len << MiscStrings::crlf;
			ostr << std::string(chunk, len);
			ostr << MiscStrings::crlf;
			ostr >> out;*/

			char* p_out = out;
			int len_c = strlen(code_len);
			//p_out += sprintf(p_out, "%s%s", code_len, "\r\n");
			const char* p_rn = "\r\n";
			int pos = 0;
			memcpy_s(p_out, size, code_len, len_c); pos += len_c;
			memcpy_s(p_out + pos, size - pos, p_rn, 2); pos += 2;
			memcpy_s(p_out + pos, size - pos, chunk, len); pos += len;
			memcpy_s(p_out + pos, size - pos, p_rn, 2); pos += 2;
			//sprintf(p_out + len, "%s", "\r\n");
			o_len = pos;
		}
		int DecodingChunk(int id, const char* data, unsigned len, ChunkRecvCB callback) {
			char* p_data = const_cast<char*>(data);
			char* chunk{ nullptr };
			int pos_d = 0;

			do {
				int pos_s = StringTools::FindFirstOf(p_data + pos_d, len, "\r\n", 2);
				if (pos_s > 0) {
					int pos_e = StringTools::ArrFindFirstOf(p_data + pos_d + pos_s + 2, len - pos_d - pos_s - 2, "\r\n", 2);
					if (pos_e > 0) {
						char chunked_len[8]{ 0 };
						strncat_s(chunked_len, pos_s + 1, p_data + pos_d, pos_s);
						char* code_len = StringTools::Trim(chunked_len);
						int chunk_len{ 0 };
						sscanf(code_len, "%x", &chunk_len);

						pos_d += pos_s + 2;
						if (chunk_len > 0) {
							if (chunk_len == pos_e) {
								chunk = p_data + pos_d;
								UpdateMD5(chunk, chunk_len);
								pos_d += chunk_len + 2;
								callback(chunk, chunk_len);
								continue;
							}
							else
								return -3; //todo -3 mean wrong chunk body
						}
						else if (chunk_len == 0) { //todo 0 means the last chunk file.
							uint8_t ec = 0;
							int ret = VerfyTrailer(id, p_data + pos_d, len - pos_d, ec);
							return ret;
						}
						else
							return -1;  //todo -1 means wrong encoded length.
					}
					else if (pos_e == 0)
						return -2;
					else {
						auto find = um_ChunkRem_.find(id);
						if (find == um_ChunkRem_.end()) {
							ChunkRem rem;
							rem.id = id;
							rem.data = new char[len];
							memcpy_s(rem.data, len, p_data, len);
							rem.len = len; rem.buf_len = len;
							um_ChunkRem_.emplace(id, rem);
						}
						return 1; //todo 1 means the chunk file is not completely received.
					}
				}
				else
					return -2; //todo -2 means wrong encoded chunk.
			} while (1);
		}

		int VerfyTrailer(int id, const char* data, unsigned len, uint8_t& ec) {
			int in_ret = -1;
			char trailer[128]{ 0 };
			int len_t{ 0 };
			char* buf = const_cast<char*>(data);
	
			do {
				if (len > 0) {
					memcpy_s(trailer + len_t, sizeof(trailer) - len_t, buf, len);
					len_t += len;

					int pos_e = StringTools::FindFirstOf(trailer, len_t, "\r\n", 2);
					if (pos_e > 0) {
						HttpHeader_t trailer_t;
						if (ParseTrailer(trailer, len_t, trailer_t) != 0)
							in_ret = -3;	  //todo -3 means wrong trailer.
						else {
							std::string l_md5 = GetMD5();
							if (l_md5 == trailer_t.value)
								in_ret = 0;   //todo 0 means success.
							else
								in_ret = -4;  //todo -4 means verify md5 of the file failed.
						}
						break;
					}
					else if (pos_e == 0) { in_ret = -3; break; } //todo -3 means wrong trailer.
					else { 
						len = netBase_->Recieve(id, &buf, ec);
						if (len <= 0) { in_ret = -5; break; }
					}
				}
				else {
					len = netBase_->Recieve(id, &buf, ec);
					if (len <= 0) { in_ret = -5; break; }
				}				
			} while (1);

			return in_ret;
		}
		int ParseTrailer(const char* trailer, unsigned len, HttpHeader_t& o_trailer) {
			std::string str_t(trailer, len);
			return o_trailer.FromString(str_t);
		}

	protected:
		NetBase<SOCKET_TYPE>* netBase_{ nullptr };
		std::unordered_map<int, ChunkRem> um_ChunkRem_;
		NMemoryStorage<char>	memStorage_;
	};
}