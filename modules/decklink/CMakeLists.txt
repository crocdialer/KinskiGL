# module files
FILE(GLOB MODULE_HEADERS DeckLink.h)
FILE(GLOB MODULE_SOURCES DeckLink.cpp ${CMAKE_SOURCE_DIR}/blocks/decklink/Mac/include/DeckLinkAPIDispatch.cpp)

if(APPLE)
  set(MODULE_INCLUDES "${CMAKE_SOURCE_DIR}/blocks/decklink/Mac/include" PARENT_SCOPE)
elseif(UNIX)
  set(MODULE_INCLUDES "${CMAKE_SOURCE_DIR}/blocks/decklink/Linux/include" PARENT_SCOPE)
elseif(WIN32)
  set(MODULE_INCLUDES "${CMAKE_SOURCE_DIR}/blocks/decklink/Win/include" PARENT_SCOPE)
endif()

# forward variables to parent scope
set(MODULE_HEADERS ${MODULE_HEADERS} PARENT_SCOPE)
set(MODULE_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)

