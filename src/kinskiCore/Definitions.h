// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef kinskiGL_Definitions_h
#define kinskiGL_Definitions_h

#ifdef WIN32
#define KINSKI_API __declspec(dllexport)
#else
#define KINSKI_API
#endif

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stack>


#include "Exception.h"
#include "Utils.h"

//#include <boost/version.hpp>
//#if BOOST_VERSION < 105300
//#error "Kinski requires Boost version 1.53 or later"
//#endif

namespace kinski
{
    using std::string;
    using std::vector;
    using std::list;
    using std::set;
    
//    using namespace std::placeholders;
}

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define KINSKI_MSW
#else
    #if defined(linux) || defined(__linux) || defined(__linux__)
        #define KINSKI_LINUX
        #ifdef __arm__
            #define KINSKI_RASPI
        #endif

    #elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
        #define KINSKI_COCOA
        #include "TargetConditionals.h"
        #if TARGET_OS_IPHONE
            #define KINSKI_COCOA_TOUCH
        #else
            #define KINSKI_MAC
        #endif
        // This is defined to prevent the inclusion of some unfortunate macros in <AssertMacros.h>
        #define __ASSERTMACROS__
    #else
        #error "kinski compile error: Unknown platform"
    #endif
#endif

// compiler has C++11 stdlib
#if defined(KINSKI_CPP11) || defined(KINSKI_MSW) || defined (KINSKI_COCOA_TOUCH) || defined( _LIBCPP_VERSION )
    #include <memory>
    #include <functional>
#else
    #include <boost/shared_ptr.hpp>
    #include <boost/enable_shared_from_this.hpp>
    #include <boost/functional.hpp>
    namespace std
    {
        using boost::shared_ptr; // future-proof shared_ptr by putting it into std::
        using boost::weak_ptr;
        using boost::static_pointer_cast;
        using boost::dynamic_pointer_cast;
        using boost::const_pointer_cast;
        using boost::enable_shared_from_this;
        
        // backwards compatibility hack
        template<typename T> struct owner_less : public less<T>{};
    }
#endif

// forward declare boost io_service
namespace boost{ namespace asio{ class io_service; } }

#endif//kinskiGL_Definitions_h
