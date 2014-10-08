//
//  RemoteControl.cpp
//  kinskiGL
//
//  Created by Croc Dialer on 06/10/14.
//
//

#include "RemoteControl.h"
#include "gl/SerializerGL.h"

using namespace kinski;

RemoteControl::RemoteControl(boost::asio::io_service &io, const std::list<Component::Ptr> &the_list,
                             uint16_t the_port)
{
    m_components.assign(the_list.begin(), the_list.end());
    m_tcp_server = net::tcp_server(io, the_port, net::tcp_server::tcp_connection_callback());
}

void RemoteControl::start_listen(uint16_t port)
{
    m_tcp_server.start_listen(port);
    m_tcp_server.set_connection_callback(std::bind(&RemoteControl::new_connection_cb,
                                                   this, std::placeholders::_1));
}

void RemoteControl::stop_listen()
{
    m_tcp_server.stop_listen();
}

void RemoteControl::new_connection_cb(net::tcp_connection_ptr con)
{
    LOG_DEBUG << "port: "<< con->port()<<" -- new connection with: " << con->remote_ip()
    << " : " << con->remote_port();
    
    // lock our components and get a list
    std::list<Component::Ptr> comp_list = lock_components();
    
    con->send(Serializer::serializeComponents(comp_list, PropertyIO_GL()));
    
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
    try
    {
        Serializer::applyStateToComponents(lock_components(),
                                           string(response.begin(),
                                                  response.end()),
                                           PropertyIO_GL());
    } catch (std::exception &e){ LOG_ERROR << e.what(); }
}

std::list<Component::Ptr>
RemoteControl::lock_components()
{
    std::list<Component::Ptr> ret;

    for(auto &weak_comp : m_components)
    {
        Component::Ptr ptr = weak_comp.lock();
        if(ptr){ ret.push_back(ptr); }
    }
    return ret;
}
