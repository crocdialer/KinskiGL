//
//  networking.h
//  kinskiGL
//
//  Created by Fabian on 18/01/14.
//
//

#ifndef __kinskiGL__networking__
#define __kinskiGL__networking__

#include "Definitions.h"

// forward declarations
namespace boost
{
    namespace asio
    {
        class io_service;
    }
}

namespace kinski
{
    namespace net
    {
        
        std::string local_ip(bool ipV6 = false);
        
        KINSKI_API void async_send_udp(boost::asio::io_service& io_service,
                                       const std::vector<uint8_t> &bytes,
                                       const std::string &ip,
                                       int port);
        
        KINSKI_API class udp_server
        {
        public:
            
            typedef std::function<void (const std::vector<uint8_t>&)> receive_function;
            
            udp_server();
            udp_server(boost::asio::io_service& io_service, receive_function f = receive_function());
            
            KINSKI_API void start_receive(int port);
            KINSKI_API void set_receive_function(receive_function f);
            
        private:
            std::shared_ptr<class udp_server_impl> m_impl;
        };
    }// namespace kinski
    
}// namespace kinski

#endif /* defined(__kinskiGL__networking__) */
