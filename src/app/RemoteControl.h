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
    class RemoteControl
    {
    public:
        
//        template <typename L>
//        RemoteControl(boost::asio::io_service &io, const L &component_collection,
//                      uint16_t the_port = 33333):
//        RemoteControl(io,
//                      std::list<Component::Ptr>(begin(component_collection),
//                                                end(component_collection)),
//                      the_port){}
        
        RemoteControl(){};
        RemoteControl(boost::asio::io_service &io, const std::list<Component::Ptr> &the_list,
                      uint16_t the_port = 33333);
        
    private:
        
        
        void new_connection_cb(net::tcp_connection_ptr con);
        
        void receive_cb(net::tcp_connection_ptr rec_con,
                        const std::vector<uint8_t>& response);
        
        std::list<Component::Ptr> lock_components(const std::list<Component::WeakPtr> &weak_components);
        
        //!
        net::tcp_server m_tcp_server;
        
        //!
        std::list<Component::WeakPtr> m_components;
        
        //!
        std::vector<net::tcp_connection_ptr> m_tcp_connections;
    };
}

#endif /* defined(__kinskiGL__RemoteControl__) */
