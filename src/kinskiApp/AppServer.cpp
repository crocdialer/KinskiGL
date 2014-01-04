//
//  AppServer.cpp
//  kinskiGL
//
//  Created by Fabian on 6/8/13.
//
//

#include "AppServer.h"
#include "kinskiGL/SerializerGL.h"//temp -> use delegate instead

using namespace std;
using namespace boost::asio::ip;

namespace kinski {

    void tcp_connection::start()
    {
        try
        {
            m_message = Serializer::serializeComponent(m_component.lock(), PropertyIO_GL());
            
            boost::asio::async_read(m_socket, boost::asio::buffer(m_receive_buffer),
                                     boost::bind(&tcp_connection::handle_read, shared_from_this(),
                                                 boost::asio::placeholders::error,
                                                 boost::asio::placeholders::bytes_transferred));
            
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
        m_receive_buffer.resize(2<<16);
    }
    
    void tcp_connection::handle_read(const boost::system::error_code& error,
                                     size_t bytes_transferred)
    {
        if(error)
        {
            if(error == boost::asio::error::eof)
            {

            }else
            {
                LOG_DEBUG<<error.message();
                return;
            }
        }
        
        std::string message = std::string(m_receive_buffer.begin(),
                                          m_receive_buffer.begin() + bytes_transferred);
        
        Component::Ptr c = m_component.lock();
        if(App::Ptr app = std::dynamic_pointer_cast<App>(c))
        {
            app->got_message(message);
        }
        
        try
        {
            //Serializer::applyStateToComponent(m_component.lock(), message, PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_ERROR<<e.what();
        }
    }
    
    void tcp_connection::handle_write(const boost::system::error_code& error,
                                      size_t bytes_transferred)
    {
        if(error)
        {
            LOG_DEBUG<<error.message();
        }
    }
    
    
    AppServer::AppServer(kinski::App::Ptr the_app, int port) :
    m_app(the_app),
    m_acceptor_tcp(the_app->io_service())
    {
        start_accept_tcp(port);
    }
    
    std::string AppServer::local_ip(bool ipV6)
    {
        std::string ret = "unknown_ip";
        boost::asio::io_service io;
        boost::asio::ip::tcp::resolver resolver(io);
        boost::asio::ip::tcp::resolver::query query(ipV6 ? tcp::v6() : tcp::v4(), host_name(), "");
        boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query), end;
        
        for (; it != end; ++it)
        {
            const boost::asio::ip::tcp::endpoint &endpoint = *it;
            ret = endpoint.address().to_string();
        }
        return ret;
    }
    
    void AppServer::start()
    {
        //start_accept();
    }
    
    void AppServer::stop()
    {
        m_acceptor_tcp.close();
    }
    
    void AppServer::start_accept_tcp(int port)
    {
        App::Ptr app = m_app.lock();
        
        m_acceptor_tcp.close();
        m_acceptor_tcp = tcp::acceptor(app->io_service(), tcp::endpoint(tcp::v4(), port));
        tcp_connection::Ptr new_connection = tcp_connection::create(m_acceptor_tcp.get_io_service(), m_app);
        
        m_acceptor_tcp.async_accept(new_connection->socket(),
                                    boost::bind(&AppServer::handle_accept_tcp, this, new_connection,
                                    boost::asio::placeholders::error));
        LOG_DEBUG<<"listening on port: "<<
        m_acceptor_tcp.local_endpoint().port();
    }
    
    void AppServer::handle_accept_tcp(tcp_connection::Ptr new_connection,
                                      const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }
        else
        {
            if(!boost::asio::error::connection_aborted)
                LOG_ERROR<<error.message();
            
            return;
        }
        start_accept_tcp(m_acceptor_tcp.local_endpoint().port());
    }
    
    void AppServer::start_accept_udp(int port)
    {
    
    }
}