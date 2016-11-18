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
    
    std::vector<std::string> device_list();
    
    bool setup() override;	// use default port, baud (0,9600)
    bool setup(const std::string &the_name, int the_baudrate);
    void close() override;
    bool is_initialized() const override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() const override;
    std::string description() const override;
    void drain() override;
    void flush(bool flushIn = true, bool flushOut = true) override;
    
    void async_read_bytes();
    void async_write_bytes(const void *buffer, size_t sz);
    
    void set_receive_cb(receive_cb_t the_cb) override;
    void set_connect_cb(connect_cb_t cb) override;
    
private:
    void start_receive();
    
    Serial(boost::asio::io_service &io, receive_cb_t cb = receive_cb_t());
    std::shared_ptr<struct SerialImpl> m_impl;
};
    
    //----------------------------------------------------------------------
}
