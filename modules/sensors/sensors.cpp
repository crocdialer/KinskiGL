#include <boost/asio.hpp>
#include "core/Serial.hpp"
#include "core/Timer.hpp"
#include "sensors.h"

namespace kinski{ namespace sensors{

#define QUERY_ID_CMD "ID"
#define QUERY_TIME_OUT 2.0

void scan_for_serials(boost::asio::io_service &io, device_cb_t the_device_cb)
{
    io.post([&io, the_device_cb]
    {
        for(const auto &dev : Serial::device_list())
        {
            auto serial = Serial::create(io);
            
            if(serial->open(dev))
            {
                Timer timer(io, [serial](){ serial->set_receive_cb(); });
                timer.expires_from_now(QUERY_TIME_OUT);
                
                serial->set_receive_cb([&io, the_device_cb, serial, timer]
                                       (UARTPtr the_uart, const std::vector<uint8_t> &the_data)
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
                        io.post([the_device_cb, id_str, the_uart]()
                        {
                            the_device_cb(id_str, the_uart);
                        });
                       
                        // we got the id now -> stop listening
                        the_uart->set_receive_cb();
                    }
                });
                serial->write(QUERY_ID_CMD + string("\n"));
            }
        }
    });
}
    
void scan_for_devices(boost::asio::io_service &io, device_cb_t the_device_cb)
{
    scan_for_serials(io, the_device_cb);
}


}}
