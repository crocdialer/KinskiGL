// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#ifdef WIN32
#define KINSKI_API __declspec(dllexport)
#else
#define KINSKI_API
#endif

#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>

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

//! forward declare a class and define shared-, const- and weak smart-pointers for it.
#define DEFINE_CLASS_PTR(CLASS_NAME)\
class CLASS_NAME;\
typedef std::shared_ptr<CLASS_NAME> CLASS_NAME##Ptr;\
typedef std::shared_ptr<const CLASS_NAME> CLASS_NAME##ConstPtr;\
typedef std::weak_ptr<CLASS_NAME> CLASS_NAME##WeakPtr;


// forward declare boost io_service
namespace boost{ namespace asio{ class io_service; } }

#include "Logger.hpp"
#include "Exception.hpp"
#include "Utils.hpp"

namespace kinski
{
    using std::string;
    using std::vector;
    using std::list;
    using std::set;
    
    // forward declase some classes
    DEFINE_CLASS_PTR(Image);
}