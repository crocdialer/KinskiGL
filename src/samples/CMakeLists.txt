function(addSample theName thePath)

    FILE(GLOB FOLDER_SOURCES "${thePath}/*.c*")
    FILE(GLOB FOLDER_HEADERS "${thePath}/*.h*")
    SOURCE_GROUP("Source Files" FILES ${FOLDER_SOURCES})
    SOURCE_GROUP("Header Files" FILES ${FOLDER_HEADERS})

    INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")

    # 
    add_executable(${theName} ${FOLDER_SOURCES} ${FOLDER_HEADERS})
    target_link_libraries (${theName} ${LIBS})

    # add the install targets
    install (TARGETS ${theName} DESTINATION bin)

endfunction()

# add our sample(s)
FILE(GLOB FOLDER_CONTENT *)

FOREACH(folderItem ${FOLDER_CONTENT})
    IF(IS_DIRECTORY ${folderItem})
        STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" itemName ${folderItem})
        #MESSAGE("adding sample: ${itemName}")
        addSample(${itemName} ${folderItem})
    ENDIF()
ENDFOREACH(folderItem)

