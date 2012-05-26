# add our sample(s)

FILE(GLOB folder_source *.c*)
FILE(GLOB folder_header *.h*)
SOURCE_GROUP("Source Files" FILES ${FOLDER_SOURCES})
SOURCE_GROUP("Header Files" FILES ${FOLDER_HEADERS})

INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")

# simple example
add_executable(simple simple.cpp )
target_link_libraries (simple ${LIBS})

# add the install targets
install (TARGETS simple DESTINATION bin)
