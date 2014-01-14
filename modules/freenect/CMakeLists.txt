FIND_PATH(FREENECT_INCLUDE_DIR NAMES libfreenect.h
  PATHS
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
  /usr/include/libfreenect
  /usr/local/include/libfreenect
  /usr/include
  /usr/local/include
  /opt/local/include
  NO_DEFAULT_PATH
)

#SET( FREENECT_INCLUDE_DIR /usr/local/include/libfreenect)
FIND_LIBRARY(FREENECT_LIBRARIES
  NAMES
  freenect
  PATHS
  ENV LD_LIBRARY_PATH
  ENV LIBRARY_PATH
  /usr/lib
  /usr/lib/x86_64-linux-gnu
  /usr/local/lib
  /opt/local/lib
  NO_DEFAULT_PATH
)

set(MODULE_INCLUDES ${FREENECT_INCLUDE_DIR} PARENT_SCOPE)
set(MODULE_LIBRARIES ${FREENECT_LIBRARIES} PARENT_SCOPE)

