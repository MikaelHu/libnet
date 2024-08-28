#pragma once
//========================================================================
//[File Name]:NonCopyable.h
//[Description]: a definition fo non-copyable class.
//[Author]:Nico Hu
//[Date]:2020-07-14
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================


class NonCopyable {
protected:
	NonCopyable() = default;
	virtual ~NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};