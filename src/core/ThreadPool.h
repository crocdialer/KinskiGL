//
//  ThreadPool.h
//  gl
//
//  Created by Fabian on 6/12/13.
//
//

#ifndef __gl__ThreadPool__
#define __gl__ThreadPool__

#include "core/Definitions.h"

// forward declared io_service
namespace boost
{
    namespace asio
    {
        class io_service;
    }
}

namespace kinski
{
    
    class ThreadPool
    {
    public:
        
        typedef std::function<void()> Task;
        
        ThreadPool(size_t num = 1);
        ~ThreadPool();
        
        void set_num_threads(int num);
        int get_num_threads();
        boost::asio::io_service& io_service();
        
        /*!
         * submit an arbitrary task to be processed by the threadpool
         */
        void submit(Task t);
        
    private:
        void join_all();
        
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
    
}//namespace

#endif /* defined(__gl__ThreadPool__) */
