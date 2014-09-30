function(KINSKI_ADD_SAMPLE theName thePath)
    
    foreach(module ${ARGV2})
      KINSKI_ADD_MODULE(${module} MODULE_SOURCES)
      SET(MODULE_FILES ${MODULE_FILES} ${MODULE_SOURCES})
      #message("adding module ${module} with files: ${MODULE_SOURCES}")
    endforeach(module)

    FILE(GLOB FOLDER_SOURCES "${thePath}/*.c*" "${thePath}/*.mm")
    FILE(GLOB FOLDER_HEADERS "${thePath}/*.h" "${thePath}/*.hpp")

    FILE(GLOB_RECURSE resFiles "${thePath}/res/*")

    SOURCE_GROUP("Source Files" FILES ${FOLDER_SOURCES})
    SOURCE_GROUP("Header Files" FILES ${FOLDER_HEADERS})

    INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")
    
    IF( APPLE )
    SET_SOURCE_FILES_PROPERTIES(
        icon.icns
        ${resFiles}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    SET( MACOSX_BUNDLE_ICON_FILE icon.icns)
    ADD_EXECUTABLE(${theName} MACOSX_BUNDLE ${FOLDER_SOURCES} ${FOLDER_HEADERS}
            ${MODULE_FILES} ${resFiles})
    ELSE( APPLE )
    if(KINSKI_RASPI)
      include_directories("/opt/vc/include/" "/opt/vc/include/interface/vcos/pthreads"
        "/opt/vc/include/interface/vmcs_host/linux" ) 
      link_directories("/opt/vc/lib")
      
    endif(KINSKI_RASPI)
    add_executable(${theName} ${FOLDER_SOURCES} ${FOLDER_HEADERS} ${MODULE_FILES})
    ENDIF( APPLE )

    target_link_libraries (${theName} ${LIBS})

    # add the install targets
    install (TARGETS ${theName} DESTINATION bin)

endfunction()

function(KINSKI_ADD_MODULE MODULE_NAME FILE_LIST)

  SET(MODULE_PATH "${CMAKE_SOURCE_DIR}/modules/${MODULE_NAME}")
  IF(IS_DIRECTORY ${MODULE_PATH})

    # CmakeLists.txt in module folder is supposed to define
    # MODULE_INCLUDES and MODULE_LIBRARIES 
    ADD_SUBDIRECTORY(${MODULE_PATH} "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}")

    INCLUDE_DIRECTORIES(${MODULE_PATH} ${MODULE_INCLUDES} PARENT_SCOPE)
    SET(LIBS ${LIBS} ${MODULE_LIBRARIES} PARENT_SCOPE)

    FILE(GLOB MODULE_SOURCES "${MODULE_PATH}/*.c*" "${MODULE_PATH}/*.m" "${MODULE_PATH}/*.mm")
    FILE(GLOB MODULE_HEADERS "${MODULE_PATH}/*.h" "${MODULE_PATH}/*.hpp")
    SET(${FILE_LIST} ${MODULE_SOURCES} ${MODULE_HEADERS} PARENT_SCOPE)

  else()
    MESSAGE("Could not find a module named ${MODULE_NAME}")
  endif(IS_DIRECTORY ${MODULE_PATH})

endfunction(KINSKI_ADD_MODULE)

function(addTestMacro)
  if(BUILD_TESTS)
      FILE(GLOB TEST_SOURCES "tests/*.c*")
      FILE(GLOB TEST_HEADERS "tests/*.h")

  SOURCE_GROUP("Unit-Tests" FILES ${TEST_SOURCES} ${TEST_HEADERS})
      FOREACH(testFile ${TEST_SOURCES})    
          STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/tests/" "" testName ${testFile})
          STRING(REPLACE ".cpp" "" testName ${testName})
          add_executable(${testName} ${testFile} ${TEST_HEADERS})
          TARGET_LINK_LIBRARIES(${testName} ${LIBS})
          add_test(${testName} "${EXECUTABLE_OUTPUT_PATH}/${testName}")
          MESSAGE("Added Test: ${testName}")
      ENDFOREACH(testFile)
  endif(BUILD_TESTS)
endfunction()

