//
//  Definitions.h
//  kinskiGL
//
//  Created by Fabian on 9/19/12.
//
//

#ifndef kinskiGL_Definitions_h
#define kinskiGL_Definitions_h

#ifdef WIN32
#define KINSKI_API __declspec(dllexport)
#else
#define KINSKI_API
#endif

#include <vector>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/version.hpp>
#include <boost/static_assert.hpp>

#if BOOST_VERSION < 104900
#error "Kinski requires Boost version 1.49 or later"
#endif

namespace kinski
{
    
    using boost::int8_t;
    using boost::uint8_t;
    using boost::int16_t;
    using boost::uint16_t;
    using boost::int32_t;
    using boost::uint32_t;
    using boost::int64_t;
    using boost::uint64_t;
    
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define KINSKI_MSW
#elif defined(linux) || defined(__linux) || defined(__linux__)
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
    
}

// compiler has C++11 stdlib
#if __cplusplus > 199711L
#include <memory>
#elif defined( KINSKI_COCOA )
#include <tr1/memory>
namespace std
{
    using std::tr1::shared_ptr;
    using std::tr1::weak_ptr;
    using std::tr1::static_pointer_cast;
    using std::tr1::dynamic_pointer_cast;
    using std::tr1::const_pointer_cast;
    using std::tr1::enable_shared_from_this;
}
#else
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
namespace std
{
    using boost::shared_ptr; // future-proof shared_ptr by putting it into std::
    using boost::weak_ptr;
    using boost::static_pointer_cast;
    using boost::dynamic_pointer_cast;
    using boost::const_pointer_cast;
    using boost::enable_shared_from_this;
}
#endif


#endif
