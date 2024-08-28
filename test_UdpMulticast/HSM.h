#pragma once
#include <functional>
#include <unordered_map>
#include <string>


class HSM {
	//return(state return):int, param(uper state output):int
	typedef std::function<int(int)> FuncState;

protected:
	virtual int OnState() = 0;
	void AddState(std::string& str_state, const FuncState& func_state, bool b_init_state = false) {
		if (b_init_state)
			initState_ = str_state;
		um_States_.emplace(str_state, func_state);
	}
	void SetNextState(std::string& str_state) {
		nextState_ = str_state;
	}

protected:
	std::unordered_map<std::string, FuncState> um_States_;
	std::string initState_;
	std::string nextState_;
};

template<typename T>
class HttpReqParserHsm : public HSM {
	enum eStateParam { e_start = 0, e_finish};
	enum e_ResultType { e_good = 0, e_bad, e_indeterminate, e_error };

public:
	explicit HttpReqParserHsm() {
		Create();
	}

	int Parse(const char* data, unsigned len) {
		if (data) {
			data_ = data;
			len_ = len;
			int ret = OnState();
			if (ret == e_good)
				return 0;
			else if (ret == e_indeterminate)
				return pos_;
			else if (ret == e_bad)
				return -1;
			else if (ret == e_error)
				return -2;
		}
		return -3;
	}

	T& GetResult() { return result_; }

protected:
	void Create() {
		this->AddState("startline", [this](eStateParam in)->int {
			if (in == e_start) {
				if (!IsChar(input) || IsCtl(input) || IsTspecial(input))
					return e_bad;
				pos_ = 0;
				int ret = this->um_States_.at("method")(e_start);
				return ret;
			}
			else if (in == e_finish) {
				int ret = this->um_States_.at("header")(e_start);
				return ret;
			}
			else
				return e_bad;
			});
		this->AddState("method", [this](int in) {
			if (in == e_start) {
				int pos = FindFirstOf(data_ + pos_, len_ - pos_, ' ');
				if (pos >= 0) {
					char method[32] = { 0 };
					if (SubStr(data_, pos_, pos, method) == 0)
						T.method_ = std::string(method);
					pos_ += pos + 1;
					int ret = this->um_States_.at("uri")(e_start);
					return ret;
				}
				else {
					this->nextState_ = "method";
					return e_indeterminate;
				}
			}
			else
				return e_bad;
			});
		this->AddState("uri", [this](int in) {
			if (in == e_start) {
				int pos = FindFirstOf(data_ + pos_, len_ - pos_, ' ');
				if (pos >= 0) {
					char* p_uri = new char[pos + 1];
					memset(p_uri, 0, pos + 1);
					if (SubStr(data_, pos_, pos, p_uri) == 0)
						T.uri_ = std::string(p_uri);
					delete[] p_uri;
					pos_ += pos + 1;
					int ret = this->um_States_.at("version")(e_start);
					return ret;
				}
				else {
					this->nextState_ = "uri";
					return e_indeterminate;
				}
			}
			else
				return e_bad;
			});
		this->AddState("version", [this](int in) {
			if (in == e_start) {
				int pos = FindFirstOf(data_ + pos_, len_ - pos_, "\r\n", 2);
				if (pos >= 0) {
					char ver[32] = { 0 };
					memset(p_url, 0, pos + 1);
					if (SubStr(data_, pos_, pos, ver) == 0)
						T.httpVer_ = std::string(ver);
					pos_ += pos + 2;
					int ret = this->um_States_.at("startline")(e_finish);
					return ret;
				}
				else {
					this->nextState_ = "version";
					return e_indeterminate;
				}
			}
			else
				return e_bad;
			});
		this->AddState("header", [this](int in) {
			if (in == e_start) {
				int pos = FindFirstOf(data_ + pos_, len_ - pos_, '\r\n');
				if (pos == 0) {
					pos_ += 2;
					int ret = this->um_States_.at("header")(e_finish);
					return ret;
				}
				else if (pos > 0) {
					int ret = this->um_States_.at("header-name")(e_start);
					return ret;
				}
				else {
					this->nextState_ = "header";
					return e_indeterminate;
				}
			}
			else if (in == e_finish) {
				int ret = this->um_States_.at("entitybody")(e_start);
				return ret;
			}
			else
				return e_bad;
			});
		this->AddState("header-name", [this](int in) {
			if (in == e_start) {
				int pos = FindFirstOf(data_ + pos_, len_ - pos_, ':');
				if (pos >= 0) {
					char* p_header_name = new char[pos + 1];
					memset(p_url, 0, pos + 1);
					if (SubStr(data_, pos_, pos, p_header_name) == 0) {
						Header header;
						header.name = std::string(p_header_name);
						T.v_headers_.emplace_back(header);
					}
					delete[] p_header_name;
					pos_ += pos + 1;
					int ret = this->um_States_.at("header-value")(e_start);
					return ret;
				}
				else {
					this->nextState_ = "header-name";
					return e_indeterminate;
				}
			}
			else
				return e_bad;
			});
		this->AddState("header-value", [this](int in) {
			if (in == e_start) {
				int pos = FindFirstOf(data_ + pos_, len_ - pos_, '\r\n');
				if (pos >= 0) {
					char* p_header_value = new char[pos + 1];
					memset(p_url, 0, pos + 1);
					if (SubStr(data_, pos_, pos, p_header_value) == 0) {
						T.v_headers_.back().value = std::string(p_header_value);
					}
					delete[] p_header_value;
					pos_ += pos + 2;
					int ret = this->um_States_.at("header")(e_finish);
					return ret;
				}
				else {
					this->nextState_ = "header-value";
					return e_indeterminate;
				}
			}
			else
				return e_bad;
			});
		this->AddState("entitybody", [this](int in) {//todo content-length=???
			if (in == e_start) {
				size_t body_len = len_ - pos_;
				if (body_len > 0) {
					T.entityBody_ = new char[body_len];
					memset(T.entityBody_, 0, body_len);
					memcpy_s(T.entityBody_, body_len, data_ + pos_, body_len);
				}
				return e_good;
			}
			else
				return e_bad;
			});
	}
	virtual int OnState() override {
		if (!this->nextState_.empty())
			return this->um_States_.at(this->nextState_)();
		else if (!this->initState_.empty())
			return this->um_States_.at(this->initState_)();

		return e_error;
	}
	int CheckMethod() {
		const char* method = result_.method_.c_str();
		size_t len = result_.method_.length();
		for (int i = 0; i < len; i++) {
			if (!IsChar(*(method + i)) || IsCtl(*(method + i)) || IsTspecial(*(method + i))) {
				return -1;
			}
		}
		return 0;
	}
	int CheckUri(){
		const char* uri = result_.uri_.c_str();
		size_t len = result_.uri_.length();
		for (int i = 0; i < len; i++) {
			if (IsCtl(*(uri + i))) {
				return -1;
			}
		}
		return 0;
	}
	int CheckVersion() {
		const char* version = result_.version_.c_str();
		size_t len = result_.version_.length();
		if (FindFirstOf(version, 0, "HTTP/", 5) == 0)
			return 0;
		return -1;
	}
	int CheckHeaders() {
		//todo
	}
	int CheckEntityBody() {
		int len = GetEntityBodyLen();
		if (len < 0)
			return -2;
		if (len == len_ - pos_)
			return 0;
		else if (len < len_ - pos_)
			return len;
		else
			return -1;
	}
	int GetEntityBodyLen() {
		auto find = std::find(result_.v_headers_.begin(), result_.v_headers_.begin(), [](const Header& header) {
			return Header.name == "Content-length";
			});
		if (find != result_.v_headers_.end()) {
			size_t body_len = std::stoi(find->value);
			return body_len;
		}
		return -1;
	}

protected:
	char* data_{ nullptr };
	size_t len_{ 0 };
	size_t pos_{ 0 };
	T result_;
};