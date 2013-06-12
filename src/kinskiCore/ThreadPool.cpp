//
//  ThreadPool.cpp
//  kinskiGL
//
//  Created by Fabian on 6/12/13.
//
//
#include "ThreadPool.h"
#include "Logger.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

namespace kinski
{
    ThreadPool::ThreadPool(size_t num):
    m_io_service(new boost::asio::io_service()),
    m_io_work(new boost::asio::io_service::work(*m_io_service)),
    m_threads(new boost::thread_group())
    {
        set_num_threads(num);
    }
    
    ThreadPool::~ThreadPool()
    {
        m_io_work.reset();
        m_io_service->stop();
        if(m_threads)
        {
            try{m_threads->join_all();}
            catch(std::exception &e){LOG_ERROR<<e.what();}
        }
    }
    
    int ThreadPool::get_num_threads()
    {
        return m_threads->size();
    }
    
    void ThreadPool::set_num_threads(int num)
    {
        if(m_threads && num > 0)
        {
            try
            {
                m_threads->join_all();
                
                for(uint32_t i = 0; i < num; ++i)
                {
                    m_threads->create_thread(boost::bind(&boost::asio::io_service::run, m_io_service));
                }
            }
            catch(std::exception &e){LOG_ERROR<<e.what();}
        }
    }
}