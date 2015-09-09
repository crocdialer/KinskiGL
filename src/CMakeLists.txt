INCLUDE(Kinski_Helpers)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

OPTION (BUILD_APPLIB "Build App framework" ON)
set(PROJECT_DIRS "${CMAKE_SOURCE_DIR}/src/projects" CACHE PROJECT_DIRS "Project directories")

ADD_SUBDIRECTORY("core")
SET(LIBS ${LIBS} "core")

##### GLM
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/blocks/glm/include")

ADD_SUBDIRECTORY("gl")
INCLUDE_DIRECTORIES("gl")
SET(LIBS ${LIBS} "gl")

if(NOT (BUILD_IOS OR KINSKI_RASPI))
ADD_SUBDIRECTORY("AntTweakBar")
INCLUDE_DIRECTORIES("AntTweakBar")
SET(LIBS ${LIBS} "AntTweakBar")
ADD_SUBDIRECTORY("cv")
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
