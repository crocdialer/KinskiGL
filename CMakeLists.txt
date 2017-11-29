cmake_minimum_required (VERSION 3.3)

project (kinskiGL)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules
                      ${CMAKE_MODULE_PATH})

# Allow the developer to select if Dynamic or Static libraries are built
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
OPTION (USE_CXX11 "Use C++11 language dialect and library" ON)
OPTION (BUILD_TESTS "Build Tests" ON)

if(USE_CXX11)
    add_definitions(-DKINSKI_CPP11)

    #set(CMAKE_CXX_STANDARD 14)
    #set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
    SET(CMAKE_CXX_FLAGS "-std=c++1z -stdlib=libc++")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++1z")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    ELSE( APPLE )
    SET (CMAKE_CXX_FLAGS "-std=c++14 -pthread -Wall")
    endif(APPLE)

endif(USE_CXX11)

SET (LIB_TYPE SHARED)
IF (NOT BUILD_SHARED_LIBS)
  SET (LIB_TYPE STATIC)
ENDIF (NOT BUILD_SHARED_LIBS)

##### BOOST
set(Boost_USE_STATIC_LIBS ON)
set(BOOST_COMPONENTS system filesystem)
if(BUILD_TESTS)
    ENABLE_TESTING()
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)
endif(BUILD_TESTS)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
#####

if(UNIX AND NOT APPLE)
    set(KINSKI_LINUX TRUE)

    # check if we are on raspberry pi
    find_file(BCMHOST bcm_host.h PATHS "/opt/vc/include/")

    # check if we have a mali gpu
    find_file(MALI libmali.so PATHS "/usr/lib/arm-linux-gnueabihf")

    # it's an ARM for sure
    if(BCMHOST OR MALI)
      set(KINSKI_ARM TRUE)
      add_definitions(-DKINSKI_ARM)
    endif()

    # broadcom SoC -> RaspberryPi
    if(BCMHOST)
      set(KINSKI_RASPI TRUE)
      add_definitions(-DKINSKI_RASPI)
    endif()

    if(MALI)
        set(KINSKI_MALI TRUE)
      	add_definitions(-DKINSKI_MALI)
    endif()
endif()

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(RELEASE_BUILDS "Release MinSizeRel")
if(${CMAKE_BUILD_TYPE} IN_LIST RELEASE_BUILDS)
    list(APPEND CXX_FLAGS "-O3")
endif()

ADD_SUBDIRECTORY("src")
