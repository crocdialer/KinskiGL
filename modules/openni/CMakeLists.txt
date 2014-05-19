##### OPENNI
if(WIN32)
    SET(OPENNI_INCLUDE_DIRS "")
    SET(OPENNI_LIB_DIR "")
    SET(OPENNI_LIBRARIES "OpenNI64")
else(WIN32)
    SET(OPENNI_INCLUDE_DIRS "/usr/local/include/ni")
    SET(OPENNI_LIB_DIR "/usr/local/lib")
  #SET(OPENNI_LIBRARIES "OpenNI")

    FIND_LIBRARY( OPENNI_LIBRARIES DOC "Absolute path to OpenNI"
                  NAMES "OpenNI"
                  HINTS
                  PATHS
                  /usr/local/lib
                  /usr/lib
    )
    
    MESSAGE(${OPENNI_LIBRARIES})
endif(WIN32)

set(MODULE_INCLUDES ${OPENNI_INCLUDE_DIRS} PARENT_SCOPE)
set(MODULE_LIBRARIES ${OPENNI_LIBRARIES} PARENT_SCOPE)
