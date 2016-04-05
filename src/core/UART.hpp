//
//  UART.hpp
//  kinskiGL
//
//  Created by Fabian on 05/04/16.
//
//

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
        if(!the_container.empty())
        {
            size_t elem_sz = sizeof(the_container.front());
            return write_bytes(&the_container[0], elem_sz * the_container.size());
        }
        return 0;
    };
    
    //! returns the number of bytes available for reading
    virtual size_t available() = 0;
    
    //! empty buffers
    virtual void drain() = 0;
};
}// namespace