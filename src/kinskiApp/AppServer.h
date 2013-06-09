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
#include "kinskiGL/SerializerGL.h"//temp -> use delegate instead
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

namespace kinski
{
    class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
    {
    public:
        typedef boost::shared_ptr<tcp_connection> Ptr;
        
        static Ptr create(boost::asio::io_service& io_service)
        {
            return Ptr(new tcp_connection(io_service));
        }
        
        tcp::socket& socket(){ return m_socket; }
        
        void start();
        
    private:
        tcp_connection(boost::asio::io_service& io_service);
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        
        tcp::socket m_socket;
        std::string m_message;
    };
    
    class AppServer
    {
    public:
        AppServer(kinski::App::Ptr the_app);
        
    private:
        
        void start_accept();
        void handle_accept(tcp_connection::Ptr new_connection,
                           const boost::system::error_code& error);
        
        kinski::App::Ptr m_app;
        tcp::acceptor m_acceptor;
    };
}// namespace

#endif /* defined(__kinskiGL__AppServer__) */
