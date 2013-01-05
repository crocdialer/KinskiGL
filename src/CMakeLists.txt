function(addTestMacro)
if(BUILD_TESTS)
    FILE(GLOB TEST_SOURCES "tests/*.c*")
    FILE(GLOB TEST_HEADERS "tests/*.h")

#SOURCE_GROUP("Unit-Tests" FILES ${TEST_SOURCES} ${TEST_HEADERS})

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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_SUBDIRECTORY("kinskiCore")
SET(LIBS ${LIBS} "kinskiCore")


ADD_SUBDIRECTORY("kinskiGL")
INCLUDE_DIRECTORIES("kinskiGL")
SET(LIBS ${LIBS} "kinskiGL")

if(NOT (BUILD_IOS OR KINSKI_RASPI))
ADD_SUBDIRECTORY("AntTweakBar")
INCLUDE_DIRECTORIES("AntTweakBar")
SET(LIBS ${LIBS} "AntTweakBar")
endif()

#ADD_SUBDIRECTORY("kinskiCV")
#SET(LIBS ${LIBS} "kinskiCV")

if(BUILD_APPLIB)
ADD_SUBDIRECTORY("kinskiApp")
SET(LIBS ${LIBS} "kinskiApp")
endif(BUILD_APPLIB)



if(BUILD_SAMPLES)
    if(KINSKI_RASPI)
    ADD_SUBDIRECTORY("samples_raspi")
    else()
    ADD_SUBDIRECTORY("samples")
    endif(KINSKI_RASPI)
endif(BUILD_SAMPLES)
