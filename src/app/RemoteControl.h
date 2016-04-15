// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  RemoteControl.hpp
//
//  Created by Croc Dialer on 06/10/14.

#pragma once

#include "core/core.hpp"
#include "core/Component.hpp"
#include "core/networking.hpp"

namespace kinski
{
    class RemoteControl;
    typedef std::unique_ptr<RemoteControl> RemoteControlPtr;
    
    typedef std::function<void(net::tcp_connection_ptr, const std::vector<std::string>&)> RCAction;
    typedef std::map<std::string, RCAction> CommandMap;
    
    class RemoteControl
    {
    public:
        
        RemoteControl(){};
        RemoteControl(boost::asio::io_service &io, const std::list<Component::Ptr> &the_list);
        
        void start_listen(uint16_t port = 33333);
        void stop_listen();
        uint16_t listening_port() const;
        
        void add_command(const std::string &the_cmd);
        
        void add_command(const std::string &the_cmd, RCAction the_action);
        void remove_command(const std::string &the_cmd);
        
        const CommandMap& command_map() const { return m_command_map; }
        CommandMap& command_map() { return m_command_map; }
        
        std::list<Component::Ptr> components();
        void set_components(const std::list<Component::Ptr>& the_components);
        
    private:
        
        void new_connection_cb(net::tcp_connection_ptr con);
        void receive_cb(net::tcp_connection_ptr rec_con,
                        const std::vector<uint8_t>& response);
        
        
        //!
        CommandMap m_command_map;
        
        //!
        net::tcp_server m_tcp_server;
        
        //!
        std::list<Component::WeakPtr> m_components;
        
        //!
        std::vector<net::tcp_connection_ptr> m_tcp_connections;
    };
}