//
//  AppServer.h
//  kinskiGL
//
//  Created by Fabian on 6/8/13.
//
//

#ifndef __kinskiGL__AppServer__
#define __kinskiGL__AppServer__

#include "App.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

namespace kinski
{
    class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
    {
    public:
        typedef boost::shared_ptr<tcp_connection> Ptr;
        
        static Ptr create(boost::asio::io_service& io_service, kinski::Component::WeakPtr the_component)
        {
            return Ptr(new tcp_connection(io_service, the_component));
        }
        
        tcp::socket& socket(){ return m_socket; }
        
        void start();
        
    private:
        tcp_connection(boost::asio::io_service& io_service, kinski::Component::WeakPtr the_component);
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        
        tcp::socket m_socket;
        std::string m_message;
        kinski::Component::WeakPtr m_component;
    };
    
    class AppServer
    {
    public:
        AppServer(kinski::App::Ptr the_app);
        
        void start();
        void stop();
        
    private:
        
        void start_accept();
        void handle_accept(tcp_connection::Ptr new_connection,
                           const boost::system::error_code& error);
        
        kinski::App::WeakPtr m_app;
        tcp::acceptor m_acceptor;
    };
}// namespace

#endif /* defined(__kinskiGL__AppServer__) */
