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
        
        void async_send_udp(boost::asio::io_service& io_service,
                            const std::string &str,
                            const std::string &ip,
                            int port)
        {
            async_send_udp(io_service, std::vector<uint8_t>(str.begin(), str.end()), ip, port);
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
        
        void async_send_udp_broadcast(boost::asio::io_service& io_service,
                                                 const std::string &str,
                                                 int port)
        {
            async_send_udp_broadcast(io_service, std::vector<uint8_t>(str.begin(), str.end()), port);
        }
        
        void async_send_udp_broadcast(boost::asio::io_service& io_service,
                                      const std::vector<uint8_t> &bytes,
                                      int port)
        {
            try
            {
                // set broadcast enpoint
                udp::endpoint receiver_endpoint(address_v4::broadcast(), port);
                
                udp::socket socket(io_service, udp::v4());
                socket.set_option(udp::socket::reuse_address(true));
                socket.set_option(boost::asio::socket_base::broadcast(true));
                
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
        
        /////////////////////////////////////////////////////////////////
        
        struct udp_server_impl
        {
        public:
            udp_server_impl(boost::asio::io_service& io_service, net::receive_function f):
            socket(io_service),
            resolver(io_service),
            recv_buffer(1 << 20),
            receive_function(f){}
            
            udp::socket socket;
            udp::resolver resolver;
            udp::endpoint remote_endpoint;
            std::vector<uint8_t> recv_buffer;
            net::receive_function receive_function;
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
        
        void udp_server::set_receive_buffer_size(size_t sz)
        {
            m_impl->recv_buffer.resize(sz);
        }
        
        void udp_server::start_listen(int port)
        {
            if(!m_impl->socket.is_open())
            {
                m_impl->socket.open(udp::v4());
                m_impl->socket.bind(udp::endpoint(udp::v4(), port));
            }
            if(port != m_impl->socket.local_endpoint().port())
            {
                m_impl->socket.connect(udp::endpoint(udp::v4(), port));
            }
            
            m_impl->socket.async_receive_from(boost::asio::buffer(m_impl->recv_buffer),
                                              m_impl->remote_endpoint,
                                              [&](const boost::system::error_code& error,
                                                  std::size_t bytes_transferred)
            {
                if (!error)
                {
                    if(m_impl->receive_function)
                    {
                        std::vector<uint8_t> datavec(m_impl->recv_buffer.begin(),
                                                     m_impl->recv_buffer.begin() + bytes_transferred);
                        m_impl->receive_function(datavec);
                    }
                    start_listen(m_impl->socket.local_endpoint().port());
                }
                else
                {
                    LOG_WARNING<<error.message();
                }
            });
        }
            
        void udp_server::stop_listen()
        {
            m_impl->socket.close();
        }
            
        /////////////////////////////////////////////////////////////////
        
        struct tcp_server::tcp_server_impl
        {
            tcp::acceptor acceptor;
            tcp::socket socket;
            
            tcp_server_impl(boost::asio::io_service& io_service, short port):
            acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
            socket(io_service){}
        };
        
        tcp_server::tcp_server(boost::asio::io_service& io_service, short port):
        m_impl(new tcp_server_impl(io_service, port))
        {
//                do_accept();
        }
        
        struct tcp_connection_impl
        {
            tcp_connection_impl(tcp::socket s, net::receive_function f):
            socket(std::move(s)),
            recv_buffer(2048),
            receive_function(f){}
            
            tcp::socket socket;
            std::vector<uint8_t> recv_buffer;
            net::receive_function receive_function;
        };
                
        tcp_connection::tcp_connection(boost::asio::io_service& io_service, net::receive_function f)
        {
            
        }
    }
}