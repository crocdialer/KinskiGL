#include <mutex>
#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include "core/file_functions.hpp"
#include "core/CircularBuffer.hpp"
#include "Serial.hpp"

namespace kinski
{
    struct SerialImpl
    {
        std::string m_device_name;
        boost::asio::serial_port m_serial_port;
        Serial::connection_cb_t m_connect_cb, m_disconnect_cb;
        Serial::receive_cb_t m_receive_cb;
        std::vector<uint8_t> m_rec_buffer;
        CircularBuffer<uint8_t> m_buffer{512 * (1 << 10)};
        std::mutex m_mutex;
        
        SerialImpl(boost::asio::io_service &io, Serial::receive_cb_t rec_cb):
        m_serial_port(io),
        m_receive_cb(rec_cb){}
    };
    
    ///////////////////////////////////////////////////////////////////////////////
    
    SerialPtr Serial::create(boost::asio::io_service &io, receive_cb_t cb)
    {
        auto ret = SerialPtr(new Serial(io, cb));
        ret->set_connect_cb([](UARTPtr the_uart)
        {
            LOG_TRACE_1 << "connected: " << the_uart->description();
        });
        return ret;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    Serial::Serial(boost::asio::io_service &io, receive_cb_t cb):
    m_impl(new SerialImpl(io, cb))
    {
    
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    Serial::~Serial()
    {
        close();
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    std::vector<std::string> Serial::device_list()
    {
        std::vector<std::string> ret;
        std::vector<std::string> search_patterns = {"tty.usb", "ttyACM", "ttyUSB"};
        
        for(const auto &path : fs::get_directory_entries("/dev"))
        {
            for(const auto &p : search_patterns)
            {
                if(path.find(p) != string::npos){ ret.push_back(path); break; }
            }
        }
        return ret;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    bool Serial::open()
    {
        bool ret = false;
        for(const auto &d : device_list()){ if(open(d)){ ret = true; break; } }
        return ret;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    bool Serial::open(const std::string &the_name, int the_baudrate)
    {
        close();
        boost::asio::serial_port_base::baud_rate br(the_baudrate);
        boost::asio::serial_port_base::flow_control flow_control;
        boost::asio::serial_port_base::parity parity;
        boost::asio::serial_port_base::stop_bits stop_bits;
        boost::asio::serial_port_base::character_size char_size;
        
        try
        {
            m_impl->m_serial_port.open(the_name);
            m_impl->m_serial_port.set_option(br);
            m_impl->m_serial_port.set_option(flow_control);
            m_impl->m_serial_port.set_option(parity);
            m_impl->m_serial_port.set_option(stop_bits);
            m_impl->m_serial_port.set_option(char_size);
            m_impl->m_device_name = the_name;
            if(m_impl->m_connect_cb){ m_impl->m_connect_cb(shared_from_this()); }
            async_read_bytes();
            return true;
        }
        catch(boost::system::system_error &e)
        {
            LOG_WARNING << e.what() << " (" << description() << ")";
        }
        return false;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::close()
    {
        if(is_open())
        {
            try{ m_impl->m_serial_port.close(); }
            catch(boost::system::system_error &e){ LOG_WARNING << e.what(); }
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    bool Serial::is_open() const
    {
        return m_impl->m_serial_port.is_open();
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    size_t Serial::read_bytes(void *buffer, size_t sz)
    {
        std::unique_lock<std::mutex> lock(m_impl->m_mutex);
        size_t num_bytes = std::min(m_impl->m_buffer.size(), sz);
        std::copy(m_impl->m_buffer.begin(), m_impl->m_buffer.begin() + num_bytes,
                  (uint8_t*)buffer);
        auto tmp = std::vector<uint8_t>(m_impl->m_buffer.begin() + num_bytes,
                                        m_impl->m_buffer.end());
        m_impl->m_buffer.assign(tmp.begin(), tmp.end());
        return num_bytes;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    size_t Serial::write_bytes(const void *buffer, size_t sz)
    {
//        try{ return boost::asio::write(m_impl->m_serial_port, boost::asio::buffer(buffer, sz)); }
//        catch(boost::system::system_error &e){ LOG_WARNING << e.what(); }
        async_write_bytes(buffer, sz);
        return sz;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::async_read_bytes()
    {
        auto weak_self = std::weak_ptr<Serial>(shared_from_this());
        auto impl_cp = m_impl;
        
        m_impl->m_rec_buffer.resize(512);
        m_impl->m_serial_port.async_read_some(boost::asio::buffer(m_impl->m_rec_buffer),
                                              [weak_self, impl_cp](const boost::system::error_code& error,
                                                                   std::size_t bytes_transferred)
        {
            auto self = weak_self.lock();
            
            if(!error)
            {
                if(bytes_transferred)
                {
                    LOG_TRACE_3 << "received " << bytes_transferred << " bytes";
              
                    if(impl_cp->m_receive_cb)
                    {
                        std::vector<uint8_t> datavec(impl_cp->m_rec_buffer.begin(),
                                                     impl_cp->m_rec_buffer.begin() + bytes_transferred);
                        impl_cp->m_receive_cb(self, datavec);
                    }
                    else
                    {
                        std::unique_lock<std::mutex> lock(impl_cp->m_mutex);
                        std::copy(impl_cp->m_rec_buffer.begin(),
                                  impl_cp->m_rec_buffer.begin() + bytes_transferred,
                                  std::back_inserter(impl_cp->m_buffer));
                    }
                }
                if(self){ self->async_read_bytes(); }
            }
            else
            {
                switch(error.value())
                {
                    case boost::asio::error::eof:
                    case boost::asio::error::connection_reset:
                    case boost::system::errc::no_such_device_or_address:
                        LOG_TRACE_1 << "disconnected: " << impl_cp->m_device_name;
                        if(impl_cp->m_disconnect_cb && self){ impl_cp->m_disconnect_cb(self); }
                        break;
                        
                    case boost::asio::error::operation_aborted:
                    default:
                        LOG_TRACE_2 << error.message() << " (" << error.value() << ")";
                        break;
                }
            }
        });
    };
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::async_write_bytes(const void *buffer, size_t sz)
    {
        std::vector<uint8_t> bytes((uint8_t*)buffer, (uint8_t*)buffer + sz);
        
        boost::asio::async_write(m_impl->m_serial_port, boost::asio::buffer(bytes),
                                 [bytes](const boost::system::error_code& error,
                                         std::size_t bytes_transferred)
        {
            if(error){ LOG_ERROR << error.message(); }
            else if(bytes_transferred < bytes.size())
            {
                LOG_WARNING << "not all bytes written";
            }
        });
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    size_t Serial::available() const
    {
        return m_impl->m_buffer.size();
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    std::string Serial::description() const
    {
        return m_impl->m_device_name;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::drain()
    {
        m_impl->m_serial_port.cancel();
        m_impl->m_buffer.clear();
        async_read_bytes();
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::set_receive_cb(receive_cb_t the_cb)
    {
        m_impl->m_receive_cb = the_cb;
        
        // we have some buffered data -> deliver it to the newly attached callback
        if(m_impl->m_receive_cb && !m_impl->m_buffer.empty())
        {
            std::unique_lock<std::mutex> lock(m_impl->m_mutex);
            m_impl->m_receive_cb(shared_from_this(), std::vector<uint8_t>(m_impl->m_buffer.begin(),
                                                                          m_impl->m_buffer.end()));
            m_impl->m_buffer.clear();
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::set_connect_cb(connection_cb_t the_cb)
    {
        m_impl->m_connect_cb = the_cb;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::set_disconnect_cb(connection_cb_t the_cb)
    {
        m_impl->m_disconnect_cb = the_cb;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
}
