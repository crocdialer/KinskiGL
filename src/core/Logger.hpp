// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <sstream>
#include "core/core.hpp"

namespace kinski {
    enum Severity { SEV_DISABLED = 0, SEV_PRINT = 1, SEV_FATAL = 2, SEV_ERROR = 3, SEV_WARNING = 4,
                    SEV_INFO = 5, SEV_DEBUG = 6, SEV_TRACE = 7};

class KINSKI_API Logger
{
    struct ModuleSeverity
    {
        ModuleSeverity() {}
        ModuleSeverity(Severity theSeverity,int theMinId, int myMaxId)
            : m_severity(theSeverity), m_minId(theMinId), m_maxId(myMaxId)
        {}

        Severity m_severity;
        int m_minId;
        int m_maxId;
    };
 public:
    
    virtual ~Logger();
    static Logger* get();

    /**
    Used to detect if a message should be logged depending on its severity and the logger severity settings.
    returns true if theSeverity is higher (numerically smaller) than the verbosity setting
    a different verbosity can be defined for any id range in any module; if there are different
    verbosity settings for an overlapping id region in the same module, the setting for the
    smallest id-range takes precedence.
    */
    bool ifLog(Severity theSeverity, const char *theModule, int theId);
    void log(Severity theSeverity, const char *theModule, int theId, const std::string &theText);
    void setLoggerTopLevelTag(const std::string & theTagString);

    void setSeverity(const Severity theSeverity);
    Severity getSeverity() const { return m_globalSeverity; };
    
    void add_outstream(std::ostream *the_stream);
    void clear_streams();
    
    bool use_time_stamp() const{ return m_use_timestamp;};
    void set_use_time_stamp(bool b){m_use_timestamp = b;}
    
    bool use_thread_id() const{ return m_use_thread_id;};
    void set_use_thread_id(bool b){m_use_thread_id = b;}

    private:
    
        Logger();
        static Logger *s_instance;
    
        std::string _myTopLevelLogTag;
        Severity m_globalSeverity;
        std::multimap<std::string, ModuleSeverity> m_severitySettings;
    
        std::list<std::ostream*> m_out_streams;
    
        bool m_use_timestamp, m_use_thread_id;
};

/**
This class is used to collect the output and deliver it to the Logger on destruction
*/
class MessagePort
{
 public:
    MessagePort(Severity theSeverity, const char *theModule, int theId)
        : m_severity(theSeverity), m_module(theModule), m_Id(theId)
    {}
    ~MessagePort()
    {
        Logger::get()->log(m_severity, m_module, m_Id, m_stream.str());
    }

    inline std::ostringstream& getStream()
    {
        return m_stream;
    }
 
 private:
    
    std::ostringstream m_stream;
    const Severity m_severity;
    const char *m_module;
    const int m_Id;
};

#define KINSKI_LOG_CHECK(SEVERITY,MODULE,MSGID) kinski::Logger::get()->ifLog(SEVERITY,MODULE,MSGID) \
    && (kinski::MessagePort(SEVERITY,MODULE,MSGID).getStream())

#define LOG_INFO KINSKI_LOG_CHECK(kinski::SEV_INFO, __FILE__ ,__LINE__)
#define LOG_TRACE KINSKI_LOG_CHECK(kinski::SEV_TRACE, __FILE__ ,__LINE__)
#define LOG_DEBUG KINSKI_LOG_CHECK(kinski::SEV_DEBUG, __FILE__ ,__LINE__)
#define LOG_PRINT KINSKI_LOG_CHECK(kinski::SEV_PRINT, __FILE__ ,__LINE__)
#define LOG_ERROR KINSKI_LOG_CHECK(kinski::SEV_ERROR, __FILE__ ,__LINE__)
#define LOG_WARNING KINSKI_LOG_CHECK(kinski::SEV_WARNING, __FILE__ ,__LINE__)

#define LOG_INFO_IF(b) b && LOG_INFO
#define LOG_TRACE_IF(b) b && LOG_TRACE
#define LOG_DEBUG_IF(b) b && LOG_DEBUG
#define LOG_PRINT_IF(b) b && LOG_PRINT
#define LOG_ERROR_IF(b) b && LOG_ERROR
#define LOG_WARNING_IF(b) b && LOG_WARNING

}//namespace