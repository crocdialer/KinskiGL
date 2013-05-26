// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Logger.h"
#include "Exception.h"
#include "file_functions.h"
#include <boost/thread.hpp>

#include <iostream>
#include <limits>

namespace kinski {
    
    const std::string currentDateTime()
    {
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
        return buf;
    }
    
    Logger *Logger::s_instance = NULL;
    
    Logger* Logger::get()
    {
        if(!s_instance)
            s_instance = new Logger;
        
        return s_instance;
    }
    
    Logger::Logger():
    _myTopLevelLogTag(""),
    m_globalSeverity(SEV_INFO)
    {
        clear_streams();
    }

    Logger::~Logger()
    {
    
    }

    void Logger::setLoggerTopLevelTag(const std::string &theTagString)
    {
        _myTopLevelLogTag = theTagString;
    }
    
    void Logger::setSeverity(const Severity theSeverity)
    {
        m_globalSeverity = theSeverity;
    }

    /**
    return true if theSeverity is higher (numerically smaller) than the verbosity setting
    a different verbosity can be defined for any id range in any module; if there are different
    verbosity settings for an overlapping id region in the same module, the setting for the
    smallest id-range takes precedence.
    */
    bool Logger::ifLog(Severity theSeverity, const char *theModule, int theId)
    {
        if (!m_severitySettings.empty())
        {
            Severity mySeverity = m_globalSeverity;
            const std::string myModule(theModule); // remove everything before the last backslash
            
            // find all setting for a particular module
            std::multimap<std::string,ModuleSeverity>::const_iterator myLowerBound =
                m_severitySettings.lower_bound(myModule);
            if (myLowerBound != m_severitySettings.end()) {
                std::multimap<std::string,ModuleSeverity>::const_iterator myUpperBound =
                    m_severitySettings.upper_bound(myModule);
    
                // find smallest range containing theId with matching module name
                unsigned int myRange = std::numeric_limits<unsigned int>::max();
                for (std::multimap<std::string,ModuleSeverity>::const_iterator myIter = myLowerBound;
                    myIter != myUpperBound; ++myIter)
                {
                    if (myIter->first == myModule) {
                        int myMinId = myIter->second.m_minId;
                        int myMaxId = myIter->second.m_maxId;
                        if (theId >= myMinId && theId <= myMaxId)
                        {
                            unsigned int myNewRange = myMaxId - myMinId;
                            if (myNewRange < myRange)
                            {
                                mySeverity = myIter->second.m_severity;
                                myRange = myNewRange;
                            }
                        }
                    }
                }
            }
            return theSeverity <= mySeverity;
        }
        return theSeverity <= m_globalSeverity;
    }

    void Logger::log(Severity theSeverity, const char *theModule, int theId,
                     const std::string &theText)
    {
        std::string myLogTag(_myTopLevelLogTag);
        std::ostringstream myText;
        myText << theText;
        if (theSeverity > SEV_PRINT)
        {
            myText<<" [" << getFilenamePart(theModule) << " at:" << theId << "]";
            myText<<" [thread-id: "<<boost::this_thread::get_id() <<"]";
        }
        
        std::string currentTimeString = currentDateTime();
        std::stringstream stream;
        
        switch (theSeverity)
        {
            case SEV_TRACE:
                stream << currentTimeString<<" TRACE: " << myText.str() << std::endl;
                break;
            case SEV_DEBUG:
                stream << currentTimeString<<" DEBUG: " << myText.str() << std::endl;
                break;
            case SEV_INFO:
                stream << currentTimeString<<" INFO: " << myText.str() << std::endl;
                break;
            case SEV_WARNING:
                stream << currentTimeString<<" WARNING: " << myText.str() << std::endl;
                break;
            case SEV_PRINT:
                stream << myText.str() << std::endl;
                break;
            case SEV_ERROR:
                stream << currentTimeString<<" ERROR: " << myText.str() << std::endl;
                break;
            default:
                throw Exception("Unknown logger severity");
                break;
        }
        
        // give log string to outstreams
        std::list<std::ostream*>::iterator stream_it = m_out_streams.begin();
        for (; stream_it != m_out_streams.end(); ++stream_it)
        {
            (**stream_it)<<stream.str();
        }
        
        #if ANDROID
        switch (theSeverity) {
            case SEV_TRACE :
                __android_log_print(ANDROID_LOG_VERBOSE, myLogTag.c_str(), myText.str().c_str());//__VA_ARGS__)
                break;
            case SEV_DEBUG :
                __android_log_print(ANDROID_LOG_DEBUG, myLogTag.c_str(), myText.str().c_str());//__VA_ARGS__)
                break;
            case SEV_WARNING :
                __android_log_print(ANDROID_LOG_WARN, myLogTag.c_str(), myText.str().c_str());//__VA_ARGS__)
                break;
            case SEV_INFO :
            case SEV_PRINT :
                __android_log_print(ANDROID_LOG_INFO, myLogTag.c_str(), myText.str().c_str());//__VA_ARGS__)
                break;
            case SEV_ERROR :
                __android_log_print(ANDROID_LOG_ERROR, myLogTag.c_str(), myText.str().c_str());//__VA_ARGS__)
                break;
            default:
                throw Exception("Unknown logger severity");
                break;
        }
        #endif

    }
    
    void Logger::add_outstream(std::ostream *the_stream)
    {
        m_out_streams.push_back(the_stream);
    }
    
    void Logger::clear_streams()
    {
        m_out_streams.clear();
        m_out_streams.push_back(&std::cout);
    }
};
