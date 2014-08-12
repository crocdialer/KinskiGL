//
//  networking.h
//  kinskiGL
//
//  Created by Fabian on 18/01/14.
//
//

#ifndef __kinskiGL__networking__
#define __kinskiGL__networking__

#include "Definitions.h"

namespace kinski
{
    namespace net
    {
        // signature for a receive function
        typedef std::function<void (const std::vector<uint8_t>&)> receive_function;
        
        typedef std::shared_ptr<class tcp_connection> tcp_connection_ptr;
        
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
            
        private:
            std::shared_ptr<struct udp_server_impl> m_impl;
        };
        
        KINSKI_API class tcp_server
        {
        public:
            
            tcp_server(boost::asio::io_service& io_service, short port);
            
            KINSKI_API void start_listen(int port);
            KINSKI_API void stop_listen();
            KINSKI_API void set_receive_function(receive_function f);
            KINSKI_API void set_receive_buffer_size(size_t sz);
            
        private:
//            void do_accept()
//            {
//                acceptor_.async_accept(socket_,
//                                       [this](boost::system::error_code ec)
//                                       {
//                                           if (!ec)
//                                           {
//                                               std::make_shared<session>(std::move(socket_))->start();
//                                           }
//                                           
//                                           do_accept();
//                                       });
//            }
            
            struct tcp_server_impl;
            std::shared_ptr<tcp_server_impl> m_impl;
            
//            tcp::acceptor acceptor_;
//            tcp::socket socket_;
        };
        
        class tcp_connection : public std::enable_shared_from_this<tcp_connection>
        {
        public:
            
            static tcp_connection_ptr create(boost::asio::io_service& io_service,
                                             receive_function f = receive_function())
            {
                return tcp_connection_ptr(new tcp_connection(io_service, f));
            }

            void send(const std::vector<uint8_t> &bytes);
            void receive();
            
            KINSKI_API void set_receive_function(receive_function f);
            
        private:
            tcp_connection();
            tcp_connection(boost::asio::io_service& io_service, receive_function f);

            std::shared_ptr<struct tcp_connection_impl> m_impl;
        };
        
    }// namespace net
    
}// namespace kinski

#endif /* defined(__kinskiGL__networking__) */
