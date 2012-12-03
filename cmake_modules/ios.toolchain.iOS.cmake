# __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
#
# Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
#
# It is distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
# __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

SET (CMAKE_CROSSCOMPILING 1)

# System and architecture
SET(TARGET_PLATFORM "iPhoneOS")
SET(IOS_ARCH "armv7")

# SDK- and Deploy-versions
SET(IOS_SDK_VERSION "6.0")
SET(IOS_DEPLOY_TGT "6.0")

SET(IOS True)

# run xcode-select for SDK paths
find_program(CMAKE_XCODE_SELECT xcode-select)
if(CMAKE_XCODE_SELECT)
    execute_process(COMMAND ${CMAKE_XCODE_SELECT} "-print-path" 
        OUTPUT_VARIABLE OSX_DEVELOPER_ROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# some internal values
set(DEVROOT "${OSX_DEVELOPER_ROOT}/Platforms/${TARGET_PLATFORM}.platform/Developer")
set(SDKROOT "${DEVROOT}/SDKs/${TARGET_PLATFORM}${IOS_SDK_VERSION}.sdk")

if(XCODE)
    set(CMAKE_OSX_SYSROOT "iphoneos${IOS_SDK_VERSION}" CACHE STRING "SDK version" FORCE)
else()
    set(CMAKE_OSX_SYSROOT ${SDKROOT} CACHE STRING "SDK version" FORCE)
    
    if(NOT IOS_VER_OPTION)
    set(IOS_VER_OPTION "-D__IPHONE_OS_VERSION_MIN_REQUIRED=__IPHONE_5_0")
    #REMOVE_DEFINITIONS(${IOS_VER_OPTION})
    ADD_DEFINITIONS(${IOS_VER_OPTION})
    endif()
endif()

# Skip the platform compiler checks for cross compiling
SET(CMAKE_CXX_COMPILER_WORKS TRUE)
SET(CMAKE_C_COMPILER_WORKS TRUE)

SET(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${IOS_DEPLOY_TGT})

#set target device: "1" -> iPhone, "2" -> iPad, "1,2 " -> both (remember the <whitespace> after the '2' !!!)
SET(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2 ")

SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_OSX_ARCHITECTURES "${IOS_ARCH}" CACHE STRING "SDK Architecture" FORCE)

SET (CMAKE_C_COMPILER             "/usr/bin/clang")
SET (CMAKE_C_FLAGS_DEBUG          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g -std=c++11")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -std=c++11")

SET (CMAKE_AS      "${DEVROOT}/usr/bin/as")
SET (CMAKE_LINKER  "${DEVROOT}/usr/bin/ld")
SET (CMAKE_NM      "${DEVROOT}/usr/bin/nm")
SET (CMAKE_RANLIB  "${DEVROOT}/usr/bin/ranlib")

# Flags
SET(OUR_FLAGS="-arch ${IOS_ARCH}  -Wall -pipe -no-cpp-precomp  --sysroot=${SDKROOT} 
        -miphoneos-version-min=${IOS_DEPLOY_TGT}
        -I${SDKROOT}/usr/include/")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OUR_FLAGS}" CACHE STRING "c flags")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OUR_FLAGS}" CACHE STRING "c++ flags")

# Header
INCLUDE_DIRECTORIES(SYSTEM "${SDKROOT}/usr/include")
INCLUDE_DIRECTORIES(SYSTEM "${SDKROOT}/System/Library/Frameworks")

# System Libraries
LINK_DIRECTORIES("${SDKROOT}/usr/lib")
LINK_DIRECTORIES("${SDKROOT}/System/Library/Frameworks")
#LINK_DIRECTORIES("${SDKROOT}/usr/lib/gcc/arm-apple-darwin10/4.2.1")

SET(CMAKE_FIND_ROOT_PATH ${DEVROOT} ${SDKROOT})
SET (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
SET (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

