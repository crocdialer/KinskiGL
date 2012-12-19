#ifndef __KINSKI_EXCEPTION_INCLUDED__
#define __KINSKI_EXCEPTION_INCLUDED__

#include <iostream>
#include <string>
#include <stdexcept>

namespace kinski
{
    class Exception : public std::runtime_error
    {
    public:
        explicit Exception(const std::string &msg): std::runtime_error(msg),
        m_message(msg)
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
            //std::cerr<<m_message<<std::endl;
        };
        
    private:
        std::string m_message;
    };
    
} // namespace kinski

#endif // __KINSKI_EXCEPTION_INCLUDED__
