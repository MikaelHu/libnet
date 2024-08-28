#pragma once
//========================================================================
//[File Name]:Timer.h
//[Description]:a implemetation of Timer.
//[Author]:Nico Hu
//[Date]:2020-07-15
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <chrono>

using namespace std::chrono;
class Timer
{
public:
	explicit Timer() :tmBegin_(high_resolution_clock::now()) {}
	void reset() { tmBegin_ = high_resolution_clock::now(); }

	//default: Millisecond
	template<typename Duration = milliseconds>
	int64_t elapsed() const{
		return duration_cast<Duration>(high_resolution_clock::now() - tmBegin_).count();
	}
	//Microsecond
	int64_t elapsed_micro() const{
		return elapsed<microseconds>();
	}
	//Nanosecond
	int64_t elapsed_nano() const{
		return elapsed<nanoseconds>();
	}
	//Second
	int64_t elapsed_seconds() const{
		return elapsed<seconds>();
	}
	//Minute
	int64_t elapsed_minutes() const{
		return elapsed<minutes>();
	}
	//Hour
	int64_t elasped_hours() const{
		return elapsed<hours>();
	}

protected:
	time_point<high_resolution_clock> tmBegin_;
};