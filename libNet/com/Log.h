#pragma once
//========================================================================
//[File Name]:Log.h
//[Description]: An implemetation of Log class.
//[Author]:Nico Hu
//[Date]:2020-07-25
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <iostream>
#include <stdarg.h>
#include <fstream>
#include <mutex>
#include <time.h>


constexpr char LOG_FILE[] = "./log.txt";

class Log {
	friend class Gargo;
public:
	enum class LOG_LEV{
		e_Info = 1,
		e_Verbos,
		e_Warning,
		e_Error
	};
public:
	static Log* Instance() {
		if (sInstance_ == nullptr) {
			sInstance_ = new Log();
		}
		return sInstance_;
	}
	~Log() {
		if (ofs_.is_open())
			ofs_.close();
	}

	void Write(LOG_LEV log_lev, const char* format, ...) {
		char lev[16] = {0};
		switch (log_lev) {
		case LOG_LEV::e_Info: 
		{
			sprintf_s(lev, "%s", "[Info] ");
			break;
		}
		case LOG_LEV::e_Verbos:
		{
			sprintf_s(lev, "%s", "[Verbos] ");
			break;
		}
		case LOG_LEV::e_Warning:
		{
			sprintf_s(lev, "%s", "[Warning] ");
			break;
		}
		case LOG_LEV::e_Error:
		{
			sprintf_s(lev, "%s", "[Error] ");
			break;
		}
		default:
			break;
		}
		char buff[1024] = { 0 };
		va_list st;
		va_start(st, format);
		vsprintf_s(buff, format, st);
		time_t t = time(0);
		char tmp[64];
		strftime(tmp, sizeof(tmp), "[%Y-%m-%d %H:%M:%S] ", localtime(&t));
		try
		{
			if (!ofs_.is_open()) {
				ofs_.open(file_.c_str(), std::ofstream::out | std::ofstream::app);
			}
			{
				std::lock_guard<std::mutex> lock(mtx_);
				ofs_ << lev << tmp << buff << std::endl;
				ofs_.flush();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Log write raise ex:" << e.what() << std::endl;
		}		
		va_end(st);
	}

protected:
	Log() : file_(LOG_FILE) {
		static Gargo gargo;
		ofs_.open(file_.c_str(), std::ofstream::out | std::ofstream::app);
	}
	Log(const Log&) = delete;
	Log& operator=(const Log&) = delete;
	Log(Log&&) noexcept = delete;
	Log& operator=(Log&&) noexcept = delete;

protected:
	static Log* sInstance_;
	std::string file_;
	std::ofstream ofs_;
	std::mutex mtx_;

	class Gargo {
	public:
		~Gargo(){
			if (sInstance_ != nullptr) {
				delete sInstance_;
				sInstance_ = nullptr;
			}
		}
	};
};

Log* Log::sInstance_ = nullptr;