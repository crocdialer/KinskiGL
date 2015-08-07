INCLUDE_DIRECTORIES("../")

# add our sample(s)
FILE(GLOB FOLDER_CONTENT *)

FOREACH(folderItem ${FOLDER_CONTENT})
    IF(IS_DIRECTORY ${folderItem})

        STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" itemName ${folderItem})
                
        MESSAGE("adding project: '${itemName}'")

        # include optional CMakeLists.txt in sample folder
        FILE(GLOB CMAKELISTS_EXIST "${folderItem}/CMakeLists.txt")
        if(CMAKELISTS_EXIST)
            message("using CMakeLists.txt for sample '${itemName}'")
            ADD_SUBDIRECTORY(${folderItem})
        else(CMAKELISTS_EXIST)
            KINSKI_ADD_SAMPLE(${itemName} ${folderItem})
        endif()

    ENDIF()
ENDFOREACH(folderItem)

