#pragma once
//========================================================================
//[File Name]:Event.h
//[Description]: A definition of Event for the tcp socket transactions.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <string>
#include <functional>


namespace Net {
    namespace Stream {
        struct Event {
            enum class eEvent_t {
                e_undef = 0,
                e_connOpened = 5,
                e_connClosed,
                e_connRecon,
                e_netErr = 10,
                e_netEof = 12
            };

            eEvent_t type{ eEvent_t::e_undef };
            std::string message;
        };

        typedef std::function<void(int, const Event&)> CBEvent;
    }
}