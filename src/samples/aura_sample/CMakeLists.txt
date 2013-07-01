##### ASSIMP
find_package(ASSIMP REQUIRED)
INCLUDE_DIRECTORIES(${ASSIMP_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ASSIMP_LIBRARIES})

##### OPENCL
find_package(OpenCL REQUIRED)
INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENCL_LIBRARIES})
    
find_package(BULLET REQUIRED)
INCLUDE_DIRECTORIES(${BULLET_INCLUDE_DIRS})
set(LIBS ${LIBS} ${BULLET_LIBRARIES})

find_package(FMOD REQUIRED)
INCLUDE_DIRECTORIES(${FMOD_INCLUDE_DIRS})
set(LIBS ${LIBS} ${FMOD_LIBRARIES})

addSample(${itemName} ${folderItem})

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
set(EXECUTABLE_PATH ${CMAKE_CURRENT_BINARY_DIR}/Release/${itemName}.app/Contents/MacOS)
else()
set(EXECUTABLE_PATH ${CMAKE_CURRENT_BINARY_DIR}/Debug/${itemName}.app/Contents/MacOS)
endif()

set(LIB_PATH ${EXECUTABLE_PATH}/../SharedSupport)
set(LIB_FILE ${LIB_PATH}/libfmodex.dylib)
set(NEW_LIB_PATH "@executable_path/../SharedSupport")

add_custom_command( 
    TARGET ${itemName} 
    POST_BUILD 
    COMMAND mkdir -p ${EXECUTABLE_PATH}/../SharedSupport)
add_custom_command( 
    TARGET ${itemName} 
    POST_BUILD 
    COMMAND cp ${FMOD_LIBRARIES} ${EXECUTABLE_PATH}/../SharedSupport)
#add_custom_command( 
#    TARGET ${itemName} 
#    POST_BUILD 
#    COMMAND 
#        # space separated list of libraries
#        TARGETID=`otool -DX ${LIB_FILE}`
#        NEWTARGETID=${NEW_LIB_PATH}/libfmodex.dylib
#        install_name_tool -id ${NEWTARGETID} ${LIB_FILE}
##install_name_tool -change ${TARGETID} ${NEWTARGETID} ${EXECUTABLE_PATH}/${itemName}
#        )

