cmake_minimum_required (VERSION 2.8)

project (kinskiGL)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules
                      ${CMAKE_MODULE_PATH})

##### PKGCONFIG
#find_package(PkgConfig REQUIRED)
#####

OPTION (BUILD_SAMPLES "Build Samples" ON)

# Allow the developer to select if Dynamic or Static libraries are built
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
SET (LIB_TYPE SHARED)
IF (NOT BUILD_SHARED_LIBS)
  SET (LIB_TYPE STATIC)
ENDIF (NOT BUILD_SHARED_LIBS)

##### OPENGL
find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

##### GLFW
#pkg_search_module(GLFW REQUIRED libglfw)
find_package(GLFW REQUIRED)
INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${GLFW_LIBRARIES})
#####

##### BOOST
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS thread system timer chrono)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
#####

##### OPENCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBS})
#####

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

ADD_SUBDIRECTORY("src")
