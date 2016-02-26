// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

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

#include <memory>
#include <functional>
#include <algorithm>

// forward declare boost io_service
namespace boost{ namespace asio{ class io_service; } }

#include "Logger.hpp"
#include "Exception.hpp"
#include "Utils.hpp"