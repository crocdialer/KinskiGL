//
//  RemoteControl.h
//  kinskiGL
//
//  Created by Croc Dialer on 06/10/14.
//
//

#ifndef __kinskiGL__RemoteControl__
#define __kinskiGL__RemoteControl__

#include "core/Definitions.h"
#include "core/Component.h"
#include "core/networking.h"

namespace kinski
{
    class RemoteControl;
    typedef std::unique_ptr<RemoteControl> RemoteControlPtr;
    
    class RemoteControl
    {
    public:
        
        RemoteControl(){};
        RemoteControl(boost::asio::io_service &io, const std::list<Component::Ptr> &the_list,
                      uint16_t the_port = 33333);
        
        void start_listen(uint16_t port = 33333);
        void stop_listen();
        
    private:
        
        void new_connection_cb(net::tcp_connection_ptr con);
        void receive_cb(net::tcp_connection_ptr rec_con,
                        const std::vector<uint8_t>& response);
        
        std::list<Component::Ptr> lock_components();
        
        //!
        net::tcp_server m_tcp_server;
        
        //!
        std::list<Component::WeakPtr> m_components;
        
        //!
        std::vector<net::tcp_connection_ptr> m_tcp_connections;
    };
}

#endif /* defined(__kinskiGL__RemoteControl__) */
