// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  RemoteControl.cpp
//
//  Created by Croc Dialer on 06/10/14.

#include "gl/Fbo.hpp"
#include "app/ViewerApp.hpp"

#include "gl/SerializerGL.hpp"
#include "RemoteControl.hpp"

using namespace crocore;
using namespace kinski;

RemoteControl::RemoteControl(io_service_t &io, const std::list<ComponentPtr> &the_list)
{
    set_components(the_list);
    m_tcp_server = net::tcp_server(io, net::tcp_server::tcp_connection_callback());
    m_udp_server = net::udp_server(io);
}

void RemoteControl::start_listen(uint16_t tcp_port, uint16_t udp_port)
{
    m_tcp_server.start_listen(tcp_port);
    m_tcp_server.set_connection_callback(std::bind(&RemoteControl::new_connection_cb,
                                                   this, std::placeholders::_1));

    add_command("request_state", [this](net::tcp_connection_ptr con, const std::vector<std::string> &)
    {
        // send the state string via tcp
        con->write(serializer::serialize(components(), PropertyIO_GL(), true));
    });

    add_command("load_settings", [this](net::tcp_connection_ptr con, const std::vector<std::string> &)
    {
        for(auto &comp : components())
        {
            comp->call_function("load_settings");
        }

        // send the state string via tcp
        con->write(serializer::serialize(components(), PropertyIO_GL(), true));
    });

    add_command("save_settings", [this](net::tcp_connection_ptr con, const std::vector<std::string> &)
    {
        for(auto &comp : components())
        {
            comp->call_function("save_settings");
        }

        // send the state string via tcp
        con->write(serializer::serialize(components(), PropertyIO_GL(), true));
    });

    add_command("log_stream", [](net::tcp_connection_ptr con,
                                 const std::vector<std::string> &the_args)
    {
        LOG_TRACE << "adding log_stream: " << con->remote_ip();
        Logger::get()->add_outstream(con);
    });

    add_command("echo", [](net::tcp_connection_ptr con, const std::vector<std::string> &the_args)
    {
        // send an echo
        if(!the_args.empty()){ con->write(the_args.front()); }
    });

    add_command("generate_snapshot", [this](net::tcp_connection_ptr con,
                                            const std::vector<std::string> &the_args)
    {
        for(auto &comp : components())
        {
            if(auto ptr = std::dynamic_pointer_cast<ViewerApp>(comp))
            {
                ptr->main_queue().post([con, ptr]()
                                       {
                                           LOG_DEBUG << "generate_snapshot ...";
                                           Stopwatch timer;
                                           timer.start();

                                           ImagePtr img;

                                           if(false)
                                           {
                                               ptr->generate_snapshot();
                                               img = gl::create_image_from_framebuffer(ptr->snapshot_fbo());
                                           }else{ img = gl::create_image_from_framebuffer(); }

                                           ptr->background_queue().post([con, img]()
                                                                        {
                                                                            LOG_DEBUG
                                                                            << "compressing snapshot data ...";
                                                                            auto compressed_data = encode_jpg(img);
                                                                            auto message = std::vector<uint8_t>(4);
                                                                            *(uint32_t *)(&message[0]) = compressed_data.size();
                                                                            message.insert(message.end(),
                                                                                           compressed_data.begin(),
                                                                                           compressed_data.end());
                                                                            con->write(message);
                                                                            LOG_DEBUG << "sending snapshot: "
                                                                                      << compressed_data.size()
                                                                                      << " bytes";
                                                                        });
                                       });


                return;
            }
        }

        // send the state string via tcp
        con->write(serializer::serialize(components(), PropertyIO_GL(), true));
    });

    m_udp_server.start_listen(udp_port);
    m_udp_server.set_receive_function([this](const std::vector<uint8_t> &the_data,
                                             const std::string &the_ip,
                                             uint16_t the_port)
                                      {
                                          LOG_TRACE_2 << "incoming udp(" << m_udp_server.listening_port() << "): "
                                                      << the_ip
                                                      << ": " << the_port << "\n"
                                                      << std::string(the_data.begin(), the_data.end());
                                          receive_cb(nullptr, the_data);
                                      });
}

uint16_t RemoteControl::tcp_port() const
{
    return m_tcp_server.listening_port();
}

uint16_t RemoteControl::udp_port() const
{
    return m_udp_server.listening_port();
}

void RemoteControl::set_components(const std::list<ComponentPtr> &the_components)
{
    m_components.assign(the_components.begin(), the_components.end());
}

void RemoteControl::stop_listen()
{
    m_tcp_server.stop_listen();
    m_udp_server.stop_listen();
}

void RemoteControl::new_connection_cb(net::tcp_connection_ptr con)
{
    LOG_TRACE << "port: " << con->port() << " -- new connection with: " << con->remote_ip()
              << " : " << con->remote_port();

    // manage existing tcp connections
    std::vector<net::tcp_connection_ptr> tmp;

    for(auto &con : m_tcp_connections)
    {
        if(con->is_open()){ tmp.push_back(con); }
    }
    m_tcp_connections = tmp;
    m_tcp_connections.push_back(con);

    con->set_tcp_receive_cb(std::bind(&RemoteControl::receive_cb, this, std::placeholders::_1,
                                      std::placeholders::_2));
}

void RemoteControl::receive_cb(net::tcp_connection_ptr rec_con,
                               const std::vector<uint8_t> &response)
{
    auto lines = split(std::string(response.begin(), response.end()), '\n');
    bool cmd_found = false;

    for(const auto &l : lines)
    {
        auto tokens = split(l, ' ');

        if(!tokens.empty())
        {
            auto iter = m_command_map.find(tokens.front());

            if(iter != m_command_map.end())
            {
                std::vector<std::string> args(++tokens.begin(), tokens.end());

                LOG_TRACE_1 << "Executing command: " << iter->first;
                cmd_found = true;

                for(const auto &a : args){ LOG_TRACE_1 << "arg: " << a; }

                // call the function object
                iter->second(rec_con, args);
            }
        }
    }
    if(!cmd_found)
    {
        try
        {
            std::string str(response.begin(), response.end());

            if(serializer::is_valid_json(str))
            {
                serializer::apply_state(components(), str, PropertyIO_GL());
            }
        }catch(std::exception &e) { LOG_ERROR << e.what(); }
    }
}

std::list<ComponentPtr>
RemoteControl::components()
{
    std::list<ComponentPtr> ret;
    std::list<ComponentWeakPtr> weak_comps;

    for(auto &weak_comp : m_components)
    {
        ComponentPtr ptr = weak_comp.lock();
        if(ptr)
        {
            weak_comps.push_back(weak_comp);
            ret.push_back(ptr);
        }
    }
    m_components = weak_comps;
    return ret;
}

void RemoteControl::add_command(const std::string &the_cmd, remote_cb_t the_cb)
{
    if(!the_cb)
    {
        the_cb = [this, the_cmd](net::tcp_connection_ptr con,
                                 const std::vector<std::string> &args)
        {
            for(auto &comp : components())
            {
                comp->call_function(the_cmd, args);
            }
        };
    }
    m_command_map[the_cmd] = the_cb;
}

void RemoteControl::remove_command(const std::string &the_cmd)
{
    auto iter = m_command_map.find(the_cmd);

    if(iter != m_command_map.end()){ m_command_map.erase(iter); }
}
