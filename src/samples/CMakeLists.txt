# add our sample(s)

FILE(GLOB FOLDER_SOURCES *.c*)
FILE(GLOB FOLDER_HEADERS *.h*)
SOURCE_GROUP("Source Files" FILES ${FOLDER_SOURCES})
SOURCE_GROUP("Header Files" FILES ${FOLDER_HEADERS})

INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")

# simple example
add_executable(simple simple.cpp ${FOLDER_HEADERS})
target_link_libraries (simple ${LIBS})

# add the install targets
install (TARGETS simple DESTINATION bin)
