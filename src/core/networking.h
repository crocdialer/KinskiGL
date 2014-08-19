//
//  networking.h
//  gl
//
//  Created by Fabian on 18/01/14.
//
//

#ifndef __gl__networking__
#define __gl__networking__

#include "Definitions.h"

namespace kinski
{
    namespace net
    {
        class tcp_server;
        class tcp_connection;
        class udp_server;
        
        // signature for a receive function
        typedef std::function<void (const std::vector<uint8_t>&)> receive_function;
        typedef std::function<void(tcp_connection&,
                                   const std::vector<uint8_t>&)> tcp_receive_callback;
        
        std::string local_ip(bool ipV6 = false);
        
        KINSKI_API void send_tcp(const std::string &str, const std::string &ip_string, int port);
        KINSKI_API void send_tcp(const std::vector<uint8_t> &bytes,
                                 const std::string &ip_string, int port);
        
        KINSKI_API void send_udp(const std::vector<uint8_t> &bytes,
                                 const std::string &ip_string, int port);
        
        KINSKI_API void send_udp_broadcast(const std::vector<uint8_t> &bytes, int port);
        
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
            
            typedef std::function<void(tcp_connection&)> tcp_connection_callback;
            
            tcp_server();
            
            tcp_server(boost::asio::io_service& io_service,
                       short port,
                       tcp_connection_callback ccb);
            
            KINSKI_API void start_listen(int port);
            KINSKI_API void stop_listen();
            KINSKI_API void set_connection_callback(tcp_connection_callback ccb);
            
            KINSKI_API unsigned short listening_port() const;
            
        private:

            struct tcp_server_impl;
            std::shared_ptr<tcp_server_impl> m_impl;
        };
        
        KINSKI_API class tcp_connection
        {
        public:
            
            struct tcp_connection_impl;
            
            tcp_connection(boost::asio::io_service& io_service,
                           std::string the_ip,
                           short the_port,
                           tcp_receive_callback f);
            
            tcp_connection(std::shared_ptr<tcp_connection_impl> the_impl);
            ~tcp_connection();
            
            KINSKI_API void send(const std::string &str);
            KINSKI_API void send(const std::vector<uint8_t> &bytes);
            
            KINSKI_API void set_receive_function(tcp_receive_callback f);
            KINSKI_API void start_receive();
            
            KINSKI_API void close();
            KINSKI_API bool is_open() const;
            
            KINSKI_API unsigned short port() const;
            KINSKI_API std::string remote_ip() const;
            KINSKI_API unsigned short remote_port() const;

        private:
            
            typedef std::shared_ptr<tcp_connection_impl> impl_ptr;
            impl_ptr m_impl;
            void _start_receive(impl_ptr the_impl_ptr = impl_ptr());
        };
        
    }// namespace net
    
}// namespace kinski

#endif /* defined(__gl__networking__) */
