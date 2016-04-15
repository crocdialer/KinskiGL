// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  UART.hpp
//  kinskiGL
//
//  Created by Fabian on 05/04/16.

#pragma once

#include "core/core.hpp"

namespace kinski
{
//! UART interface
class UART
{
public:
    
    //! setup and initialize the device
    virtual bool setup() = 0;
    
    //! close the device, cancel current transfers
    virtual void close() = 0;
    
    //! returns true if the device is initialized and ready to transfer
    virtual bool is_initialized() const = 0;
    
    //! reads up to sz bytes into buffer, returns the number of bytes actually read
    virtual size_t read_bytes(void *buffer, size_t sz) = 0;
    
    //! transfer sz bytes from buffer, returns the number of bytes actually written
    virtual size_t write_bytes(const void *buffer, size_t sz) = 0;
    
    /*!
     * convenience template to transfer the content of containers, like std::string or std::vector
     */
    template<typename T> size_t write(const T &the_container)
    {
        std::vector<uint8_t> data(std::begin(the_container), std::end(the_container));
        return data.empty() ? 0 : write_bytes(&data[0], data.size());
    };
    
    //! returns the number of bytes available for reading
    virtual size_t available() = 0;
    
    //! empty buffers
    virtual void drain() = 0;
    
    virtual void flush(bool flush_in = true, bool flush_out = true) = 0;
    
    virtual std::string description() = 0;
};
    
typedef std::shared_ptr<UART> UART_Ptr;
    
}// namespace