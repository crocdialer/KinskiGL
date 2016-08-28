// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  networking.h
//
//  Created by Fabian on 18/01/14.

#pragma once

#include "core/core.hpp"

namespace kinski
{
    namespace net
    {
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
            
            typedef std::function<void (const std::vector<uint8_t>&, const std::string&, uint16_t)>
                receive_cb_t;
            
            udp_server();
            udp_server(boost::asio::io_service& io_service, receive_cb_t f = receive_cb_t());
            
            KINSKI_API void start_listen(uint16_t port);
            KINSKI_API void stop_listen();
            KINSKI_API void set_receive_function(receive_cb_t f);
            KINSKI_API void set_receive_buffer_size(size_t sz);
            KINSKI_API uint16_t listening_port() const;
            
        private:
            std::shared_ptr<struct udp_server_impl> m_impl;
        };
        
        typedef std::shared_ptr<class tcp_connection> tcp_connection_ptr;
        
        KINSKI_API class tcp_server
        {
        public:
            
            typedef std::function<void(tcp_connection_ptr)> tcp_connection_callback;
            
            tcp_server();
            
            tcp_server(boost::asio::io_service& io_service, tcp_connection_callback ccb);
            
            KINSKI_API void start_listen(uint16_t port);
            KINSKI_API void stop_listen();
            KINSKI_API void set_connection_callback(tcp_connection_callback ccb);
            
            KINSKI_API uint16_t listening_port() const;
            
        private:
            std::shared_ptr<struct tcp_server_impl> m_impl;
        };
        
        KINSKI_API class tcp_connection : public std::enable_shared_from_this<tcp_connection>
        {
        public:
            
            // tcp receive function
            typedef std::function<void(tcp_connection_ptr,
                                       const std::vector<uint8_t>&)> tcp_receive_cb_t;
            
            KINSKI_API static tcp_connection_ptr create(boost::asio::io_service& io_service,
                                                        std::string the_ip,
                                                        uint16_t the_port,
                                                        tcp_receive_cb_t f);
            
            virtual ~tcp_connection();
            
            KINSKI_API void send(const std::string &str);
            KINSKI_API void send(const std::vector<uint8_t> &bytes);
            KINSKI_API void send(void* data, size_t num_bytes);
            
            KINSKI_API void tcp_receive_cb(tcp_receive_cb_t f);
            
            KINSKI_API bool close();
            KINSKI_API bool is_open() const;
            
            KINSKI_API uint16_t port() const;
            KINSKI_API std::string remote_ip() const;
            KINSKI_API uint16_t remote_port() const;

        private:
            
            friend tcp_server_impl;
            struct tcp_connection_impl;
            std::shared_ptr<tcp_connection_impl> m_impl;
            
            tcp_connection(boost::asio::io_service& io_service,
                           std::string the_ip,
                           uint16_t the_port,
                           tcp_receive_cb_t f);
            
            tcp_connection();
            
            void start_receive();
        };
    }// namespace net
}// namespace kinski