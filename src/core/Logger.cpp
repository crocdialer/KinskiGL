// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <iostream>
#include <limits>
#include <thread>
#include <mutex>
#include <fstream>
#include <cstdarg>
#include "file_functions.hpp"
#include "ThreadPool.hpp"
#include "Logger.hpp"

namespace kinski {
    
    namespace
    {
        std::mutex mutex;
        std::ofstream log_file_stream;
        kinski::ThreadPool thread_pool;
    }
    
    const std::string currentDateTime()
    {
        time_t now = time(0);
        struct tm  tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
        return buf;
    }
    
    Logger *Logger::s_instance = nullptr;
    
    Logger* Logger::get()
    {
        if(!s_instance){ s_instance = new Logger(); }
        return s_instance;
    }
    
    Logger::Logger():
    m_global_severity(Severity::INFO),
    m_use_timestamp(true),
    m_use_thread_id(false)
    {
        clear_streams();
        add_outstream(&std::cout);
    }

    Logger::~Logger()
    {
        log_file_stream.close();
    }
    
    void Logger::set_severity(const Severity theSeverity)
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        // change state
        m_global_severity = theSeverity;
    }

    /**
    return true if theSeverity is higher (numerically smaller) than the verbosity setting
    a different verbosity can be defined for any id range in any module; if there are different
    verbosity settings for an overlapping id region in the same module, the setting for the
    smallest id-range takes precedence.
    */
    bool Logger::if_log(Severity theSeverity, const char *theModule, int theId)
    {
//        if (!m_severitySettings.empty())
//        {
//            Severity mySeverity = m_globalSeverity;
//            const std::string myModule(theModule); // remove everything before the last backslash
//            
//            // find all setting for a particular module
//            std::multimap<std::string,ModuleSeverity>::const_iterator myLowerBound =
//                m_severitySettings.lower_bound(myModule);
//            if (myLowerBound != m_severitySettings.end()) {
//                std::multimap<std::string,ModuleSeverity>::const_iterator myUpperBound =
//                    m_severitySettings.upper_bound(myModule);
//    
//                // find smallest range containing theId with matching module name
//                unsigned int myRange = std::numeric_limits<unsigned int>::max();
//                for (std::multimap<std::string,ModuleSeverity>::const_iterator myIter = myLowerBound;
//                    myIter != myUpperBound; ++myIter)
//                {
//                    if (myIter->first == myModule) {
//                        int myMinId = myIter->second.m_minId;
//                        int myMaxId = myIter->second.m_maxId;
//                        if (theId >= myMinId && theId <= myMaxId)
//                        {
//                            unsigned int myNewRange = myMaxId - myMinId;
//                            if (myNewRange < myRange)
//                            {
//                                mySeverity = myIter->second.m_severity;
//                                myRange = myNewRange;
//                            }
//                        }
//                    }
//                }
//            }
//            return theSeverity <= mySeverity;
//        }
        return theSeverity <= m_global_severity;
    }

    void Logger::log(Severity theSeverity, const char *theModule, int theId,
                     const std::string &theText)
    {
        std::stringstream stream;
        std::ostringstream postfix;
        postfix << theText;
        
        std::lock_guard<std::mutex> lock(mutex);
        
        if (theSeverity > Severity::PRINT)
        {
            if(m_use_timestamp){ stream << currentDateTime(); }
            postfix <<" [" << fs::get_filename_part(theModule) << " at:" << theId << "]";
            if(m_use_thread_id){ postfix << " [thread-id: "<< std::this_thread::get_id() <<"]"; }
        }
    
        switch (theSeverity)
        {
            case Severity::TRACE_1:
            case Severity::TRACE_2:
            case Severity::TRACE_3:
                stream <<" TRACE: " << postfix.str();
                break;
            case Severity::DEBUG:
                stream <<" DEBUG: " << postfix.str();
                break;
            case Severity::INFO:
                stream <<" INFO: " << postfix.str();
                break;
            case Severity::WARNING:
                stream <<" WARNING: " << postfix.str();
                break;
            case Severity::PRINT:
                stream << postfix.str();
                break;
            case Severity::ERROR:
                stream <<" ERROR: " << postfix.str();
                break;
            default:
                throw Exception("Unknown logger severity");
                break;
        }
        
        std::string log_str = stream.str();
        
        // pass log string to outstreams
        thread_pool.submit([this, log_str]()
        {
            for (auto &os : m_out_streams){ *os << log_str << std::endl; }
        });
    }
    
    void Logger::add_outstream(std::ostream *the_stream)
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        // change state
        m_out_streams.push_back(the_stream);
    }
    
    void Logger::remove_outstream(std::ostream *the_stream)
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        // change state
        m_out_streams.remove(the_stream);
    }
    
    void Logger::clear_streams()
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        // change state
        m_out_streams.clear();
    }
    
    bool Logger::use_log_file() const
    {
        return log_file_stream.is_open();
    }
    
    void Logger::set_use_log_file(bool b, const std::string &the_log_file)
    {
        if(b)
        {
            log_file_stream.open(the_log_file);
            add_outstream(&log_file_stream);
        }
        else
        {
            remove_outstream(&log_file_stream);
            log_file_stream.close();
        }
        
    }
    
    void log(Severity the_severity, const char *the_format_text, ...)
    {
        Logger *l = Logger::get();
        
        if(the_severity > l->severity()){ return; }
        
        const size_t buf_sz = 1024 * 2;
        char buf[buf_sz];
        va_list argptr;
        va_start(argptr, the_format_text);
        vsnprintf(buf, buf_sz, the_format_text, argptr);
        va_end(argptr);
        
        l->log(the_severity, "unknown module", 0, buf);
    }
};
