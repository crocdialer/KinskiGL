#ifndef __KINSKI_EXCEPTION_INCLUDED__
#define __KINSKI_EXCEPTION_INCLUDED__

#include <string>
#include <stdexcept>

namespace kinski 
{
class Exception : public std::runtime_error
{
private:
    std::string m_message;
    
public:
    explicit Exception(const std::string &msg): std::runtime_error(msg), m_message(msg) 
    {
        log();
    };

    virtual ~Exception() throw(){};
    
    inline std::string getMessage() 
    { 
        return m_message; 
    };
    
    inline void log()
    {
        //TODO: implement logging
    };
    
};
} // namespace kinski

#endif // __ATS_EXCEPTION_INCLUDED__
