#pragma once
//========================================================================
//[File Name]:StreamWriter.h
//[Description]:a implemetation of stream writer, which allow write byte stream 
// to std::out, ofstream etc.
//[Author]:Nico Hu
//[Date]:2020-07-15
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <iostream>
#include <stdarg.h>
#include <mutex>
#include <time.h>


class StreamWriter {
	friend class Gargo;
public:
	static StreamWriter* Instance() {
		if (sInstance_ == nullptr) {
			sInstance_ = new StreamWriter();
		}
		return sInstance_;
	}
	~StreamWriter() { }

	template<typename T>
	void Write(T& ostr, const char* format, ...) {
		char buff[1024] = { 0 };
		va_list st;
		va_start(st, format);
		vsprintf_s(buff, format, st);
		time_t t = time(0);
		char tmp[64];
		strftime(tmp, sizeof(tmp), "[%Y-%m-%d %H:%M:%S] ", localtime(&t));
		try
		{
			/*if (!ostr.is_open()) {
				ostr.open(file_.c_str(), std::ios::out | std::ios::app);
			}*/
			{
				std::lock_guard<std::mutex> lock(mtx_);
				ostr << tmp << buff << std::endl;
				ostr.flush();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "StreamWriter write raise ex:" << e.what() << std::endl;
		}
		va_end(st);
	}

protected:
	StreamWriter() { static Gargo gargo; }
	StreamWriter(const StreamWriter&) = delete;
	StreamWriter& operator=(const StreamWriter&) = delete;
	StreamWriter(StreamWriter&&) noexcept = delete;
	StreamWriter& operator=(StreamWriter&&) noexcept = delete;

protected:
	static StreamWriter* sInstance_;
	std::mutex mtx_;

	class Gargo {
	public:
		~Gargo() {
			if (sInstance_ != nullptr) {
				delete sInstance_;
				sInstance_ = nullptr;
			}
		}
	};
};

StreamWriter* StreamWriter::sInstance_ = nullptr;