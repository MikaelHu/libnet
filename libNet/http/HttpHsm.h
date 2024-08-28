#pragma once
//========================================================================
//[File Name]:HttpHsm.h
//[Description]: An implemetation of HSM for the usage of parsing http protocal.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <functional>
#include <unordered_map>
#include <string>
#include "StringTools.h"
#include "HttpTools.h"


class HSM {
	//return(state return):int, param(state-input param):int
	using FuncState = std::function<int(int)>;

protected:
	virtual int OnState() = 0;
	void AddState(const char* str_state, const FuncState& func_state, bool b_init_state = false) {
		if (b_init_state)
			initState_ = str_state;
		um_States_.emplace(str_state, func_state);
	}
	void SetNextState(std::string& str_state) {
		nextState_ = str_state;
	}

protected:
	virtual void Create() = 0;

protected:
	std::unordered_map<std::string, FuncState> um_States_;
	std::string initState_;
	std::string nextState_;
};

namespace Http {
	template<typename T>
	class HttpParserHsmBase : public HSM {
	protected:
		enum eStateParam { e_start = 0, e_finish };
		enum e_ResultType { e_good = 0, e_good_chunks, e_bad, e_indeterminate, e_error };

	public:
		int Parse(const char* data, unsigned len) {
			if (data) {
				data_ = const_cast<char*>(data);
				len_ = len;
				pos_ = 0;
				int ret = OnState();
				if (ret == e_good)
					return -1;
				else if (ret == e_good_chunks)
					return pos_;  //todo if the file chunk is too big, for e: > 64k?
				else if (ret == e_indeterminate)
					return pos_; //todo bug if pos_==0 for example: entityboy
				else if (ret == e_bad)
					return -3;
				else if (ret == e_error)
					return -4;
			}
			return -5;
		}
		bool IsParserEntitybody() { return nextState_ == std::string("entitybody"); }
		T& GetResult() { return result_; }
		void Reset() {
			nextState_.clear();
			data_ = nullptr; len_ = 0; pos_ = 0;
			result_.Reset();
		}

	protected:
		void CreateComStates() {
			this->AddState("header", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos == 0) {
						this->pos_ += 2;
						int ret = this->um_States_.at("header")(this->e_finish);
						return ret;
					}
					else if (pos > 0) {
						int ret = this->um_States_.at("header-name")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "header";
						return this->e_indeterminate;
					}
				}
				else if (in == this->e_finish) {
					int ret = this->um_States_.at("entitybody")(this->e_start);
					return ret;
				}
				else
					return this->e_bad;
				});
			this->AddState("header-name", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ':');
					if (pos >= 0) {
						char* p_header_name = new char[pos + 1];
						memset(p_header_name, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_name) == 0) {
							HttpHeader_t header;
							header.name = std::string(p_header_name);
							this->result_.v_headers_.emplace_back(header);
						}
						delete[] p_header_name;
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("header-value")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "header-name";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("header-value", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos >= 0) {
						char* p_header_value = new char[pos + 1];
						memset(p_header_value, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_value) == 0) {
							char* p_header_value_t = StringTools::Trim(p_header_value);
							this->result_.v_headers_.back().value = std::string(p_header_value_t);
						}
						delete[] p_header_value;
						this->pos_ += pos + 2;
						int ret = this->um_States_.at("header")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "header-value";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("entitybody", [this](int in)->int {
				if (in == this->e_start) {
					auto transfer_encoding = HttpTools::GetHeaderValue(this->GetResult(), "Transfer-Encoding");
					if (transfer_encoding == std::string("chunked")) {
						int ret = this->um_States_.at("entitybody-chunked")(this->e_start);
						return ret;
					}
					else {
						int entity_body_len = HttpTools::GetEntityBodyLen(this->GetResult());
						if (entity_body_len > 0) {
							this->result_.entityBody_.reserve(entity_body_len);
							int len_r = this->len_ - this->pos_;
							if (len_r == entity_body_len) {
								this->result_.entityBody_ = std::string(this->data_ + this->pos_, len_r);
								return this->e_good;
							}
							else if (len_r < entity_body_len) {
								//todo if the entity body is to big? neet to cut it into pieces?
								this->nextState_ = "entitybody";
								return this->e_indeterminate;
							}
							else {
								return this->e_bad;
							}
						}
						return this->e_good;
					}
				}
				else if (in == this->e_finish) {
					std::string trailer = HttpTools::GetHeaderValue(this->GetResult(), "Trailer");
					if (trailer.empty())
						return this->e_good;
					else if (trailer == std::string("Content-MD5")) {
						int ret = this->um_States_.at("content-md5")(this->e_start);
						return ret;
					}
					else //todo other trailer?
						return this->e_good;
				}
				else
					return this->e_bad;
				});
			this->AddState("entitybody-chunked", [this](int in)->int {
				if (in == this->e_start) {
					this->result_.entityBody_.clear();

					do {
						int pos_s = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
						if (pos_s > 0) {
							char chunked_len[8]{ 0 };
							int chunk_len{ 0 };
							if (StringTools::SubStr(this->data_, this->pos_, pos_s, chunked_len) == 0) {
								char* code_len = StringTools::Trim(chunked_len);								
								sscanf(code_len, "%x", &chunk_len);
							}
							if (chunk_len > 0) {
								int offset = this->pos_ + pos_s + 2;
								if (this->len_ - offset >= chunk_len + 2) {
									this->pos_ += pos_s + 2;
									int pos_e = StringTools::ArrFindFirstOf(this->data_ + this->pos_ + chunk_len, \
										this->len_ - this->pos_ - chunk_len, "\r\n", 2);
									if (pos_e == 0) {
										this->result_.entityBody_.reserve(chunk_len);
										this->result_.entityBody_ += std::string(this->data_ + this->pos_, chunk_len);
										this->pos_ += chunk_len + 2;
										continue;
										//this->nextState_ = "entitybody-chunked";											
										//return this->e_good_chunks;
									}
									else 
										return this->e_bad;
								}
								else {
									this->nextState_ = "entitybody-chunked";
									return this->e_indeterminate;
								}
							}
							else if (chunk_len == 0) {
								this->pos_ += pos_s + 2;
								int ret = this->um_States_.at("entitybody")(this->e_finish);
								return ret;
							}
							else
								return this->e_bad;
						}
						else if (pos_s == 0)
							return this->e_bad;
						else {
							this->nextState_ = "entitybody-chunked";
							return this->e_indeterminate;
						}
					} while (1);
				}
				else
					return this->e_bad;
				});
			this->AddState("content-md5", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos > 0) {
						int ret = this->um_States_.at("content-md5-name")(this->e_start);
						return ret;
					}
					else if (pos < 0) {
						this->nextState_ = "content-md5";
						return this->e_indeterminate;
					}
					else
						return this->e_bad;
				}
				else if (in == this->e_finish) {
					//todo? check if this->pos_ < this->len_, which means there are more data left!
					return this->e_good;
				}
				else
					return this->e_bad;
				});
			this->AddState("content-md5-name", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ':');
					if (pos >= 0) {
						char* p_header_name = new char[pos + 1];
						memset(p_header_name, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_name) == 0) {
							HttpHeader_t header;
							header.name = std::string(p_header_name);
							this->result_.v_headers_.emplace_back(header);
						}
						delete[] p_header_name;
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("content-md5-value")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "content-md5-name";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("content-md5-value", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos >= 0) {
						char* p_header_value = new char[pos + 1];
						memset(p_header_value, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_value) == 0) {
							char* p_header_value_t = StringTools::Trim(p_header_value);
							this->result_.v_headers_.back().value = std::string(p_header_value_t);
						}
						delete[] p_header_value;
						this->pos_ += pos + 2;
						int ret = this->um_States_.at("content-md5")(this->e_finish);
						return ret;
					}
					else {
						this->nextState_ = "content-md5-value";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
		}
		virtual int OnState() override {
			if (!this->nextState_.empty())
				return this->um_States_.at(this->nextState_)(this->e_start);
			else if (!this->initState_.empty())
				return this->um_States_.at(this->initState_)(this->e_start);
			return e_error;
		}

	protected:
		char* data_{ nullptr };
		size_t len_{ 0 };
		size_t pos_{ 0 };
		T result_;
	};
	template<typename T>
	class HttpReqParserHsm : public HttpParserHsmBase<T> {		
	public:
		explicit HttpReqParserHsm() : HttpParserHsmBase<T>(){ Create(); }

	protected:
		virtual void Create() override {
			this->AddState("startline", [this](int in)->int {
				if (in == this->e_start) {
					this->pos_ = 0;
					int ret = this->um_States_.at("method")(this->e_start);
					return ret;
				}
				else if (in == this->e_finish) {
					int ret = this->um_States_.at("header")(this->e_start);
					return ret;
				}
				else
					return this->e_bad;
				}, true);
			this->AddState("method", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ' ');
					if (pos >= 0) {
						char method[32] = { 0 };
						if (StringTools::SubStr(this->data_, this->pos_, pos, method) == 0) {
							this->result_.method_ = std::string(method);
							if(HttpTools::CheckMethod(this->result_) != 0)
								return this->e_bad;
						}							
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("url")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "method";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("url", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ' ');
					if (pos >= 0) {
						char* p_url = new char[pos + 1];
						memset(p_url, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_url) == 0)
							this->result_.url_ = std::string(p_url);
						delete[] p_url;
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("version")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "url";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("version", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos >= 0) {
						char ver[32] = { 0 };
						if (StringTools::SubStr(this->data_, this->pos_, pos, ver) == 0)
							this->result_.httpVer_ = std::string(ver);
						this->pos_ += pos + 2;
						int ret = this->um_States_.at("startline")(this->e_finish);
						return ret;
					}
					else {
						this->nextState_ = "version";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});

			this->CreateComStates();
		}
	};
	template<typename T>
	class HttpRespParserHsm : public HttpParserHsmBase<T> {
	public:
		explicit HttpRespParserHsm() : HttpParserHsmBase<T>() { Create(); }

	protected:
		virtual void Create() override {
			this->AddState("startline", [this](int in)->int {
				if (in == this->e_start) {
					this->pos_ = 0;
					int ret = this->um_States_.at("version")(this->e_start);
					return ret;
				}
				else if (in == this->e_finish) {
					int ret = this->um_States_.at("header")(this->e_start);
					return ret;
				}
				else
					return this->e_bad;
				}, true);
			this->AddState("version", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ' ');
					if (pos >= 0) {
						char ver[32] = { 0 };
						if (StringTools::SubStr(this->data_, this->pos_, pos, ver) == 0)
							this->result_.httpVer_ = std::string(ver);
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("status")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "version";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("status", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ' ');
					if (pos >= 0) {
						char status[32] = { 0 };
						if (StringTools::SubStr(this->data_, this->pos_, pos, status) == 0)
							this->result_.status_ = (StatusType)std::stoi(std::string(status));
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("reason-phrase")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "status";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("reason-phrase", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos >= 0) {
						char reason_phrase[64] = { 0 };
						if (StringTools::SubStr(this->data_, this->pos_, pos, reason_phrase) == 0)
							this->result_.reasonPhrase_ = std::string(reason_phrase);
						this->pos_ += pos + 2;
						int ret = this->um_States_.at("startline")(this->e_finish);
						return ret;
					}
					else {
						this->nextState_ = "reason-phrase";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});

			this->CreateComStates();

			/*this->AddState("header", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos == 0) {
						this->pos_ += 2;
						int ret = this->um_States_.at("header")(this->e_finish);
						return ret;
					}
					else if (pos > 0) {
						int ret = this->um_States_.at("header-name")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "header";
						return this->e_indeterminate;
					}
				}
				else if (in == this->e_finish) {
					int ret = this->um_States_.at("entitybody")(this->e_start);
					return ret;
				}
				else
					return this->e_bad;
				});
			this->AddState("header-name", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ':');
					if (pos >= 0) {
						char* p_header_name = new char[pos + 1];
						memset(p_header_name, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_name) == 0) {
							HttpHeader_t header;
							header.name = std::string(p_header_name);
							this->result_.v_headers_.emplace_back(header);
						}
						delete[] p_header_name;
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("header-value")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "header-name";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("header-value", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos >= 0) {
						char* p_header_value = new char[pos + 1];
						memset(p_header_value, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_value) == 0) {
							char* p_header_value_t = StringTools::Trim(p_header_value);
							this->result_.v_headers_.back().value = std::string(p_header_value_t);
						}
						delete[] p_header_value;
						this->pos_ += pos + 2;
						int ret = this->um_States_.at("header")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "header-value";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("entitybody", [this](int in)->int {
				if (in == this->e_start) {
					auto transfer_encoding = HttpTools::CheckHeaders(this->GetResult(), "Transfer-Encoding");
					if (transfer_encoding == std::string("chunked")) {
						int ret = this->um_States_.at("entitybody-chunked")(this->e_start);
						return ret;
					}
					else {
						int entity_body_len = HttpTools::GetEntityBodyLen(this->GetResult());
						if (entity_body_len > 0) {
							this->result_.entityBody_.reserve(entity_body_len);
							int len_r = this->len_ - this->pos_;
							if (len_r == entity_body_len) {
								this->result_.entityBody_ = std::string(this->data_ + this->pos_, len_r);
								return this->e_good;
							}
							else if (len_r < entity_body_len) {
								//todo if the entity body is to big? neet to cut it into pieces?
								this->nextState_ = "entitybody";
								return this->e_indeterminate;
							}
							else {
								return this->e_bad;
							}
						}
						return this->e_good;
					}
				}
				else if (in == this->e_finish) {
					std::string trailer = HttpTools::CheckHeaders(this->GetResult(), "Trailer");
					if (trailer.empty())
						return this->e_good;
					else if (trailer == std::string("Content-MD5")) {
						int ret = this->um_States_.at("content-md5")(this->e_start);
						return ret;
					}
					else //todo other trailer?
						return this->e_good;
				}
				else
					return this->e_bad;
				});
			this->AddState("entitybody-chunked", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos > 0) {
						char chunked_len[8]{ 0 };
						if (StringTools::SubStr(this->data_, this->pos_, pos, chunked_len) == 0) {
							char* chunked_len_t = StringTools::Trim(chunked_len);
							int data_len{ 0 };
							sscanf(chunked_len_t, "%x", &data_len);
							if (data_len > 0) {
								this->pos_ += pos + 2;

								this->result_.entityBody_.reserve(data_len);
								int len_r = this->len_ - this->pos_;
								if (len_r == data_len) {
									this->result_.entityBody_ = std::string(this->data_ + this->pos_, len_r);
									this->pos_ += data_len;

									this->nextState_ = "entitybody-chunked"; //todo ???
									return this->e_good;  //todo it means one chunk parsed finished?
								}
								else if (len_r < data_len) {
									//todo if the chunked data is to big? neet to cut it into pieces?
									this->nextState_ = "entitybody-chunked";
									return this->e_indeterminate;
								}
								else { //todo if two or more chunks ared sent together?
									return this->e_bad;
								}
							}
							else if (data_len == 0) {
								int ret = this->um_States_.at("entitybody")(this->e_finish);
								return ret;
							}
							else
								return this->e_bad;
						}
					}
					else if (pos == 0)
						return this->e_bad;
					else {
						this->nextState_ = "entitybody-chunked";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("content-md5", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos > 0) {
						int ret = this->um_States_.at("content-md5-name")(this->e_start);
						return ret;
					}
					else if (pos < 0) {
						this->nextState_ = "content-md5";
						return this->e_indeterminate;
					}
					else
						return this->e_bad;
				}
				else if (in == this->e_finish) {
					//todo? check if this->pos_ < this->len_, which means there are more data left!
					return this->e_good;
				}
				else
					return this->e_bad;
				});
			this->AddState("content-md5-name", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, ':');
					if (pos >= 0) {
						char* p_header_name = new char[pos + 1];
						memset(p_header_name, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_name) == 0) {
							HttpHeader_t header;
							header.name = std::string(p_header_name);
							this->result_.v_headers_.emplace_back(header);
						}
						delete[] p_header_name;
						this->pos_ += pos + 1;
						int ret = this->um_States_.at("content-md5-value")(this->e_start);
						return ret;
					}
					else {
						this->nextState_ = "content-md5-name";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});
			this->AddState("content-md5-value", [this](int in)->int {
				if (in == this->e_start) {
					int pos = StringTools::FindFirstOf(this->data_ + this->pos_, this->len_ - this->pos_, "\r\n", 2);
					if (pos >= 0) {
						char* p_header_value = new char[pos + 1];
						memset(p_header_value, 0, pos + 1);
						if (StringTools::SubStr(this->data_, this->pos_, pos, p_header_value) == 0) {
							char* p_header_value_t = StringTools::Trim(p_header_value);
							this->result_.v_headers_.back().value = std::string(p_header_value_t);
						}
						delete[] p_header_value;
						this->pos_ += pos + 2;
						int ret = this->um_States_.at("content-md5")(this->e_finish);
						return ret;
					}
					else {
						this->nextState_ = "content-md5-value";
						return this->e_indeterminate;
					}
				}
				else
					return this->e_bad;
				});*/
		}
	};
}