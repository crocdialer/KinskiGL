#include "Logger.h"
#include "Exception.h"
#include "file_functions.h"

#include <iostream>
#include <vector>

namespace kinski {
    
    const size_t SEVERITIES = 9;
    const char * SeverityName[] = {"PRINT","TESTRESULT","FATAL", "ERROR", "WARNING", "INFO",
        "DEBUG", "TRACE", "DISABLED", 0};
    const char * const LOG_MODULE_VERBOSITY_ENV = "AC_LOG_MODULE_VERBOSITY";
    const char * const LOG_GLOBAL_VERBOSITY_ENV = "AC_LOG_VERBOSITY";
    
    Logger *Logger::s_instance = NULL;
    
    Logger* Logger::get()
    {
        if(!s_instance)
            s_instance = new Logger;
        
        return s_instance;
    }
    
    Logger::Logger():
    _myTopLevelLogTag(""),
    _myGlobalSeverity(SEV_INFO)
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
        _myGlobalSeverity = theSeverity;
    }

    /**
    return true if theSeverity is higher (numerically smaller) than the verbosity setting
    a different verbosity can be defined for any id range in any module; if there are different
    verbosity settings for an overlapping id region in the same module, the setting for the
    smallest id-range takes precedence.
    */
    bool Logger::ifLog(Severity theSeverity, const char *theModule, int theId)
    {
        if (!_mySeveritySettings.empty())
        {
            Severity mySeverity = _myGlobalSeverity;
            const std::string myModule(theModule); // remove everything before the last backslash
            
            
            // find all setting for a particular module
            std::multimap<std::string,ModuleSeverity>::const_iterator myLowerBound =
                _mySeveritySettings.lower_bound(myModule);
            if (myLowerBound != _mySeveritySettings.end()) {
                std::multimap<std::string,ModuleSeverity>::const_iterator myUpperBound =
                    _mySeveritySettings.upper_bound(myModule);
    
                // find smallest range containing theId with matching module name
                unsigned int myRange = std::numeric_limits<unsigned int>::max();
                for (std::multimap<std::string,ModuleSeverity>::const_iterator myIter = myLowerBound;
                    myIter != myUpperBound; ++myIter)
                {
                    if (myIter->first == myModule) {
                        int myMinId = myIter->second.myMinId;
                        int myMaxId = myIter->second.myMaxId;
                        if (theId >= myMinId && theId <= myMaxId)
                        {
                            unsigned int myNewRange = myMaxId - myMinId;
                            if (myNewRange < myRange)
                            {
                                mySeverity = myIter->second.mySeverity;
                                myRange = myNewRange;
                            }
                        }
                    }
                }
            }
            return theSeverity <= mySeverity;
        }
        return theSeverity <= _myGlobalSeverity;
    }

    void Logger::log(Severity theSeverity, const char * theModule, int theId,
                     const std::string & theText)
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
            case SEV_TESTRESULT :
                __android_log_print(ANDROID_LOG_INFO, myLogTag.c_str(), myText.str().c_str());//__VA_ARGS__)
                break;
            default:
                throw Exception("Unknown logger severity");
                break;
        }
        #endif

    }
};
