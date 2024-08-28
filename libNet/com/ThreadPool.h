#pragma once
//========================================================================
//[File Name]:ThreadPool.h
//[Description]:a implemetation of Thread Pool.
//[Author]:Nico Hu
//[Date]:2020-07-15
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================

#include <list>
#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include "SyncTaskQueue.h"

namespace Thread {
	constexpr int16_t MaxTaskCount = 1000;

	class ThreadPool {
	public:
		using Task = std::function<void()>;
		/*template<class R, typename... Args> //todo
		using Task = std::function<R(Args...)>;*/
		explicit ThreadPool() : taskQueue_(MaxTaskCount) {
		}
		~ThreadPool() {
			Stop();
		}

		void Start(int num_threads = std::thread::hardware_concurrency()) {
			bExit_ = false;
			for (int i = 0; i < num_threads; ++i) {
				threadGroup_.emplace_back(std::make_shared<std::thread>([this]() {
					while (!bExit_) {
						std::list<Task> list;
						taskQueue_.Take(list);

						for (auto& task : list) {
							if (bExit_) return;
							task();
						}
					}
					}));
			}
		}
		void Stop() {
			std::call_once(flag_, [this] { StopThreadGroup(); });
		}
		template<class F, class... Args, class = typename std::enable_if<!std::is_member_function_pointer<F>::value>::type>
		void AddTask(F&& f, Args&&... args) {
			Task t = [=] {return f(args...); };
			taskQueue_.Put(t);
		}
		template<class C, class... DArgs, class P, class... Args>
		void AddTask(void(C::* f)(DArgs...) const, P&& p, Args&&... args) {
			Task t = [=] {return (*p.*f)(args...); };
			taskQueue_.Put(t);
		}
		template<class C, class... DArgs, class P, class... Args>
		void AddTask(void(C::* f)(DArgs...), P&& p, Args&&... args) {
			Task t = [=] {return (*p.*f)(args...); };
			taskQueue_.Put(t);
		}
		/*void AddTask(Task&& t) {
			taskQueue_.Put(std::forward<Task>(t));
		}
		void AddTask(const Task& t) {
			taskQueue_.Put(t);
		}*/

	protected:
		void StopThreadGroup() {
			taskQueue_.Stop();
			bExit_ = true;
			for (auto thread : threadGroup_) {
				if (thread && thread->joinable())
					thread->join();
			}
			threadGroup_.clear();
		}
	protected:
		std::list<std::shared_ptr<std::thread>> threadGroup_;
		SyncTaskQueue<Task>		taskQueue_;
		std::atomic_bool		bExit_{ false };
		std::once_flag			flag_;
	};
}