// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <map>
#include "Connection.hpp"

namespace kinski
{

DEFINE_CLASS_PTR(Serial);

class Serial : public Connection, public std::enable_shared_from_this<Serial>
{
    
public:
    
    static SerialPtr create(io_service_t &io, receive_cb_t cb = receive_cb_t());
    virtual ~Serial();
    
    static std::vector<std::string>
    device_list(const std::set<std::string>& the_patterns = std::set<std::string>());
    
    static std::map<std::string, SerialPtr> connected_devices();
    
    bool open(const std::string &the_name, int the_baudrate = 57600);
    
    bool open() override;
    void close() override;
    bool is_open() const override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() const override;
    std::string description() const override;
    void drain() override;
    void set_receive_cb(receive_cb_t the_cb = receive_cb_t()) override;
    void set_connect_cb(connection_cb_t cb = connection_cb_t()) override;
    void set_disconnect_cb(connection_cb_t cb = connection_cb_t()) override;
    
private:
    
    void async_read_bytes();
    void async_write_bytes(const void *buffer, size_t sz);
    
    Serial(io_service_t &io, receive_cb_t cb = receive_cb_t());
    std::shared_ptr<struct SerialImpl> m_impl;
};
    
    //----------------------------------------------------------------------
}
