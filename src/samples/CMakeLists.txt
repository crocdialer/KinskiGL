function(addSample theName thePath)

    FILE(GLOB FOLDER_SOURCES "${thePath}/*.c*")
    FILE(GLOB FOLDER_HEADERS "${thePath}/*.h*")

    FILE(GLOB_RECURSE resFiles "${thePath}/res/*")

    SOURCE_GROUP("Source Files" FILES ${FOLDER_SOURCES})
    SOURCE_GROUP("Header Files" FILES ${FOLDER_HEADERS})

    INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")
    
    IF( APPLE )
    
    SET( MACOSX_BUNDLE_ICON_FILE ${thePath}/res/icon.icns)
    SET_SOURCE_FILES_PROPERTIES(
        ${resFiles}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    ADD_EXECUTABLE(${theName} MACOSX_BUNDLE ${FOLDER_SOURCES} ${FOLDER_HEADERS}
            ${resFiles})
    ELSE( APPLE )
    add_executable(${theName} ${FOLDER_SOURCES} ${FOLDER_HEADERS})
    ENDIF( APPLE )

    target_link_libraries (${theName} ${LIBS})

    # add the install targets
    install (TARGETS ${theName} DESTINATION bin)

endfunction()

INCLUDE_DIRECTORIES("../")

# add our sample(s)
FILE(GLOB FOLDER_CONTENT *)

FOREACH(folderItem ${FOLDER_CONTENT})
    IF(IS_DIRECTORY ${folderItem})

        STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" itemName ${folderItem})
                
        MESSAGE("adding sample: '${itemName}'")

        # include optional CMakeLists.txt in sample folder
        FILE(GLOB CMAKELISTS_EXIST "${folderItem}/CMakeLists.txt")
        if(CMAKELISTS_EXIST)
            message("using CMakeLists.txt for sample '${itemName}'")
            ADD_SUBDIRECTORY(${folderItem})
        else(CMAKELISTS_EXIST)
            addSample(${itemName} ${folderItem})
        endif()

    ENDIF()
ENDFOREACH(folderItem)

