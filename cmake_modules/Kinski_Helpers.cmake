function(KINSKI_ADD_SAMPLE theName thePath)

    foreach(module ${ARGV2})
      KINSKI_ADD_MODULE(${module} MODULE_SOURCES)
      SET(MODULE_FILES ${MODULE_FILES} ${MODULE_SOURCES})
      #message("adding module ${module} with files: ${MODULE_SOURCES}")
    endforeach(module)

    # sources
    FILE(GLOB FOLDER_SOURCES "${thePath}/*.c*" "${thePath}/*.mm")

    # headers
    FILE(GLOB FOLDER_HEADERS "${thePath}/*.h" "${thePath}/*.hpp")

    # all resources
    FILE(GLOB_RECURSE resFiles "${thePath}/res/*")

    # icon file
    FILE(GLOB_RECURSE ICON_FILE "${thePath}/res/icon.icns")
    #message(${ICON_FILE})

    SOURCE_GROUP("Source Files" FILES ${FOLDER_SOURCES})
    SOURCE_GROUP("Header Files" FILES ${FOLDER_HEADERS})

    INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")

    IF( APPLE )
      SET_SOURCE_FILES_PROPERTIES(${resFiles} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
      set(MACOSX_BUNDLE_ICON_FILE "icon.icns")
      ADD_EXECUTABLE(${theName} MACOSX_BUNDLE ${ICON_FILE} ${FOLDER_SOURCES} ${FOLDER_HEADERS}
              ${MODULE_FILES} ${resFiles})
    ELSE( APPLE )
    if(KINSKI_RASPI)
      include_directories("/opt/vc/include/" "/opt/vc/include/interface/vcos/pthreads"
        "/opt/vc/include/interface/vmcs_host/linux" )
      link_directories("/opt/vc/lib")
      add_definitions(-DUSE_VCHIQ_ARM)
      add_definitions(-DOMX_SKIP64BIT)

    endif(KINSKI_RASPI)
    add_executable(${theName} ${FOLDER_SOURCES} ${FOLDER_HEADERS} ${MODULE_FILES})
    ENDIF( APPLE )

    target_link_libraries (${theName} ${LIBS})

    # add the install targets
    install (TARGETS ${theName} DESTINATION bin)

endfunction()

function(KINSKI_ADD_MODULE MODULE_NAME FILE_LIST)

  INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/modules/" PARENT_SCOPE)
  SET(MODULE_PATH "${CMAKE_SOURCE_DIR}/modules/${MODULE_NAME}")
  IF(IS_DIRECTORY ${MODULE_PATH})

    set(MODULE_SOURCES false)
    set(MODULE_HEADERS false)

    # CmakeLists.txt in module folder is supposed to define
    # MODULE_INCLUDES and MODULE_LIBRARIES
    # optionally MODULE_SOURCES and MODULE_HEADERS can be defined
    ADD_SUBDIRECTORY(${MODULE_PATH} "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}")

    INCLUDE_DIRECTORIES(${MODULE_PATH} ${MODULE_INCLUDES} PARENT_SCOPE)
    SET(LIBS ${LIBS} ${MODULE_LIBRARIES} PARENT_SCOPE)

    if(NOT MODULE_SOURCES)
      FILE(GLOB MODULE_SOURCES "${MODULE_PATH}/*.c*" "${MODULE_PATH}/*.m" "${MODULE_PATH}/*.mm")
    endif()
    if(NOT MODULE_HEADERS)
      FILE(GLOB MODULE_HEADERS "${MODULE_PATH}/*.h" "${MODULE_PATH}/*.hpp")
    endif()

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

function(STRINGIFY_SHADERS FOLDER_NAME)

  SET(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/ShaderLibrary")

  # create output implementation and header
  file(WRITE ${OUTPUT}.h
  "/* Generated file, do not edit! */\n\n"
  "#ifndef KINSKI_SHADER_LIBRARY\n"
  "#define KINSKI_SHADER_LIBRARY\n")
  file(WRITE ${OUTPUT}.cpp
    "/* Generated file, do not edit! */\n\n"
    "#include \"ShaderLibrary.h\"\n\n")

  # gather all shader files
  FILE(GLOB SHADER_FILES  "${FOLDER_NAME}/*.vert"
                          "${FOLDER_NAME}/*.geom"
                          "${FOLDER_NAME}/*.frag"
                          "${FOLDER_NAME}/*.glsl")

  foreach(SHADER_FILE ${SHADER_FILES})

    get_filename_component(FILENAME ${SHADER_FILE} NAME)
    string(REGEX REPLACE "[.]" "_" NAME ${FILENAME})

    file(STRINGS ${SHADER_FILE} LINES)

    file(APPEND ${OUTPUT}.h "extern char const* const ${NAME};\n")
    file(APPEND ${OUTPUT}.cpp "\nchar const* const ${NAME} = \n")

    foreach(LINE ${LINES})
      string(REPLACE "\"" "\\\"" LINE "${LINE}")
      file(APPEND ${OUTPUT}.cpp "   \"${LINE}\\n\"\n")
    endforeach(LINE)

    file(APPEND ${OUTPUT}.cpp ";\n")
  endforeach(SHADER_FILE)

  file(APPEND ${OUTPUT}.h "#endif")

endfunction(STRINGIFY_SHADERS)
