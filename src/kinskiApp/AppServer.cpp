//
//  tcp_server.cpp
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
    
    
    tcp_server::tcp_server(kinski::App::Ptr the_app, int port) :
    m_app(the_app),
    m_acceptor_tcp(the_app->io_service())
    {
        start_accept(port);
    }
    
    std::string tcp_server::local_ip(bool ipV6)
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
    
    void tcp_server::stop()
    {
        m_acceptor_tcp.close();
    }
    
    void tcp_server::start_accept(int port)
    {
        App::Ptr app = m_app.lock();
        
        m_acceptor_tcp.close();
        m_acceptor_tcp = tcp::acceptor(app->io_service(), tcp::endpoint(tcp::v4(), port));
        tcp_connection::Ptr new_connection = tcp_connection::create(m_acceptor_tcp.get_io_service(), m_app);
        
        m_acceptor_tcp.async_accept(new_connection->socket(),
                                    boost::bind(&tcp_server::handle_accept, this, new_connection,
                                    boost::asio::placeholders::error));
        LOG_DEBUG<<"listening on port: "<<
        m_acceptor_tcp.local_endpoint().port();
    }
    
    void tcp_server::handle_accept(tcp_connection::Ptr new_connection,
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
        start_accept(m_acceptor_tcp.local_endpoint().port());
    }
    
    udp_server::udp_server(kinski::App::Ptr the_app, int port) :
    m_app(the_app),
    m_socket(the_app->io_service(), udp::endpoint(udp::v4(), port)),
    resolver_(the_app->io_service())
    {
        start_receive(port);
    }
    
    void udp_server::start_receive(int port)
    {
        if(port != m_socket.local_endpoint().port())
            m_socket.connect(udp::endpoint(udp::v4(), port));
        
        m_socket.async_receive_from(boost::asio::buffer(recv_buffer_),
                                   remote_endpoint_,
                                   boost::bind(&udp_server::handle_receive, this,
                                               boost::asio::placeholders::error,
                                               boost::asio::placeholders::bytes_transferred));
    }
    
    void udp_server::handle_receive(const boost::system::error_code& error,
                                    std::size_t bytes_transferred)
    {
        if (!error)
        {
//            boost::shared_ptr<std::string> message(new std::string("poooop"));
//            
//            m_socket.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
//                                  boost::bind(&udp_server::handle_send, this, message,
//                                              boost::asio::placeholders::error,
//                                              boost::asio::placeholders::bytes_transferred));
            
            if(App::Ptr app = m_app.lock())
            {
                app->got_message(std::string(recv_buffer_.begin(),
                                             recv_buffer_.begin() + bytes_transferred));
            }
            
            start_receive(m_socket.local_endpoint().port());
        }
        else
        {
            LOG_ERROR<<error.message();
        }
    }
    
    void udp_server::send(const std::vector<uint8_t> &bytes, const std::string &ip, int port)
    {
        try
        {
            udp::resolver::query query(udp::v4(), ip, "");
            
            udp::endpoint receiver_endpoint = *resolver_.resolve(query);
            
            m_socket.async_send_to(boost::asio::buffer(bytes), receiver_endpoint,
                                  boost::bind(&udp_server::handle_send, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        } catch (std::exception &e) { LOG_ERROR << e.what(); }
    };
    
    void udp_server::handle_send(const boost::system::error_code& error,
                                 std::size_t bytes_transferred)
    {
        if(error)
        {
            LOG_ERROR << error.message();
        }
    }
}