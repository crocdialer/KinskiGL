//
//  networking.cpp
//  kinskiGL
//
//  Created by Fabian on 18/01/14.
//
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "Logger.h"
#include "networking.h"

namespace kinski
{
    namespace net
    {
        using namespace boost::asio::ip;
        
        std::string local_ip(bool ipV6)
        {
            std::string ret = "unknown_ip";
            
            try
            {
                boost::asio::io_service io;
                tcp::resolver resolver(io);
                tcp::resolver::query query(ipV6 ? tcp::v6() : tcp::v4(), host_name(), "");
                tcp::resolver::iterator it = resolver.resolve(query), end;
                
                for (; it != end; ++it)
                {
                    const tcp::endpoint &endpoint = *it;
                    ret = endpoint.address().to_string();
                }
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
            
            return ret;
        }
        
        void send_tcp(const std::vector<uint8_t> &bytes,
                      const std::string &ip_string, int port)
        {
            try
            {
                boost::asio::io_service io_service;
                tcp::socket s(io_service);
                tcp::resolver resolver(io_service);
                boost::asio::connect(s, resolver.resolve({ip_string, kinski::as_string(port)}));
                boost::asio::write(s, boost::asio::buffer(bytes));
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
        }
        
        void async_send_tcp(boost::asio::io_service& io_service,
                            const std::vector<uint8_t> &bytes,
                            const std::string &ip_string,
                            int port)
        {
            try
            {
                boost::asio::io_service io_service;
                tcp::socket s(io_service);
                tcp::resolver resolver(io_service);
                boost::asio::connect(s, resolver.resolve({ip_string, kinski::as_string(port)}));
                boost::asio::async_write(s, boost::asio::buffer(bytes),
                                         [](const boost::system::error_code& error,  // Result of operation.
                                            std::size_t bytes_transferred)           // Number of bytes sent.
                                         {
                                             if (error){LOG_ERROR << error.message();}
                                             else{ }
                                         });
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
        }
        
        void send_udp(const std::vector<uint8_t> &bytes,
                      const std::string &ip_string, int port)
        {
            try
            {
                boost::asio::io_service io_service;
                
                udp::resolver resolver(io_service);
                udp::resolver::query query(udp::v4(), ip_string, kinski::as_string(port));
                udp::endpoint receiver_endpoint = *resolver.resolve(query);
                
                udp::socket socket(io_service, udp::v4());
                socket.send_to(boost::asio::buffer(bytes), receiver_endpoint);
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
        }
        
        void async_send_udp(boost::asio::io_service& io_service, const std::vector<uint8_t> &bytes,
                            const std::string &ip_string, int port)
        {
            try
            {
                udp::resolver resolver(io_service);
                udp::resolver::query query(udp::v4(), ip_string, kinski::as_string(port));
                udp::endpoint receiver_endpoint = *resolver.resolve(query);
                
                udp::socket socket(io_service, udp::v4());
                
                socket.async_send_to(boost::asio::buffer(bytes), receiver_endpoint,
                                     [](const boost::system::error_code& error,  // Result of operation.
                                        std::size_t bytes_transferred)           // Number of bytes sent.
                                     {
                                         if (error){LOG_ERROR << error.message();}
                                         else{ }
                                     });
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
        }
        
        class udp_server_impl
        {
        public:
            udp_server_impl(boost::asio::io_service& io_service, udp_server::receive_function f):
            socket(io_service),
            resolver(io_service),
            receive_function(f)
            {
                
            }
            
            void start_listen(int port)
            {
                if(!socket.is_open())
                {
                    socket.open(udp::v4());
                    socket.bind(udp::endpoint(udp::v4(), port));
                }
                if(port != socket.local_endpoint().port())
                {
                    socket.connect(udp::endpoint(udp::v4(), port));
                }
                
                socket.async_receive_from(boost::asio::buffer(recv_buffer),
                                          remote_endpoint,
                                          boost::bind(&udp_server_impl::handle_receive, this,
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
            }
            
            void stop_listen()
            {
                socket.close();
            }
            
            void handle_receive(const boost::system::error_code& error,
                                std::size_t bytes_transferred)
            {
                if (!error)
                {
                    if(receive_function)
                    {
                        std::vector<uint8_t> datavec(recv_buffer.begin(),
                                                     recv_buffer.begin() + bytes_transferred);
                        receive_function(datavec);
                    }
                    start_listen(socket.local_endpoint().port());
                }
                else
                {
                    LOG_ERROR<<error.message();
                }
            }
            
            udp::socket socket;
            udp::resolver resolver;
            udp::endpoint remote_endpoint;
            std::array<char, 1 << 20> recv_buffer;
            udp_server::receive_function receive_function;
        };
        
        udp_server::udp_server(){}
        
        udp_server::udp_server(boost::asio::io_service& io_service, receive_function f):
        m_impl(new udp_server_impl(io_service, f))
        {
        
        }
        
        void udp_server::set_receive_function(receive_function f)
        {
            m_impl->receive_function = f;
        }
        
        void udp_server::start_listen(int port)
        {
            m_impl->start_listen(port);
        }
            
        void udp_server::stop_listen()
        {
            m_impl->stop_listen();
        }
    }
}
