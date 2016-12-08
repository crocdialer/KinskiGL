// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  networking.cpp
//  gl
//
//  Created by Fabian on 18/01/14.

#include <boost/asio.hpp>
#include "networking.hpp"

namespace kinski
{
    namespace net
    {
        using namespace boost::asio::ip;
        
        ///////////////////////////////////////////////////////////////////////////////
        
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
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void send_tcp(const std::vector<uint8_t> &bytes,
                      const std::string &ip_string, int port)
        {
            try
            {
                boost::asio::io_service io_service;
                tcp::socket s(io_service);
                tcp::resolver resolver(io_service);
                boost::asio::connect(s, resolver.resolve({ip_string, kinski::to_string(port)}));
                boost::asio::write(s, boost::asio::buffer(bytes));
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection_ptr async_send_tcp(boost::asio::io_service& io_service,
                                          const std::string &str,
                                          const std::string &ip,
                                          int port)
        {
            return async_send_tcp(io_service, std::vector<uint8_t>(str.begin(), str.end()), ip, port);
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection_ptr async_send_tcp(boost::asio::io_service& io_service,
                                          const std::vector<uint8_t> &bytes,
                                          const std::string &the_ip,
                                          uint16_t the_port)
        {
            auto con = tcp_connection::create(io_service, the_ip, the_port);
            con->set_connect_cb([bytes](UARTPtr the_uart)
            {
                the_uart->write_bytes(&bytes[0], bytes.size());
            });
            return con;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void send_udp(const std::vector<uint8_t> &bytes,
                      const std::string &ip_string, int port)
        {
            try
            {
                boost::asio::io_service io_service;
                
                udp::resolver resolver(io_service);
                udp::resolver::query query(udp::v4(), ip_string, kinski::to_string(port));
                udp::endpoint receiver_endpoint = *resolver.resolve(query);
                
                udp::socket socket(io_service, udp::v4());
                socket.send_to(boost::asio::buffer(bytes), receiver_endpoint);
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void async_send_udp(boost::asio::io_service& io_service,
                            const std::string &str,
                            const std::string &ip,
                            int port)
        {
            async_send_udp(io_service, std::vector<uint8_t>(str.begin(), str.end()), ip, port);
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void async_send_udp(boost::asio::io_service& io_service, const std::vector<uint8_t> &bytes,
                            const std::string &ip_string, int port)
        {
            try
            {
                auto socket_ptr = std::make_shared<udp::socket>(io_service, udp::v4());
                auto resolver_ptr = std::make_shared<udp::resolver>(io_service);
                udp::resolver::query query(udp::v4(), ip_string, kinski::to_string(port));
                
                resolver_ptr->async_resolve(query, [socket_ptr, resolver_ptr, ip_string, bytes]
                                            (const boost::system::error_code& ec,
                                             udp::resolver::iterator end_point_it)
                {
                    if(!ec)
                    {
                        socket_ptr->async_send_to(boost::asio::buffer(bytes), *end_point_it,
                                                  [socket_ptr, bytes](const boost::system::error_code& error,
                                                                      std::size_t bytes_transferred)
                        {
                            if(error){LOG_ERROR << error.message();}
                        });
                    }
                    else{ LOG_WARNING << ip_string << ": " << ec.message(); }
                });
            }
            catch (std::exception &e) { LOG_ERROR << ip_string << ": " << e.what(); }
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void async_send_udp_broadcast(boost::asio::io_service& io_service,
                                                 const std::string &str,
                                                 int port)
        {
            async_send_udp_broadcast(io_service, std::vector<uint8_t>(str.begin(), str.end()), port);
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void async_send_udp_broadcast(boost::asio::io_service& io_service,
                                      const std::vector<uint8_t> &bytes,
                                      int port)
        {
            try
            {
                // set broadcast endpoint
                udp::endpoint receiver_endpoint(address_v4::broadcast(), port);
                
                udp::socket socket(io_service, udp::v4());
                socket.set_option(udp::socket::reuse_address(true));
                socket.set_option(boost::asio::socket_base::broadcast(true));
                
                socket.async_send_to(boost::asio::buffer(bytes), receiver_endpoint,
                                     [](const boost::system::error_code& error,
                                        std::size_t bytes_transferred)
                {
                     if (error){ LOG_ERROR << error.message(); }
                });
            }
            catch (std::exception &e) { LOG_ERROR << e.what(); }
        }
        
        /////////////////////////////////////////////////////////////////
        
        struct udp_server_impl
        {
        public:
            udp_server_impl(boost::asio::io_service& io_service, udp_server::receive_cb_t f):
            socket(io_service),
            recv_buffer(1 << 20),
            receive_function(f){}
            
            ~udp_server_impl()
            {
                try{ socket.close(); }
                catch(std::exception &e){ LOG_WARNING << e.what(); }
            }
            
            udp::socket socket;
            udp::endpoint remote_endpoint;
            std::vector<uint8_t> recv_buffer;
            udp_server::receive_cb_t receive_function;
        };
        
        ///////////////////////////////////////////////////////////////////////////////
        
        udp_server::udp_server(){}
        
        ///////////////////////////////////////////////////////////////////////////////
        
        udp_server::udp_server(boost::asio::io_service& io_service, receive_cb_t f):
        m_impl(new udp_server_impl(io_service, f))
        {
        
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void udp_server::set_receive_function(receive_cb_t f)
        {
            m_impl->receive_function = f;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void udp_server::set_receive_buffer_size(size_t sz)
        {
            m_impl->recv_buffer.resize(sz);
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void udp_server::start_listen(uint16_t port)
        {
            try
            {
                if(!m_impl->socket.is_open())
                {
                    m_impl->socket.open(udp::v4());
                    m_impl->socket.bind(udp::endpoint(udp::v4(), port));
                }
                
            }
            catch(std::exception &e){ LOG_WARNING << e.what(); }
            
            if(port != m_impl->socket.local_endpoint().port())
            {
                m_impl->socket.connect(udp::endpoint(udp::v4(), port));
            }
            
            auto impl_cp = m_impl;
            
            m_impl->socket.async_receive_from(boost::asio::buffer(m_impl->recv_buffer),
                                              m_impl->remote_endpoint,
                                              [this, impl_cp](const boost::system::error_code& error,
                                                              std::size_t bytes_transferred)
            {
                if(!error)
                {
                    if(impl_cp->receive_function)
                    {
                        try
                        {
                            std::vector<uint8_t> datavec(impl_cp->recv_buffer.begin(),
                                                         impl_cp->recv_buffer.begin() + bytes_transferred);
                            impl_cp->receive_function(datavec,
                                                      impl_cp->remote_endpoint.address().to_string(),
                                                      impl_cp->remote_endpoint.port());
                        }
                        catch (std::exception &e){ LOG_WARNING << e.what(); }
                    }
                    if(impl_cp.use_count() > 1){ start_listen(impl_cp->socket.local_endpoint().port()); }
                }
                else
                {
                    LOG_WARNING << error.message();
                }
            });
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void udp_server::stop_listen()
        {
            m_impl->socket.close();
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        uint16_t udp_server::listening_port() const
        {
            return m_impl->socket.local_endpoint().port();
        }
            
        /////////////////////////////////////////////////////////////////
        
        struct tcp_server_impl
        {
            tcp::acceptor acceptor;
            tcp::socket socket;
            
            tcp_server::tcp_connection_callback connection_callback;
            
            tcp_server_impl(boost::asio::io_service& io_service,
                            tcp_server::tcp_connection_callback ccb):
            acceptor(io_service),
            socket(io_service),
            connection_callback(ccb){}
            
            void accept()
            {
                acceptor.async_accept(socket, [this](boost::system::error_code ec)
                {
                    if (!ec)
                    {
                        tcp_connection_ptr con(new tcp_connection());
                        con->m_impl = std::make_shared<tcp_connection_impl>(std::move(socket));
                        con->set_tcp_receive_cb([](tcp_connection_ptr, const std::vector<uint8_t> &data)
                        {
                            LOG_DEBUG << std::string(data.begin(), data.end());
                        });
                        con->start_receive();
                        if(connection_callback){ connection_callback(con); }
                        accept();
                    }
              });
            }
        };
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_server::tcp_server(){}
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_server::tcp_server(boost::asio::io_service& io_service, tcp_connection_callback ccb):
        m_impl(new tcp_server_impl(io_service, ccb))
        {}
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_server::set_connection_callback(tcp_connection_callback ccb)
        {
            m_impl->connection_callback = ccb;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_server::start_listen(uint16_t port)
        {
            if(!m_impl)
            {
                LOG_ERROR << "could not start listen, server not initiated";
                return;
            }
            
            if(!m_impl->acceptor.is_open() ||
               port != m_impl->acceptor.local_endpoint().port())
            {
                try
                {
                    m_impl->acceptor.close();
                    m_impl->acceptor.open(tcp::v4());
                    m_impl->acceptor.bind(tcp::endpoint(tcp::v4(), port));
                    m_impl->acceptor.listen();
                }
                catch (std::exception &e)
                {
                    LOG_ERROR << "could not start listening on port: " << port << " (" << e.what()
                    << ")";
                    return;
                }
            }
            m_impl->accept();
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        uint16_t tcp_server::listening_port() const
        {
            return m_impl->acceptor.local_endpoint().port();
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_server::stop_listen()
        {
            m_impl->acceptor.close();
        }
        
        /////////////////////////////////////////////////////////////////////////////////////
        
        struct tcp_connection_impl
        {
            tcp_connection_impl(tcp::socket s,
                                tcp_connection::tcp_receive_cb_t f = tcp_connection::tcp_receive_cb_t()):
            socket(std::move(s)),
            recv_buffer(8192),
            tcp_receive_cb(f)
            {}
            
            ~tcp_connection_impl()
            {
                try{ socket.close(); }
                catch (std::exception &e) { LOG_WARNING << e.what(); }
            }
            
            tcp::socket socket;
            std::vector<uint8_t> recv_buffer;
            
            // additional receive callback with connection context
            tcp_connection::tcp_receive_cb_t tcp_receive_cb;
            
            // used by UART interface
            UART::connection_cb_t m_connect_cb, m_disconnect_cb;
            UART::receive_cb_t m_receive_cb;
        };
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection_ptr tcp_connection::create(boost::asio::io_service& io_service,
                                                  const std::string &the_ip,
                                                  uint16_t the_port,
                                                  tcp_receive_cb_t f)
        {
            auto ret = tcp_connection_ptr(new tcp_connection(io_service, the_ip, the_port, f));
            auto connect_cb = [](UARTPtr the_uart)
            {
                LOG_TRACE_1 << "connected: " << the_uart->description();
            };
            ret->set_connect_cb(connect_cb);
            
            auto resolver_ptr = std::make_shared<tcp::resolver>(io_service);
            
            resolver_ptr->async_resolve({the_ip, kinski::to_string(the_port)},
                                        [ret, resolver_ptr, the_ip]
                                        (const boost::system::error_code& ec,
                                         tcp::resolver::iterator end_point_it)
            {
                if(!ec)
                {
                    try
                    {
                        boost::asio::async_connect(ret->m_impl->socket, end_point_it,
                                                   [ret](const boost::system::error_code& ec,
                                                         tcp::resolver::iterator end_point_it)
                        {
                            if(!ec)
                            {
                                if(ret->m_impl->m_connect_cb)
                                {
                                    ret->m_impl->m_connect_cb(ret);
                                }
                                ret->start_receive();
                            }
                        });
                    }
                    catch(std::exception &e){ LOG_WARNING << the_ip << ": " << e.what(); }
                }
                else{ LOG_WARNING << the_ip << ": " << ec.message(); }
            });
            return ret;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection::tcp_connection(){}
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection::tcp_connection(boost::asio::io_service& io_service,
                                       const std::string &the_ip,
                                       uint16_t the_port,
                                       tcp_receive_cb_t f):
        m_impl(new tcp_connection_impl(tcp::socket(io_service), f))
        {
            
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection::~tcp_connection()
        {
            close();
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        bool tcp_connection::open()
        {
            return false;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        size_t tcp_connection::write_bytes(const void *buffer, size_t num_bytes)
        {
            auto bytes = std::vector<uint8_t>((uint8_t*)buffer, (uint8_t*)buffer + num_bytes);
            
            auto impl = m_impl;
            boost::asio::async_write(m_impl->socket,
                                     boost::asio::buffer(bytes),
                                     [impl, bytes](const boost::system::error_code& error,
                                                   std::size_t bytes_transferred)
            {
                if(!error)
                {
                    if(bytes_transferred < bytes.size())
                    {
                        LOG_WARNING << "not all bytes written";
                    }
                }
                else
                {
                    switch(error.value())
                    {
                        case boost::asio::error::bad_descriptor:
                        default:
                            LOG_TRACE_1 << error.message() << " (" << error.value() << ")";
                            break;
                    }
                }
            });
            return num_bytes;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        size_t tcp_connection::read_bytes(void *buffer, size_t sz)
        {
            return 0;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::set_tcp_receive_cb(tcp_receive_cb_t tcp_cb)
        {
            m_impl->tcp_receive_cb = tcp_cb;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::start_receive()
        {
            auto impl_cp = m_impl;
            auto weak_self = std::weak_ptr<tcp_connection>(shared_from_this());
            
            impl_cp->socket.async_receive(boost::asio::buffer(impl_cp->recv_buffer),
                                          [impl_cp, weak_self](const boost::system::error_code& error,
                                                               std::size_t bytes_transferred)
            {
                auto self = weak_self.lock();
                
                if(!error)
                {
                    if(bytes_transferred)
                    {
                        std::vector<uint8_t> datavec(impl_cp->recv_buffer.begin(),
                                                     impl_cp->recv_buffer.begin() + bytes_transferred);
                        if(self && impl_cp->tcp_receive_cb)
                        {
                            impl_cp->tcp_receive_cb(self, datavec);
                        }
                        if(self && impl_cp->m_receive_cb)
                        {
                            impl_cp->m_receive_cb(self, datavec);
                        }
                        LOG_TRACE_2 << "received " << bytes_transferred << " bytes";
                    }
                    
                    // only keep receiving if there are any refs on this instance left
                    if(self){ self->start_receive(); }
                }
                else
                {
                    switch (error.value())
                    {
                        case boost::asio::error::eof:
                        case boost::asio::error::connection_reset:
                            impl_cp->socket.close();
                        case boost::asio::error::operation_aborted:
                        case boost::asio::error::bad_descriptor:
                            {
                                std::string str = "tcp_connection";
                                if(self){ str = self->description(); }
                                LOG_TRACE_2 << "disconnected: " << str;
                            }
                            
                            if(self && impl_cp->m_disconnect_cb)
                            {
                                impl_cp->m_disconnect_cb(self);
                            }
                            
                        default:
                            LOG_TRACE_3 << error.message() << " ("<<error.value() << ")";
                            break;
                    }
                }
            });
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::close()
        {
            try{ m_impl->socket.close(); }
            catch (std::exception &e) { LOG_WARNING << e.what(); }
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        bool tcp_connection::is_open() const
        {
            return m_impl->socket.is_open();
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        uint16_t tcp_connection::port() const
        {
            try{ return m_impl->socket.local_endpoint().port(); }
            catch (std::exception &e) {}
            return 0;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        std::string tcp_connection::remote_ip() const
        {
            try{ return m_impl->socket.remote_endpoint().address().to_string(); }
            catch (std::exception &e) {}
            return "0.0.0.0";
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        uint16_t tcp_connection::remote_port() const
        {
            try{ return m_impl->socket.remote_endpoint().port(); }
            catch (std::exception &e) {}
            return 0;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        size_t tcp_connection::available() const
        {
            return 0;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::drain()
        {
        
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        std::string tcp_connection::description() const
        {
            return "tcp_connection: " + remote_ip() + " (" + to_string(remote_port()) + ")";
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::set_receive_cb(receive_cb_t cb)
        {
            m_impl->m_receive_cb = cb;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::set_connect_cb(connection_cb_t cb)
        {
            m_impl->m_connect_cb = cb;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::set_disconnect_cb(connection_cb_t cb)
        {
            m_impl->m_disconnect_cb = cb;
        }
    }
}
