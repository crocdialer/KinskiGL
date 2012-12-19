#ifndef _kinski_Logger_h_included_
#define _kinski_Logger_h_included_

#include <string>
#include <sstream>
#include <map>

namespace kinski {
    enum Severity { SEV_PRINT, SEV_FATAL, SEV_ERROR, SEV_WARNING, SEV_INFO,
                    SEV_DEBUG, SEV_TRACE, SEV_DISABLED};

class Logger
{
    struct ModuleSeverity
    {
        ModuleSeverity() {}
        ModuleSeverity(Severity theSeverity,int theMinId, int myMaxId)
            : mySeverity(theSeverity), myMinId(theMinId), myMaxId(myMaxId)
        {}

        Severity mySeverity;
        int myMinId;
        int myMaxId;
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

    private:
    
        Logger();
        static Logger *s_instance;
    
        std::string _myTopLevelLogTag;
        Severity _myGlobalSeverity;
        std::multimap<std::string,ModuleSeverity> _mySeveritySettings;
};

/**
This class is used to collect the output and deliver it to the Logger on destruction
*/
class MessagePort
{
 public:
    MessagePort(Severity theSeverity, const char * theModule, int theId)
        : mySeverity(theSeverity), myModule(theModule), myId(theId)
    {}
    ~MessagePort()
    {
        Logger::get()->log(mySeverity, myModule, myId, stream.str());
    }

    inline std::ostringstream& getStream()
    {
        return stream;
    }
    
    std::ostringstream stream;
    
    const Severity mySeverity;
    const char *myModule;
    const int myId;
};

#define KINSKI_LOG_CHECK(SEVERITY,MODULE,MSGID) kinski::Logger::get()->ifLog(SEVERITY,MODULE,MSGID) \
    && (kinski::MessagePort(SEVERITY,MODULE,MSGID).getStream())

#define LOG_INFO KINSKI_LOG_CHECK(kinski::SEV_INFO, __FILE__ ,__LINE__)
#define LOG_TRACE KINSKI_LOG_CHECK(kinski::SEV_TRACE, __FILE__ ,__LINE__)
#define LOG_DEBUG KINSKI_LOG_CHECK(kinski::SEV_DEBUG, __FILE__ ,__LINE__)
#define LOG_PRINT KINSKI_LOG_CHECK(kinski::SEV_PRINT, __FILE__ ,__LINE__)
#define LOG_ERROR KINSKI_LOG_CHECK(kinski::SEV_ERROR, __FILE__ ,__LINE__)
#define LOG_WARNING KINSKI_LOG_CHECK(kinski::SEV_WARNING, __FILE__ ,__LINE__)

}//namespace
#endif
