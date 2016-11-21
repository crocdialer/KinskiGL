//#include "boost/asio.hpp"
#include "core/Serial.hpp"
#include "sensors.h"

namespace kinski{ namespace sensors{

#define QUERY_ID_CMD "ID"
    
void scan_for_devices(boost::asio::io_service &io, device_cb_t the_device_cb)
{
    for(const auto &dev : Serial::device_list())
    {
        auto s = Serial::create(io);
        
        if(s->open(dev))
        {
            s->set_receive_cb([the_device_cb, s](UARTPtr the_uart, const std::vector<uint8_t> &the_data)
            {
                // parse response, find returned ID
                std::string id_str;
                auto lines = split(string(the_data.begin(), the_data.end()), '\n');
                
                for(const auto &line : lines)
                {
                    auto tokens = split(line, ' ');
                    
                    for(uint32_t i = 0; i < tokens.size(); ++i)
                    {
                        if(tokens[i] == QUERY_ID_CMD && i < tokens.size() - 1)
                        {
                            id_str = tokens[i + 1];
                            break;
                        }
                    }
                }
                
                if(the_device_cb && !id_str.empty())
                {
                    // we got the id now -> stop listening
                    the_uart->set_receive_cb(UART::receive_cb_t());
                    
                    the_device_cb(id_str, the_uart);
                }
            });
            s->write(QUERY_ID_CMD + string("\n"));
        }
    }
}
    
    
}}
