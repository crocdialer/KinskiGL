cmake_minimum_required (VERSION 2.8)

project (kinskiGL)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules
                      ${CMAKE_MODULE_PATH})

##### PKGCONFIG
#find_package(PkgConfig REQUIRED)
#####

OPTION (BUILD_IOS "Build for ios" OFF)

if(BUILD_IOS)
SET(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake_modules/ios.toolchain.iOS.cmake)
endif(BUILD_IOS)

# Allow the developer to select if Dynamic or Static libraries are built
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
SET (LIB_TYPE SHARED)
IF (NOT BUILD_SHARED_LIBS)
  SET (LIB_TYPE STATIC)
ENDIF (NOT BUILD_SHARED_LIBS)

OPTION (BUILD_APPLIB "Build App framework" ON)
OPTION (BUILD_SAMPLES "Build Samples" ON)
OPTION (BUILD_TESTS "Build Tests" ON)

if(BUILD_IOS)
INCLUDE_DIRECTORIES("/usr/local/include")
else()
##### BOOST
set(Boost_USE_STATIC_LIBS ON)
set(BOOST_COMPONENTS thread system timer chrono)
if(BUILD_TESTS)
    ENABLE_TESTING()
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)
endif(BUILD_TESTS)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
#####
endif()

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

ADD_SUBDIRECTORY("src")
