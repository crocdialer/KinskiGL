//
//  tcp_server.h
//  kinskiGL
//
//  Created by Fabian on 6/8/13.
//
//

#ifndef __kinskiGL__tcp_server__
#define __kinskiGL__tcp_server__

#include "App.h"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace kinski
{
    class tcp_server;
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
    
    class tcp_server
    {
    public:
        tcp_server(kinski::App::Ptr the_app, int port = 11111);
        
        void stop();
        
        static std::string local_ip(bool ipV6 = false);
        
        void start_accept(int port);
        
    private:
        
        void handle_accept(tcp_connection::Ptr new_connection,
                           const boost::system::error_code& error);
        
        kinski::App::WeakPtr m_app;
        tcp::acceptor m_acceptor_tcp;
    };
    
    class udp_server
    {
    public:
        udp_server(kinski::App::Ptr the_app, int port = 11111);
        
    private:
        void start_receive(int port);
        void send(const std::vector<uint8_t> &bytes, const std::string &ip, int port);
        
        void handle_receive(const boost::system::error_code& error,
                            std::size_t /*bytes_transferred*/);
        
        void handle_send(const boost::system::error_code& /*error*/,
                         std::size_t /*bytes_transferred*/);
        
        kinski::App::WeakPtr m_app;
        udp::socket m_socket;
        udp::resolver resolver_;
        udp::endpoint remote_endpoint_;
        std::array<char, 1024> recv_buffer_;
    };
}// namespace

#endif /* defined(__kinskiGL__tcp_server__) */
