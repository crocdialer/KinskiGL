//
//  RemoteControl.cpp
//  kinskiGL
//
//  Created by Croc Dialer on 06/10/14.
//
//

#include "RemoteControl.h"
#include "gl/SerializerGL.h"
#include "app/ViewerApp.h"
//#include "cv/TextureIO.h"

using namespace kinski;

RemoteControl::RemoteControl(boost::asio::io_service &io, const std::list<Component::Ptr> &the_list)
{
    m_components.assign(the_list.begin(), the_list.end());
    m_tcp_server = net::tcp_server(io, net::tcp_server::tcp_connection_callback());
}

void RemoteControl::start_listen(uint16_t port)
{
    m_tcp_server.start_listen(port);
    m_tcp_server.set_connection_callback(std::bind(&RemoteControl::new_connection_cb,
                                                   this, std::placeholders::_1));
    
    add_command("request_state", [this](net::tcp_connection_ptr con)
    {
        // send the state string via tcp
        con->send(Serializer::serializeComponents(lock_components(), PropertyIO_GL()));
    });
    
    add_command("load_settings", [this](net::tcp_connection_ptr con)
    {
        for(auto &comp : lock_components())
        {
            comp->call_function("load_settings");
        }
        
        // send the state string via tcp
        con->send(Serializer::serializeComponents(lock_components(), PropertyIO_GL()));
    });
    
    add_command("save_settings", [this](net::tcp_connection_ptr con)
    {
        for(auto &comp : lock_components())
        {
            comp->call_function("save_settings");
        }
        
        // send the state string via tcp
        con->send(Serializer::serializeComponents(lock_components(), PropertyIO_GL()));
    });
    
    add_command("generate_snapshot", [this](net::tcp_connection_ptr con)
    {
        std::vector<uint8_t> bytes;
        
        LOG_WARNING << "command 'generate_snapshot': not implemented";
        
        for(auto &comp : lock_components())
        {
            comp->call_function("generate_snapshot");
            if(auto ptr = std::dynamic_pointer_cast<ViewerApp>(comp))
            {
//                std::vector<uint8_t> jpg_bytes;
//                gl::TextureIO::encode_jpg(ptr->snapshot_texture(), jpg_bytes);
//                con->send(jpg_bytes);
//                return;
            }
        }
        
        // send the state string via tcp
        con->send(Serializer::serializeComponents(lock_components(), PropertyIO_GL()));
    });
}

void RemoteControl::stop_listen()
{
    m_tcp_server.stop_listen();
}

void RemoteControl::new_connection_cb(net::tcp_connection_ptr con)
{
    LOG_TRACE << "port: "<< con->port()<<" -- new connection with: " << con->remote_ip()
    << " : " << con->remote_port();
    
    // manage existing tcp connections
    std::vector<net::tcp_connection_ptr> tmp;
    
    for (auto &con : m_tcp_connections)
    {
        if(con->is_open()){ tmp.push_back(con); }
    }
    m_tcp_connections = tmp;
    m_tcp_connections.push_back(con);
    
    con->set_receive_function(std::bind(&RemoteControl::receive_cb, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
}

void RemoteControl::receive_cb(net::tcp_connection_ptr rec_con,
                               const std::vector<uint8_t>& response)
{
    auto tokens = split(std::string(response.begin(), response.end()));
//    for(auto &t : tokens){ t = std::remo}
    
    if(!tokens.empty())
    {
        auto iter = m_command_map.find(tokens.front());
        
        if(iter != m_command_map.end())
        {
            LOG_DEBUG << "Executing command: " << iter->first;
            
            // call the function object
            iter->second(rec_con);
        }
        else
        {
            try
            {
                Serializer::applyStateToComponents(lock_components(),
                                                   string(response.begin(),
                                                          response.end()),
                                                   PropertyIO_GL());
            } catch (std::exception &e){ LOG_ERROR << e.what(); }
        }
    }
}

std::list<Component::Ptr>
RemoteControl::lock_components()
{
    std::list<Component::Ptr> ret;

    for(auto &weak_comp : m_components)
    {
        Component::Ptr ptr = weak_comp.lock();
        if(ptr){ ret.push_back(ptr); }
//        else{ m_components.remove(weak_comp); }
    }
    return ret;
}

void RemoteControl::add_command(const std::string &the_cmd,
                                std::function<void(net::tcp_connection_ptr)> the_action)
{
    m_command_map[the_cmd] = the_action;
}

void RemoteControl::remove_command(const std::string &the_cmd)
{
    auto iter = m_command_map.find(the_cmd);
    
    if(iter != m_command_map.end()){ m_command_map.erase(iter); }
}
