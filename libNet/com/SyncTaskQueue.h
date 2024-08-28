#pragma 
//========================================================================
//[File Name]:SyncTaskQueue.h
//[Description]:a implemetation of sync task queue for ThreadPool.
//[Author]:Nico Hu
//[Date]:2020-07-15
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================

#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <stdint.h>

namespace Thread {
	template<typename T>
	class SyncTaskQueue
	{
	public:
		explicit SyncTaskQueue(int16_t max_size, bool need_stop = false) : maxSize_{ max_size }, needStop_{ need_stop }{
		}

		void Put(const T& t) {
			Add(t);
		}
		void Put(T&& t) {
			Add(std::forward<T>(t));
		}
		void Take(std::list<T>& list) {
			std::unique_lock<std::mutex> locker(mtx_);
			notEmpty_.wait(locker, [this] {return needStop_ || NotEmpty(); });

			if (needStop_)
				return;

			list = std::move(taskQueue_);
			notFull_.notify_one();
		}
		void Take(T& t) {
			std::unique_lock<std::mutex> locker(mtx_);
			notEmpty_.wait(locker, [this] {return needStop_ || NotEmpty(); });

			if (needStop_)
				return;

			t = taskQueue_.front();
			taskQueue_.pop_front();
			notFull_.notify_one();
		}
		void Stop() {
			{
				std::lock_guard<std::mutex> locker(mtx_);
				needStop_ = true;
			}

			notFull_.notify_all();
			notEmpty_.notify_all();
		}
		bool Empty() {
			std::lock_guard<std::mutex> locker(mtx_);
			return taskQueue_.empty();
		}
		bool Full() {
			std::lock_guard<std::mutex> locker(mtx_);
			return taskQueue_.size() == maxSize_;
		}
		size_t Size() {
			std::lock_guard<std::mutex> locker(mtx_);
			return taskQueue_.size();
		}
		int Count() {
			return taskQueue_.size();
		}

	protected:
		bool NotFull() const {
			bool full = taskQueue_.size() >= maxSize_;
			if (full)
				std::cout << "ThreadPool task queue is full, wait..." << std::endl;

			return !full;
		}
		bool NotEmpty() const {
			bool empty = taskQueue_.empty();
			if (empty)
				std::cout << "ThreadPool task queue is empty, wait...,async thread id£º" << std::this_thread::get_id() << std::endl;

			return !empty;
		}
		template<typename F>
		void Add(F&& t) {
			std::unique_lock<std::mutex> locker(mtx_);
			notFull_.wait(locker, [this] {return needStop_ || NotFull(); });

			if (needStop_)
				return;

			taskQueue_.emplace_back(std::forward<F>(t));
			notEmpty_.notify_one();
		}
	protected:
		int16_t					maxSize_{ 0 };
		bool					needStop_{ false };
		std::list<T>			taskQueue_;
		std::mutex				mtx_;
		std::condition_variable notEmpty_;
		std::condition_variable notFull_;
	};
}