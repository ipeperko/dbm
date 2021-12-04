set(BINARY ${CMAKE_PROJECT_NAME}_test_manual)

find_package (Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(${BINARY}
    ../common.h
    ../db_settings.cpp
    ../db_settings.h

    test_manual.cpp
    )

target_link_libraries(${BINARY}
    dbm
    pthread
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    )

target_include_directories(${BINARY}
    PUBLIC
    ${DBM_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/nlohmann>
    )

add_test(NAME
    ${BINARY}_manual
    COMMAND ${BINARY}
    )