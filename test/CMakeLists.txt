set(BINARY ${CMAKE_PROJECT_NAME}_test)

add_subdirectory(manual)

find_package (Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(${BINARY}
    # common files
    db_settings.cpp
    db_settings.h
    common.h
    no_driver_session.h

    # tests
    tst_basic_types.cpp
    tst_model.cpp
    tst_sql_types.cpp
    tst_xml.cpp
    tst_pool.cpp
    tst_prepared_stmt.cpp
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
    ${BINARY}
    COMMAND ${BINARY}
    )


#add_executable(${BINARY}_manual
#    test_manual.cpp
#    )
#
#target_link_libraries(${BINARY}_manual
#    dbm
#    pthread
#    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
#    )
#
#target_include_directories(${BINARY}_manual
#    PUBLIC
#    ${DBM_INCLUDE_DIRS}
#    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/nlohmann>
#    )

#add_test(NAME
#    ${BINARY}_manual
#    COMMAND ${BINARY}_manual
#    )