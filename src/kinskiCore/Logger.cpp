// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Logger.h"
#include "Exception.h"
#include "file_functions.h"

#include <iostream>
#include <vector>
//#ifdef iOS
//    #include <iostream>
//#elif ANDROID
//    #include <android/log.h>
//#endif

namespace kinski {
    
    const size_t SEVERITIES = 9;
    const char * SeverityName[] = {"PRINT","TESTRESULT","FATAL", "ERROR", "WARNING", "INFO",
        "DEBUG", "TRACE", "DISABLED", 0};
    const char * const LOG_MODULE_VERBOSITY_ENV = "AC_LOG_MODULE_VERBOSITY";
    const char * const LOG_GLOBAL_VERBOSITY_ENV = "AC_LOG_VERBOSITY";
    
    Logger *s_instance = NULL;
    
    Logger* get()
    {
        if(!s_instance)
            s_instance = new Logger;
        
        return s_instance;
    }
    
    Logger::Logger() : _myTopLevelLogTag("Unset"), _myGlobalSeverity(SEV_WARNING)
    {
    }

    Logger::~Logger() {}


    void Logger::setLoggerTopLevelTag(const std::string &theTagString)
    {
        _myTopLevelLogTag = theTagString;
    }
    
    void
    Logger::setSeverity(const Severity theSeverity) {
        _myGlobalSeverity = theSeverity;
        parseEnvModuleSeverity();
    }

    /**
    return true if theSeverity is higher (numerically smaller) than the verbosity setting
    a different verbosity can be defined for any id range in any module; if there are different
    verbosity settings for an overlapping id region in the same module, the setting for the
    smallest id-range takes precedence.
    */
    bool
    Logger::ifLog(Severity theSeverity, const char *theModule, int theId) {
        if (!_mySeveritySettings.empty()) {
            Severity mySeverity = _myGlobalSeverity;
            const std::string myModule(file_string(theModule)); // remove everything before the last backslash
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
                        if (theId >= myMinId && theId <= myMaxId) {
                            unsigned int myNewRange = myMaxId - myMinId;
                            if (myNewRange < myRange) {
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

    void
    Logger::log(Severity theSeverity, const char * theModule, int theId, const std::string & theText) {
        std::string myLogTag(_myTopLevelLogTag);
        if (theSeverity == SEV_TESTRESULT) {
            myLogTag += "/TestResult/";
        }
        std::ostringstream myText;
        myText << theText;
        if (theSeverity > SEV_PRINT) {
            myText << " [" << lastFileNamePart(theModule) << " at:" << theId << "]";
        }

        #ifdef iOS
        switch (theSeverity) {
            case SEV_TRACE:
                std::cout << myLogTag << " TRACE: " << myText.str() << "\n";
                break;
            case SEV_DEBUG:
                std::cout << myLogTag << " DEBUG: " << myText.str() << "\n";
                break;
            case SEV_INFO:
                std::cout << myLogTag << " INFO: " << myText.str() << "\n";
                break;
            case SEV_WARNING:
                std::cout << myLogTag << " WARNING: " << myText.str() << "\n";
                break;
            case SEV_PRINT:
                std::cout << myLogTag << " LOG: " << myText.str() << "\n";
                break;
            case SEV_ERROR:
                std::cout << myLogTag << " ERROR: " << myText.str() << "\n";
                break;
            case SEV_TESTRESULT :
                std::cout << myLogTag << " TEST: " << myText.str() << "\n";
                break;
            default:
                throw Exception("Unknown logger severity");
                break;
        }

        #elif ANDROID
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
