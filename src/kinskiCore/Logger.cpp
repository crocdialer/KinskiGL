#include "Logger.h"
#include "Exception.h"
#include "file_functions.h"

#include <iostream>
#include <limits>

namespace kinski {
    
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
            myText << " [" << lastFileNamePart(theModule) << " at:" << theId << "]";
        }

        switch (theSeverity)
        {
            case SEV_TRACE:
                std::cout << "TRACE: " << myText.str() << std::endl;
                break;
            case SEV_DEBUG:
                std::cout << "DEBUG: " << myText.str() << std::endl;
                break;
            case SEV_INFO:
                std::cout << "INFO: " << myText.str() << std::endl;
                break;
            case SEV_WARNING:
                std::cout << "WARNING: " << myText.str() << std::endl;
                break;
            case SEV_PRINT:
                std::cout << myText.str() << std::endl;
                break;
            case SEV_ERROR:
                std::cerr << "ERROR: " << myText.str() << std::endl;
                break;
            default:
                throw Exception("Unknown logger severity");
                break;
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
};
