if(APPLE)
  find_library(AVFoundation NAMES AVFoundation)
  find_library(CoreMedia NAMES CoreMedia)
  find_library(CoreFoundation NAMES CoreFoundation)
  find_library(IOSurface NAMES IOSurface)
  set(MODULE_LIBRARIES ${AVFoundation} ${CoreFoundation} ${CoreMedia} ${IOSurface} PARENT_SCOPE)

  # OSX specific implementation
  FILE(GLOB MODULE_SOURCES osx/*.mm)
elseif(KINSKI_RASPI)

  # Raspian specific implementation
  FILE(GLOB MODULE_SOURCES rpi/*.c* rpi/utils/*.c* rpi/linux/*.c*)

  #find_library(AVFormat NAMES avformat)
  #find_library(AVCodec NAMES avcodec)
  #find_library(AVUtil NAMES avutil)
  #find_library(SWResample NAMES swresample)

  set(MODULE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/include")
  set(AVFormat "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libavformat.a")
  set(AVCodec "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libavcodec.a")
  set(AVUtil "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libavutil.a")
  set(SWResample "${CMAKE_SOURCE_DIR}/blocks/ffmpeg/lib_rpi/libswresample.a")

  find_package(ZLIB REQUIRED)
  find_library(M_LIB NAMES m)

  set(MODULE_LIBRARIES openmaxil pthread ${ZLIB_LIBRARIES} ${M_LIB} ${AVFormat} ${AVCodec} ${AVUtil} ${SWResample} vcos vchiq_arm vcilcs vcsm PARENT_SCOPE)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)

# common header files
FILE(GLOB MODULE_HEADERS MovieController.h CameraController.h)

# forward variables to parent scope
set(MODULE_INCLUDE_DIRS ${MODULE_INCLUDE_DIRS} PARENT_SCOPE)
set(MODULE_HEADERS ${MODULE_HEADERS} PARENT_SCOPE)
set(MODULE_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)
