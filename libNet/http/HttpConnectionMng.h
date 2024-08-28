#pragma once
//========================================================================
//[File Name]:HttpConnectionMng.h
//[Description]: the implemetation of http connection management.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <string>
#include <mutex>
#include <unordered_map>
#include <functional>

#include "asio.hpp"
#include "../Comm.h"


using namespace asio;
using namespace asio::ip;
using namespace Net;

namespace Http {
    enum class ConnState { e_Active = 0, e_Idle, e_Closed };
    constexpr size_t ConnResv = 512;

    class HttpConnectionMng {
    public:
        using UM_ConnState = std::unordered_map<int, ConnState>;
        using HttpStrategy = std::function<void(UM_ConnState&)>;
        using UM_HttpStrategy = std::unordered_map<std::string, HttpStrategy>;

    public:
        explicit HttpConnectionMng(io_context& io_c, int strategy_interval = 10) : timer_(io_c),
            strategyInterval_(strategy_interval) { }
        ~HttpConnectionMng() { timer_.cancel(); }

        void UpdateConn(int id, ConnState state) {
            std::lock_guard<std::mutex> lock(mtx_);
            auto find = um_ConnState_.find(id);
            if (find != um_ConnState_.end()) {
                if (state == ConnState::e_Closed)
                    um_ConnState_.erase(find);
                else
                    find->second = state;
            }
            else
                if (state != ConnState::e_Closed)
                    um_ConnState_.emplace(id, state);

            if (um_ConnState_.size() >= MaxConnection - ConnResv) {
                for (auto var : um_Strategy_) {
                    var.second(um_ConnState_);
                }
            }
        }
        template<typename F>
        void AddStrategy(const char* name, F f, bool b_time = false) {
            if (!b_time)
                um_Strategy_.emplace(name, f);
            else
                um_StrategyTime_.emplace(name, f);
        }
        void RemoveStrategy(const char* name) {
            auto find = um_Strategy_.find(name);
            if (find != um_Strategy_.end()) {
                um_Strategy_.erase(find);
            }
            find = um_StrategyTime_.find(name);
            if (find != um_Strategy_.end()) {
                um_StrategyTime_.erase(find);
            }
        }
        void Run() {
            if (strategyInterval_ > 0) {
                timer_.expires_after(std::chrono::seconds(strategyInterval_));
                timer_.async_wait(
                    [this](const std::error_code& ec)
                    {
                        if (ec != asio::error::operation_aborted) {
                            {
                                std::lock_guard<std::mutex> lock(mtx_);
                                for (auto var : um_StrategyTime_) {
                                    var.second(um_ConnState_);
                                }
                            }
                            Run();
                        }
                    });
            }
        }
        void Stop() {
            if (strategyInterval_ > 0)
                timer_.cancel();
            um_ConnState_.clear();
        }

    protected:
        std::mutex mtx_;
        int strategyInterval_{ 0 }; //unit: second
        asio::steady_timer timer_;
        UM_ConnState um_ConnState_;
        UM_HttpStrategy um_Strategy_;
        UM_HttpStrategy um_StrategyTime_;
    };
}