//
//  AppServer.h
//  kinskiGL
//
//  Created by Fabian on 6/8/13.
//
//

#ifndef __kinskiGL__AppServer__
#define __kinskiGL__AppServer__

#include "App.h"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace kinski
{
    class AppServer;
    class tcp_connection;
    class response_delegate;
    
    class tcp_connection : public std::enable_shared_from_this<tcp_connection>
    {
    public:
        typedef std::shared_ptr<tcp_connection> Ptr;
        
        static Ptr create(boost::asio::io_service& io_service, kinski::Component::WeakPtr the_component)
        {
            return Ptr(new tcp_connection(io_service, the_component));
        }
        
        tcp::socket& socket(){ return m_socket; }
        
        void start();
        
//        void send();
//        void receive();
        
    private:
        tcp_connection(boost::asio::io_service& io_service, kinski::Component::WeakPtr the_component);
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
        
        tcp::socket m_socket;
        std::string m_message;
        std::vector<uint8_t> m_receive_buffer;
        kinski::Component::WeakPtr m_component;
    };
    
    class AppServer
    {
    public:
        AppServer(kinski::App::Ptr the_app, int port = 11111);
        
        void start();
        void stop();
        
        static std::string local_ip(bool ipV6 = false);
        
        void start_accept_tcp(int port);
        void start_accept_udp(int port);
        
    private:
        
        void handle_accept_tcp(tcp_connection::Ptr new_connection,
                               const boost::system::error_code& error);
        
        kinski::App::WeakPtr m_app;
        tcp::acceptor m_acceptor_tcp;
    };
}// namespace

#endif /* defined(__kinskiGL__AppServer__) */
