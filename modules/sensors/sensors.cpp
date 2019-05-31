#include <boost/asio.hpp>
#include "crocore/Serial.hpp"
#include "crocore/networking.hpp"
#include "crocore/Timer.hpp"
#include "sensors.h"

using namespace crocore;

namespace kinski{ namespace sensors{

#define QUERY_ID_CMD "ID"
#define QUERY_TIME_OUT 5.0

void query_device(crocore::ConnectionPtr the_device, crocore::io_service_t &io, device_cb_t the_device_cb)
{
//    crocore::Timer timer(io, [the_device](){ the_device->set_receive_cb({}); });
//    timer.expires_from_now(QUERY_TIME_OUT);

    the_device->set_receive_cb([&io, the_device_cb, the_device]
                               (crocore::ConnectionPtr the_device, const std::vector<uint8_t> &the_data)
    {
        // parse response, find returned ID
        std::string id_str;
        auto lines = crocore::split(std::string(the_data.begin(), the_data.end()), '\n');

        for(const auto &line : lines)
        {
            auto tmp = crocore::split(line, ' ');
            std::vector<std::string> tokens(tmp.begin(), tmp.end());

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
            the_device->set_receive_cb({});
        }
    });
    the_device->write(QUERY_ID_CMD + std::string("\n"));
}

void scan_for_serials(crocore::io_service_t &io, device_cb_t the_device_cb, uint32_t the_baudrate)
{
    io.post([&io, the_device_cb, the_baudrate]
    {
        auto connected_devices = Serial::connected_devices();

        for(const auto &dev : Serial::device_list())
        {
            if(connected_devices.find(dev) == connected_devices.end())
            {
                io.post([&io, the_device_cb, dev, the_baudrate]
                {
                    auto serial = Serial::create(io);

                    if(serial->open(dev, the_baudrate)){ query_device(serial, io, the_device_cb); }
                });
            }
        }
    });
}

void scan_for_devices(io_service_t &io, device_cb_t the_device_cb)
{
    io.post([&io, the_device_cb]{ scan_for_serials(io, the_device_cb); });
}


}}
