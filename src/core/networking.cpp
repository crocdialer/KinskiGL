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
        
        void async_send_tcp(boost::asio::io_service& io_service,
                            const std::string &str,
                            const std::string &ip,
                            int port)
        {
            async_send_tcp(io_service, std::vector<uint8_t>(str.begin(), str.end()), ip, port);
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void async_send_tcp(boost::asio::io_service& io_service,
                            const std::vector<uint8_t> &bytes,
                            const std::string &ip_string,
                            int port)
        {
                auto socket_ptr = std::make_shared<tcp::socket>(io_service);
                auto resolver_ptr = std::make_shared<tcp::resolver>(io_service);
                
                resolver_ptr->async_resolve({ip_string, kinski::to_string(port)},
                                            [socket_ptr, resolver_ptr, ip_string, bytes]
                                            (const boost::system::error_code& ec,
                                             tcp::resolver::iterator end_point_it)
                {
                    if(!ec)
                    {
                        try
                        {
                            boost::asio::connect(*socket_ptr, end_point_it);
                            boost::asio::async_write(*socket_ptr, boost::asio::buffer(bytes),
                                                     [socket_ptr, ip_string]
                                                     (const boost::system::error_code& error,
                                                      std::size_t bytes_transferred)
                            {
                                if(error){ LOG_WARNING << ip_string << ": " << error.message(); }
                            });
                        }
                        catch(std::exception &e){ LOG_WARNING << ip_string << ": " << e.what(); }
                    }
                    else{ LOG_WARNING << ip_string << ": " << ec.message(); }
                });
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
                                                  [socket_ptr](const boost::system::error_code& error,
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
        
        ///////////////////////////////////////////////////////////////////////////////
        
        udp_server::udp_server(){}
        
        ///////////////////////////////////////////////////////////////////////////////
        
        udp_server::udp_server(boost::asio::io_service& io_service, receive_function f):
        m_impl(new udp_server_impl(io_service, f))
        {
        
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void udp_server::set_receive_function(receive_function f)
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
                        try
                        {
                            std::vector<uint8_t> datavec(m_impl->recv_buffer.begin(),
                                                         m_impl->recv_buffer.begin() + bytes_transferred);
                            m_impl->receive_function(datavec,
                                                     m_impl->remote_endpoint.address().to_string(),
                                                     m_impl->remote_endpoint.port());
                        }
                        catch (std::exception &e){ LOG_WARNING << e.what(); }
                    }
                    start_listen(m_impl->socket.local_endpoint().port());
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
                acceptor.async_accept(socket,
                [this](boost::system::error_code ec)
                {
                    if (!ec)
                    {
                        auto impl = std::make_shared<tcp_connection_impl>(std::move(socket));
                        tcp_connection_ptr con(new tcp_connection(impl));
                        con->set_receive_function([](tcp_connection_ptr, const std::vector<uint8_t> &data)
                        {
                            LOG_DEBUG << std::string(data.begin(), data.end());
                        });
                        con->start_receive();
                        if(connection_callback){ connection_callback(con); }
                    }
                    accept();
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
        
        tcp_connection_ptr tcp_connection::create(boost::asio::io_service& io_service,
                                                  std::string the_ip,
                                                  uint16_t the_port,
                                                  tcp_receive_callback f)
        { return tcp_connection_ptr(new tcp_connection(io_service, the_ip, the_port, f)); }
        
        ///////////////////////////////////////////////////////////////////////////////
        
//        tcp_connection_ptr tcp_connection::create(std::shared_ptr<tcp_connection_impl> the_impl)
//        { return tcp_connection_ptr(new tcp_connection(the_impl)); }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        struct tcp_connection_impl
        {
            tcp_connection_impl(tcp::socket s,
                                net::tcp_receive_callback f = net::tcp_receive_callback()):
            socket(std::move(s)),
            recv_buffer(8192),
            tcp_receive_cb(f)
            {}
            
            tcp::socket socket;
            std::vector<uint8_t> recv_buffer;
            
            net::tcp_receive_callback tcp_receive_cb;
        };
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection::tcp_connection(std::shared_ptr<tcp_connection_impl> the_impl):
        m_impl(the_impl)
        {}
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection::tcp_connection(boost::asio::io_service& io_service,
                                       std::string the_ip,
                                       uint16_t the_port,
                                       tcp_receive_callback f):
        m_impl(new tcp_connection_impl(tcp::socket(io_service), f))
        {
            try
            {
                tcp::resolver resolver(io_service);
                boost::asio::connect(m_impl->socket,
                                     resolver.resolve({the_ip, kinski::to_string(the_port)}));
            } catch (std::exception &e)
            {
                LOG_WARNING << e.what();
            }
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        tcp_connection::~tcp_connection()
        {
            close();
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::send(const std::string &str)
        {
            send(std::vector<uint8_t>(str.begin(), str.end()));
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::send(const std::vector<uint8_t> &bytes)
        {
            send((void*)&bytes[0], bytes.size());
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::send(void* data, size_t num_bytes)
        {
            auto impl = m_impl;
            boost::asio::async_write(m_impl->socket,
                                     boost::asio::buffer(data, num_bytes),
                                     [impl, num_bytes](const boost::system::error_code& error,
                                                       std::size_t bytes_transferred)
            {
                if(error){ LOG_ERROR << error.message(); }
                else if(bytes_transferred < num_bytes)
                {
                    LOG_WARNING << "not all bytes written";
                }
            });
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::set_receive_function(tcp_receive_callback tcp_cb)
        {
            m_impl->tcp_receive_cb = tcp_cb;
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::start_receive()
        {
            _start_receive(m_impl);
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void tcp_connection::_start_receive(std::shared_ptr<tcp_connection_impl> impl_ptr)
        {
            auto impl_cp = impl_ptr ? impl_ptr : m_impl;
            impl_cp->socket.async_receive(boost::asio::buffer(impl_cp->recv_buffer),
            [this, impl_cp](const boost::system::error_code& error,
                            std::size_t bytes_transferred)
            {
                if(!error)
                {
                    if(impl_cp->tcp_receive_cb && bytes_transferred)
                    {
                        std::vector<uint8_t> datavec(impl_cp->recv_buffer.begin(),
                                                     impl_cp->recv_buffer.begin() + bytes_transferred);
                        impl_cp->tcp_receive_cb(shared_from_this(), datavec);
                        LOG_TRACE << "received " << bytes_transferred << "bytes";
                    }
                    
                    // only keep receiving if there are any refs on this instance left
                    if(impl_cp.use_count() > 1)
                        _start_receive(impl_cp);
                }
                else
                {
                    switch (error.value())
                    {
                        case boost::asio::error::eof:
                        case boost::asio::error::connection_reset:
                            LOG_TRACE << error.message() << " ("<<error.value() << ")";
                            impl_cp->socket.close();
                            break;
                            
                        case boost::asio::error::operation_aborted:
                            break;
                            
                        default:
                            LOG_WARNING << error.message() << " ("<<error.value() << ")";
                            break;
                    }
                }
            });
        }
        
        ///////////////////////////////////////////////////////////////////////////////
        
        bool tcp_connection::close()
        {
            try
            {
                // compatibility, maybe unnecessary
//                m_impl->socket.shutdown(m_impl->socket.shutdown_both);
                
                m_impl->socket.close();
                return true;
            } catch (std::exception &e) { LOG_WARNING << e.what(); }
            return false;
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
            return -1;
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
            return -1;
        }
    }
}