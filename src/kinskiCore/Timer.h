//
//  Timer.h
//  kinskiGL
//
//  Created by Croc Dialer on 27/07/14.
//
//

#ifndef __kinskiGL__Timer__
#define __kinskiGL__Timer__

#include "Definitions.h"

namespace kinski
{
    class Timer
    {
        typedef std::function<void()> Callback;
        
        struct timer_impl;
        std::shared_ptr<timer_impl> m_impl;
        
    public:
        Timer();
        Timer(float secs, boost::asio::io_service &io, Callback cb);
        Timer(float secs, Callback cb);
        
        void expires_from_now(float secs);
        float expires_from_now() const;
        void set_callback(Callback cb);
    };
}


#endif /* defined(__kinskiGL__Timer__) */
