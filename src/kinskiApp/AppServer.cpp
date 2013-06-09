//
//  AppServer.cpp
//  kinskiGL
//
//  Created by Fabian on 6/8/13.
//
//

#include "AppServer.h"
#include "kinskiGL/SerializerGL.h"//temp -> use delegate instead

namespace kinski {

    void tcp_connection::start()
    {
        try
        {
            m_message = Serializer::serializeComponent(m_component.lock(), PropertyIO_GL());
        
            boost::asio::async_write(m_socket, boost::asio::buffer(m_message),
                                     boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                                 boost::asio::placeholders::error,
                                                 boost::asio::placeholders::bytes_transferred));
        }
        catch(Exception &e){LOG_ERROR<<e.what();}
    }
    
    tcp_connection::tcp_connection(boost::asio::io_service& io_service, Component::WeakPtr the_component)
    : m_socket(io_service), m_component(the_component)
    {
        LOG_DEBUG<<"Connection incoming ...";
    }
    
    void tcp_connection::handle_write(const boost::system::error_code& error,
                                      size_t bytes_transferred)
    {
        if(error)
        {
            LOG_DEBUG<<error.message();
        }
    }
    
    
    AppServer::AppServer(kinski::App::Ptr the_app) :
    m_app(the_app),
    m_acceptor(the_app->io_service(), tcp::endpoint(tcp::v4(), 11111))
    {
        start_accept();
    }
    
    void AppServer::start()
    {
        start_accept();
    }
    
    void AppServer::stop()
    {
        m_acceptor.close();
    }
    
    void AppServer::start_accept()
    {
        tcp_connection::Ptr new_connection = tcp_connection::create(m_acceptor.get_io_service(), m_app);
        
        m_acceptor.async_accept(new_connection->socket(),
                                boost::bind(&AppServer::handle_accept, this, new_connection,
                                            boost::asio::placeholders::error));
        LOG_DEBUG<<"listening on: "<<m_acceptor.local_endpoint().address() <<" port: "<<
        m_acceptor.local_endpoint().port();
    }
    
    void AppServer::handle_accept(tcp_connection::Ptr new_connection,
                                  const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }
        else
        {
            LOG_ERROR<<error.message();
            return;
        }
        start_accept();
    }
}