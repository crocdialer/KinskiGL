// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

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
        m_message(msg){};
        
        virtual ~Exception() throw(){};
        
        inline std::string getMessage(){ return m_message; };
        
    private:
        std::string m_message;
    };
    
} // namespace kinski

#endif // __KINSKI_EXCEPTION_INCLUDED__
