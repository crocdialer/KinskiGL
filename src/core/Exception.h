#ifndef __KINSKI_EXCEPTION_INCLUDED__
#define __KINSKI_EXCEPTION_INCLUDED__

#include <string>

namespace kinski {
class Exception {
 
private:
   
    std::string m_message;
 
public:
    Exception( std::string message ): m_message(message) {};
    inline std::string getMessage() { 
        return m_message; 
    };
    
    inline void log() { 
    };
    
};
} // namespace kinski

#endif // __ATS_EXCEPTION_INCLUDED__
