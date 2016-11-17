#include "Serial.hpp"
#include "core/file_functions.hpp"
#include "core/CircularBuffer.hpp"
#include "boost/asio.hpp"
#include <boost/asio/serial_port.hpp>

namespace kinski
{
    struct SerialImpl
    {
        std::string m_device_name;
        boost::asio::serial_port m_serial_port;
        Serial::receive_cb_t m_receive_cb;
        CircularBuffer<uint8_t> m_buffer = CircularBuffer<uint8_t>(2048);
        std::mutex m_mutex;
        
        SerialImpl(boost::asio::io_service &io, Serial::receive_cb_t rec_cb):
        m_serial_port(io),
        m_receive_cb(rec_cb){}
    };
    
    ///////////////////////////////////////////////////////////////////////////////
    
    SerialPtr Serial::create(boost::asio::io_service &io, receive_cb_t cb)
    {
        return SerialPtr(new Serial(io, cb));
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    Serial::Serial(boost::asio::io_service &io, receive_cb_t cb):
    m_impl(new SerialImpl(io, cb))
    {
    
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    std::vector<std::string> device_list()
    {
        std::vector<std::string> ret;
        std::vector<std::string> search_patterns = {"cu.", "tty.", "ttyACM", "ttyUSB"};
        
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
    
    bool Serial::setup()
    {
        auto devices = device_list();
        if(!devices.empty()){ return setup(devices.front(), 57600); }
        return false;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    bool Serial::setup(const std::string &the_name, int the_baudrate)
    {
        close();
        boost::asio::serial_port_base::baud_rate br(the_baudrate);
        boost::asio::serial_port_base::flow_control flow_control;
        boost::asio::serial_port_base::parity parity;
        boost::asio::serial_port_base::stop_bits stop_bits;
        boost::asio::serial_port_base::character_size char_size;
        
        try
        {
            m_impl->m_serial_port.set_option(br);
            m_impl->m_serial_port.set_option(flow_control);
            m_impl->m_serial_port.set_option(parity);
            m_impl->m_serial_port.set_option(stop_bits);
            m_impl->m_serial_port.set_option(char_size);
            m_impl->m_serial_port.open(the_name);
            m_impl->m_device_name = the_name;
            return true;
        }
        catch(boost::system::system_error &e){ LOG_WARNING << e.what(); }
        return false;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::close()
    {
        try{ m_impl->m_serial_port.close(); }
        catch(boost::system::system_error &e){ LOG_WARNING << e.what(); }
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    bool Serial::is_initialized() const
    {
        return m_impl->m_serial_port.is_open();
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    size_t Serial::read_bytes(void *buffer, size_t sz)
    {
        size_t num_bytes = std::min(available(), sz);
//        std::copy(m_impl->m_buffer.begin(),  m_impl->m_buffer.end(), buffer);
        return 0;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    size_t Serial::write_bytes(const void *buffer, size_t sz)
    {
        try{ return boost::asio::write(m_impl->m_serial_port, boost::asio::buffer(buffer, sz)); }
        catch(boost::system::system_error &e){ LOG_WARNING << e.what(); }
        return 0;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::async_write_bytes(const void *buffer, size_t sz)
    {
        auto impl = m_impl;
        std::vector<uint8_t> bytes((uint8_t*)buffer, (uint8_t*)buffer + sz);
        
        boost::asio::async_write(m_impl->m_serial_port,
                                 boost::asio::buffer(bytes),
         [impl, bytes](const boost::system::error_code& error,
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
        
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::flush(bool flushIn , bool flushOut)
    {
        m_impl->m_buffer.clear();
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::async_read_bytes()
    {
        
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void Serial::set_receive_cb(receive_cb_t the_cb)
    {
        m_impl->m_receive_cb = the_cb;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
}
