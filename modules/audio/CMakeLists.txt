##### FMOD
find_package(FMOD REQUIRED)
set(MODULE_INCLUDES ${FMOD_INCLUDE_DIRS} PARENT_SCOPE)
set(MODULE_LIBRARIES ${FMOD_LIBRARIES} PARENT_SCOPE)

#if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
#set(EXECUTABLE_PATH ${CMAKE_CURRENT_BINARY_DIR}/Release/${itemName}.app/Contents/MacOS)
#else()
#set(EXECUTABLE_PATH ${CMAKE_CURRENT_BINARY_DIR}/Debug/${itemName}.app/Contents/MacOS)
#endif()
#
#set(LIB_PATH ${EXECUTABLE_PATH}/../SharedSupport)
#set(LIB_FILE ${LIB_PATH}/libfmodex.dylib)
#set(NEW_LIB_PATH "@executable_path/../SharedSupport")
#
#add_custom_command( 
#    TARGET ${itemName} 
#    POST_BUILD 
#    COMMAND mkdir -p ${EXECUTABLE_PATH}/../SharedSupport)
#add_custom_command( 
#    TARGET ${itemName} 
#    POST_BUILD 
#    COMMAND cp ${FMOD_LIBRARIES} ${EXECUTABLE_PATH}/../SharedSupport)

