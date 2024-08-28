#pragma once
//========================================================================
//[File Name]:SessionIdGen.h
//[Description]: a session id generator.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <mutex>
#include <list>
#include <numeric>
#include "../Comm.h"


namespace Net {
	namespace Stream {
		template<typename T>
		class SessionIdGen {
			friend class Gargo;
		public:
			static SessionIdGen* Instance() {
				if (sInstance_ == nullptr) {
					sInstance_ = new SessionIdGen<T>(MaxConnNum);
				}
				return sInstance_;
			}
			~SessionIdGen() {}

			T GetId() {
				std::lock_guard<std::mutex> lock(mtx_);
				T id = idPool_.front();
				idPool_.pop_front();
				return id;
			}
			void ReleaseId(const T& id) {
				std::lock_guard<std::mutex> lock(mtx_);
				idPool_.emplace_back(id);
			}

		protected:
			SessionIdGen(const T& max_num) : idPool_(max_num) {
				static Gargo gargo;
				idPool_.resize(max_num);
				std::iota(idPool_.begin(), idPool_.end(), 1);
			}
			SessionIdGen(const SessionIdGen&) = delete;
			SessionIdGen& operator=(const SessionIdGen&) = delete;
			SessionIdGen(SessionIdGen&&) noexcept = delete;
			SessionIdGen& operator=(SessionIdGen&&) noexcept = delete;

		protected:
			static SessionIdGen* sInstance_;
			std::mutex mtx_;
			std::list<T> idPool_;

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
		template<typename T>
		SessionIdGen<T>* SessionIdGen<T>::sInstance_ = nullptr;
	}
}