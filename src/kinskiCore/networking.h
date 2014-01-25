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

// forward declarations
namespace boost
{
    namespace asio
    {
        class io_service;
        
        namespace ip
        {
//            namespace udp{class socket;}
//            namespace tcp{class socket;}
        }
    }
}

namespace kinski
{
    namespace net
    {
        
        std::string local_ip(bool ipV6 = false);
        
        KINSKI_API void send_tcp(const std::vector<uint8_t> &bytes,
                                 const std::string &ip_string, int port);
        
        KINSKI_API void async_send_tcp(boost::asio::io_service& io_service,
                                       const std::vector<uint8_t> &bytes,
                                       const std::string &ip,
                                       int port);
        
        KINSKI_API void send_udp(const std::vector<uint8_t> &bytes,
                                 const std::string &ip_string, int port);
        
        KINSKI_API void async_send_udp(boost::asio::io_service& io_service,
                                       const std::vector<uint8_t> &bytes,
                                       const std::string &ip,
                                       int port);
        
        KINSKI_API class udp_server
        {
        public:
            
            typedef std::function<void (const std::vector<uint8_t>&)> receive_function;
            
            udp_server();
            udp_server(boost::asio::io_service& io_service, receive_function f = receive_function());
            
            KINSKI_API void start_listen(int port);
            KINSKI_API void stop_listen();
            KINSKI_API void set_receive_function(receive_function f);
            
        private:
            std::shared_ptr<class udp_server_impl> m_impl;
        };
        
//        class tcp_connection : public std::enable_shared_from_this<tcp_connection>
//        {
//        public:
//            typedef std::shared_ptr<tcp_connection> Ptr;
//            
//            static Ptr create(boost::asio::io_service& io_service)
//            {
//                return std::make_shared<tcp_connection>(io_service));
//            }
//            
////            tcp::socket& socket(){ return m_socket; }
//            
//            void start();
//            
//            void send();
//            void receive();
//            
//        private:
//            tcp_connection(boost::asio::io_service& io_service);
////            void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
////            void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
//            
//            std::shared_ptr<class udp_server_impl> m_impl;
//            
//            tcp::socket m_socket;
//            std::string m_message;
//            std::vector<uint8_t> m_receive_buffer;
//        };
        
    }// namespace net
    
}// namespace kinski

#endif /* defined(__kinskiGL__networking__) */
