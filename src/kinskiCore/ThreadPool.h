//
//  ThreadPool.h
//  kinskiGL
//
//  Created by Fabian on 6/12/13.
//
//

#ifndef __kinskiGL__ThreadPool__
#define __kinskiGL__ThreadPool__

#include <boost/asio.hpp>
#include "kinskiCore/Definitions.h"

namespace boost {
    class thread;
    class thread_group;
    namespace asio {
        class io_service;
    }
}

namespace kinski{
    
    class ThreadPool
    {
    public:
        ThreadPool(size_t num = 1);
        ~ThreadPool();
        
        boost::asio::io_service& io_service(){return *m_io_service;};
        void set_num_threads(int num);
        int get_num_threads();
        
        template<class F> void submit(const F &task){m_io_service->post(task);}
        
    private:
        std::shared_ptr<boost::asio::io_service> m_io_service;
        std::shared_ptr<void> m_io_work;
        std::shared_ptr<boost::thread_group> m_threads;
    };
    
}//namespace

#endif /* defined(__kinskiGL__ThreadPool__) */
