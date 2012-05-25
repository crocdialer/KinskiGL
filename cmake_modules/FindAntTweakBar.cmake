
if (ANTTWEAKBAR_LIBRARIES AND ANTTWEAKBAR_INCLUDE_DIRS)
# in cache already
    set(ANTTWEAKBAR_FOUND TRUE)
else (ANTTWEAKBAR_LIBRARIES AND ANTTWEAKBAR_INCLUDE_DIRS)
    find_path(ANTTWEAKBAR_INCLUDE_DIR
            NAMES
                AntTweakBar.h
            PATHS
                /usr/include
                /usr/local/include
                /opt/local/include
                /sw/include
                ${CMAKE_SOURCE_DIR}/libs/include 
            )
    
    set(PLATFORM_STRING "WIN32") 
    if (${WIN32})
        set(PLATFORM_STRING "WIN32") 
    endif(${WIN32})
    
    if (${APPLE})
        set(PLATFORM_STRING "OSX") 
    endif(${APPLE})

    find_library(ANTTWEAKBAR_LIBRARY
            NAMES
                AntTweakBar
            PATHS
                /usr/lib
                /usr/local/lib
                /opt/local/lib
                /sw/lib
                ${CMAKE_SOURCE_DIR}/libs/lib_${PLATFORM_STRING}
            )
    
    set(ANTTWEAKBAR_INCLUDE_DIRS
            ${ANTTWEAKBAR_INCLUDE_DIR}
       )
    set(ANTTWEAKBAR_LIBRARIES
            ${ANTTWEAKBAR_LIBRARY}
       )

    if (ANTTWEAKBAR_INCLUDE_DIRS AND ANTTWEAKBAR_LIBRARIES)
        set(ANTTWEAKBAR_FOUND TRUE)
    endif (ANTTWEAKBAR_INCLUDE_DIRS AND ANTTWEAKBAR_LIBRARIES)

    if (ANTTWEAKBAR_FOUND)
        message(STATUS "Found libAntTweakBar:")
        message(STATUS " - Includes: ${ANTTWEAKBAR_INCLUDE_DIRS}")
        message(STATUS " - Libraries: ${ANTTWEAKBAR_LIBRARIES}")
    else (ANTTWEAKBAR_FOUND)
        message(FATAL_ERROR "Could not find libAntTweakBar")
    endif (ANTTWEAKBAR_FOUND)

    # show the ANTTWEAKBAR_INCLUDE_DIRS and ANTTWEAKBAR_LIBRARIES variables only in the advanced view
    mark_as_advanced(ANTTWEAKBAR_INCLUDE_DIRS ANTTWEAKBAR_LIBRARIES)

endif (ANTTWEAKBAR_LIBRARIES AND ANTTWEAKBAR_INCLUDE_DIRS)



