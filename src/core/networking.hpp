//
//  networking.h
//  gl
//
//  Created by Fabian on 18/01/14.
//
//

#pragma once

#include "core/core.hpp"

namespace kinski
{
    namespace net
    {
        class tcp_server;
        class udp_server;
        
        typedef std::shared_ptr<class tcp_connection> tcp_connection_ptr;
        
        // signature for a receive function
        typedef std::function<void (const std::vector<uint8_t>&)> receive_function;
        typedef std::function<void(tcp_connection_ptr,
                                   const std::vector<uint8_t>&)> tcp_receive_callback;
        
        std::string local_ip(bool ipV6 = false);
        
        KINSKI_API void send_tcp(const std::string &str, const std::string &ip_string, int port);
        KINSKI_API void send_tcp(const std::vector<uint8_t> &bytes,
                                 const std::string &ip_string, int port);
        
        KINSKI_API void send_udp(const std::vector<uint8_t> &bytes,
                                 const std::string &ip_string, int port);
        
        KINSKI_API void send_udp_broadcast(const std::vector<uint8_t> &bytes, int port);
        
        KINSKI_API void async_send_tcp(boost::asio::io_service& io_service,
                                       const std::string &str,
                                       const std::string &ip,
                                       int port);
        
        KINSKI_API void async_send_tcp(boost::asio::io_service& io_service,
                                       const std::vector<uint8_t> &bytes,
                                       const std::string &ip,
                                       int port);
        
        KINSKI_API void async_send_udp(boost::asio::io_service& io_service,
                                       const std::string &str,
                                       const std::string &ip,
                                       int port);
        
        KINSKI_API void async_send_udp(boost::asio::io_service& io_service,
                                       const std::vector<uint8_t> &bytes,
                                       const std::string &ip,
                                       int port);
        
        KINSKI_API void async_send_udp_broadcast(boost::asio::io_service& io_service,
                                                 const std::vector<uint8_t> &bytes,
                                                 int port);
        
        KINSKI_API void async_send_udp_broadcast(boost::asio::io_service& io_service,
                                                 const std::string &str,
                                                 int port);
        
        KINSKI_API class udp_server
        {
        public:
            
            udp_server();
            udp_server(boost::asio::io_service& io_service, receive_function f = receive_function());
            
            KINSKI_API void start_listen(int port);
            KINSKI_API void stop_listen();
            KINSKI_API void set_receive_function(receive_function f);
            KINSKI_API void set_receive_buffer_size(size_t sz);
            KINSKI_API unsigned short listening_port() const;
            
        private:
            std::shared_ptr<struct udp_server_impl> m_impl;
        };
        
        KINSKI_API class tcp_server
        {
        public:
            
            typedef std::function<void(tcp_connection_ptr)> tcp_connection_callback;
            
            tcp_server();
            
            tcp_server(boost::asio::io_service& io_service, tcp_connection_callback ccb);
            
            KINSKI_API void start_listen(int port);
            KINSKI_API void stop_listen();
            KINSKI_API void set_connection_callback(tcp_connection_callback ccb);
            
            KINSKI_API unsigned short listening_port() const;
            
        private:

            struct tcp_server_impl;
            std::shared_ptr<tcp_server_impl> m_impl;
        };
        
        KINSKI_API class tcp_connection : public std::enable_shared_from_this<tcp_connection>
        {
        public:
            
            struct tcp_connection_impl;
            
            KINSKI_API static tcp_connection_ptr create(boost::asio::io_service& io_service,
                                                        std::string the_ip,
                                                        short the_port,
                                                        tcp_receive_callback f);
            
            KINSKI_API static tcp_connection_ptr create(std::shared_ptr<tcp_connection_impl> the_impl);
            
            ~tcp_connection();
            
            KINSKI_API void send(const std::string &str);
            KINSKI_API void send(const std::vector<uint8_t> &bytes);
            
            KINSKI_API void set_receive_function(tcp_receive_callback f);
            KINSKI_API void start_receive();
            
            KINSKI_API bool close();
            KINSKI_API bool is_open() const;
            
            KINSKI_API int port() const;
            KINSKI_API std::string remote_ip() const;
            KINSKI_API int remote_port() const;

        private:
            
            tcp_connection(boost::asio::io_service& io_service,
                           std::string the_ip,
                           short the_port,
                           tcp_receive_callback f);
            
            tcp_connection(std::shared_ptr<tcp_connection_impl> the_impl);
            
            typedef std::shared_ptr<tcp_connection_impl> impl_ptr;
            impl_ptr m_impl;
            void _start_receive(impl_ptr the_impl_ptr = impl_ptr());
        };
    }// namespace net
}// namespace kinski