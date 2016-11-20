// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "UART.hpp"

namespace kinski
{

typedef std::shared_ptr<class Serial> SerialPtr;

    class Serial : public UART, public std::enable_shared_from_this<Serial>
{
    
public:
    
    static SerialPtr create(boost::asio::io_service &io, receive_cb_t cb = receive_cb_t());
    virtual ~Serial();
    
    static std::vector<std::string> device_list();
    
    bool open(const std::string &the_name, int the_baudrate = 57600);
    void close() override;
    bool is_open() const override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() const override;
    std::string description() const override;
    void drain() override;
    
    void set_receive_cb(receive_cb_t the_cb) override;
    void set_connect_cb(connection_cb_t cb) override;
    void set_disconnect_cb(connection_cb_t cb) override;
    
private:
    
    void async_read_bytes();
    void async_write_bytes(const void *buffer, size_t sz);
    
    Serial(boost::asio::io_service &io, receive_cb_t cb = receive_cb_t());
    std::unique_ptr<struct SerialImpl> m_impl;
};
    
    //----------------------------------------------------------------------
}
