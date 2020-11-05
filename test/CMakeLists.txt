set(BINARY ${CMAKE_PROJECT_NAME}_test)

find_package (Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(${BINARY}
    db_settings.cpp
    main.cpp
    model_test.cpp
    sql_types_test.cpp
    xml_test.cpp
    )

target_link_libraries(${BINARY}
    dbm
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
