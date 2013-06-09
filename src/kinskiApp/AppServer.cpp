//
//  AppServer.cpp
//  kinskiGL
//
//  Created by Fabian on 6/8/13.
//
//

#include "AppServer.h"

namespace kinski {

    void tcp_connection::start()
    {
        m_message = "PUPU UND JOCKI SAGEN IMMER WIEDER BREZELI, zuletzt um: pooop\n";
        
        boost::asio::async_write(m_socket, boost::asio::buffer(m_message),
                                 boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }
    
    tcp_connection::tcp_connection(boost::asio::io_service& io_service)
    : m_socket(io_service)
    {
        LOG_INFO<<"Connection incoming ...";
    }
    
    void tcp_connection::handle_write(const boost::system::error_code& error,
                                      size_t bytes_transferred)
    {
        
    }
    
    
    AppServer::AppServer(kinski::App::Ptr the_app)
    : m_acceptor(the_app->io_service(), tcp::endpoint(tcp::v4(), 11111))
    {
        start_accept();
    }
    
    void AppServer::start_accept()
    {
        tcp_connection::Ptr new_connection = tcp_connection::create(m_acceptor.get_io_service());
        
        m_acceptor.async_accept(new_connection->socket(),
                                boost::bind(&AppServer::handle_accept, this, new_connection,
                                            boost::asio::placeholders::error));
    }
    
    void AppServer::handle_accept(tcp_connection::Ptr new_connection,
                                  const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }
        
        start_accept();
    }
}