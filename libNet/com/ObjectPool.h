#pragma once
//========================================================================
//[File Name]:ObjectPool.h
//[Description]:a template implemetation of ObjectPool design pattern.
//[Author]:Nico Hu
//[Date]:2020-07-15
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <map>
#include "NonCopyable.h"


namespace DesignPattern {
#define THREAD_SAFE

	constexpr int MaxObjNum = 32;

	template<typename T>
	class ObjectPool :public NonCopyable {
		template<typename... Args>
		using Constructor = std::function<std::shared_ptr<T>(Args...)>;
	public:
		template<typename... Args>
		void Init(size_t num, Args&&... args) {
			if (num <= 0 || num > MaxObjNum)
				throw std::logic_error("[ObjectPool] object num out of range.");

			auto c_name = typeid(Constructor<Args...>).name();
			for (size_t i = 0; i < num; i++) {
				objMap_.emplace(c_name, std::shared_ptr<T>(new T(std::forward<Args>(args)...), [this, c_name](T* p) {
					objMap_.emplace(c_name, std::shared_ptr<T>(p));
					}));
			}
		}
		template<typename... Args>
		std::shared_ptr<T> Get() {
#ifdef THREAD_SAFE
			std::lock_guard<std::mutex> lock(mtx_);
#endif //THREAD_SAFE
			std::string c_name = typeid(Constructor<Args...>).name();
			auto range = objMap_.equal_range(c_name);
			for (auto it = range.first; it != range.second; ++it) {
				auto ptr = it->second;
				objMap_.erase(it);
				return ptr;
			}
			return nullptr;
		}

	protected:
		std::multimap<std::string, std::shared_ptr<T>> objMap_;
#ifdef THREAD_SAFE
		std::mutex mtx_;
#endif //THREAD_SAFE
	};
}