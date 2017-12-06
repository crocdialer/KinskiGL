INCLUDE(Kinski_Helpers)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

OPTION (BUILD_APPLIB "Build App framework" ON)
OPTION (BUILD_CVLIB "Build CV framework" OFF)
set(PROJECT_DIRS "${CMAKE_SOURCE_DIR}/src/projects" CACHE PROJECT_DIRS "Project directories")

ADD_SUBDIRECTORY("core")
SET(LIBS ${LIBS} "core")

##### GLM
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/blocks/glm/include")

ADD_SUBDIRECTORY("gl")
INCLUDE_DIRECTORIES("gl")
SET(LIBS ${LIBS} "gl")

if(NOT (BUILD_IOS OR KINSKI_ARM))
ADD_SUBDIRECTORY("AntTweakBar")
INCLUDE_DIRECTORIES("AntTweakBar")
SET(LIBS ${LIBS} "AntTweakBar")
if(BUILD_CVLIB)
ADD_SUBDIRECTORY("cv")
endif(BUILD_CVLIB)
endif()

if(BUILD_APPLIB)
ADD_SUBDIRECTORY("app")
SET(LIBS ${LIBS} "app")
endif(BUILD_APPLIB)

# add all project directories
foreach(P ${PROJECT_DIRS})
  get_filename_component(FOLDER_NAME ${P} NAME)
  #message(${FOLDER_NAME})
  ADD_SUBDIRECTORY(${P} "${CMAKE_BINARY_DIR}/${FOLDER_NAME}")
endforeach()
