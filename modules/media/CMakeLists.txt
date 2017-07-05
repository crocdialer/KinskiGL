OPTION (USE_GSTREAMER "Use GStreamer" OFF)

if(KINSKI_LINUX AND NOT KINSKI_ARM)
  set(USE_GSTREAMER ON)
endif()

if(USE_GSTREAMER)
    add_definitions(-DKINSKI_GSTREAMER)
    find_package(GStreamer REQUIRED COMPONENTS GL APP VIDEO)
    pkg_search_module(GLIB REQUIRED glib-2.0)
    find_library(GLIB NAMES glib-2.0)
    find_library(GOBJECT NAMES gobject-2.0)
    set(MODULE_LIBRARIES ${GSTREAMER_LIBRARIES} ${GSTREAMER_BASE_LIBRARIES} ${GSTREAMER_APP_LIBRARIES} ${GSTREAMER_VIDEO_LIBRARIES} ${GSTREAMER_GL_LIBRARIES} ${GLIB} ${GOBJECT})
    set(MODULE_INCLUDE_DIRS ${GLIB_INCLUDE_DIRS} ${GSTREAMER_INCLUDE_DIRS} ${GSTREAMER_GL_INCLUDE_DIRS} ${GSTREAMER_APP_INCLUDE_DIRS} "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/include" "/usr/local/lib/gstreamer-1.0/include")
#    message(WARNING "${MODULE_LIBRARIES}")
#    message(WARNING "${MODULE_INCLUDE_DIRS}")

    # GStreamer implementation
    FILE(GLOB MODULE_SOURCES gst/*.c*)
elseif(KINSKI_ARM)
  # Raspian specific implementation
  FILE(GLOB MODULE_SOURCES rpi/*.c* rpi/utils/*.c* rpi/linux/*.c*)

  set(MODULE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/include")
  set(AVFormat "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libavformat.a")
  set(AVCodec "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libavcodec.a")
  set(AVUtil "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libavutil.a")
  set(SWResample "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libswresample.a")

  find_package(ZLIB REQUIRED)
  find_library(M_LIB NAMES m)

  set(MODULE_LIBRARIES openmaxil pthread ${ZLIB_LIBRARIES} ${M_LIB} ${AVFormat} ${AVCodec} ${AVUtil} ${SWResample} vcos vchiq_arm vcilcs)
elseif(APPLE)
    find_library(AVFoundation NAMES AVFoundation)
    find_library(CoreMedia NAMES CoreMedia)
    find_library(CoreFoundation NAMES CoreFoundation)
    find_library(IOSurface NAMES IOSurface)
    set(MODULE_LIBRARIES ${AVFoundation} ${CoreFoundation} ${CoreMedia} ${IOSurface})

    # OSX specific implementation
    FILE(GLOB MODULE_SOURCES osx/*.mm)
endif(USE_GSTREAMER)

# common header files
FILE(GLOB MODULE_HEADERS MovieController.hpp CameraController.hpp)

# forward variables to parent scope
set(MODULE_LIBRARIES ${MODULE_LIBRARIES} PARENT_SCOPE)
set(MODULE_INCLUDE_DIRS ${MODULE_INCLUDE_DIRS} PARENT_SCOPE)
set(MODULE_HEADERS ${MODULE_HEADERS} PARENT_SCOPE)
set(MODULE_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)
