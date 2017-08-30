#include <boost/asio.hpp>
#include "core/Serial.hpp"
#include "core/Timer.hpp"
#include "sensors.h"

namespace kinski{ namespace sensors{

#define QUERY_ID_CMD "ID"
#define QUERY_TIME_OUT 5.0

void query_device(ConnectionPtr the_device, io_service_t &io, device_cb_t the_device_cb)
{
    Timer timer(io, [the_device](){ the_device->set_receive_cb(); });
    timer.expires_from_now(QUERY_TIME_OUT);

    the_device->set_receive_cb([&io, the_device_cb, the_device, timer]
                               (ConnectionPtr the_device, const std::vector<uint8_t> &the_data)
    {
        // parse response, find returned ID
        std::string id_str;
        auto lines = split(string(the_data.begin(), the_data.end()), '\n');
        
        for(const auto &line : lines)
        {
            auto tokens = split(line, ' ');
            
            for(uint32_t i = 0; i < tokens.size(); ++i)
            {
                if(tokens[i] == QUERY_ID_CMD && i < (tokens.size() - 1))
                {
                    id_str = tokens[i + 1];
                    break;
                }
            }
        }
        
        if(the_device_cb && !id_str.empty())
        {
            io.post([the_device_cb, id_str, the_device]()
            {
                the_device_cb(id_str, the_device);
            });
            
            // we got the id now -> stop listening
            the_device->set_receive_cb();
        }
    });
    the_device->write(QUERY_ID_CMD + string("\n"));
}
    
void scan_for_serials(io_service_t &io, device_cb_t the_device_cb)
{
    io.post([&io, the_device_cb]
    {
        auto connected_devices = Serial::connected_devices();
        
        for(const auto &dev : Serial::device_list())
        {
            if(connected_devices.find(dev) == connected_devices.end())
            {
                io.post([&io, the_device_cb, dev]
                {
                    auto serial = Serial::create(io);
                    
                    if(serial->open(dev)){ query_device(serial, io, the_device_cb); }
                });
            }
        }
    });
}
    
void scan_for_devices(io_service_t &io, device_cb_t the_device_cb)
{
    scan_for_serials(io, the_device_cb);
}


}}
