set(BINARY ${CMAKE_PROJECT_NAME}_test)

find_package (Boost REQUIRED COMPONENTS system filesystem unit_test_framework)
include_directories (${Boost_INCLUDE_DIRS} ../third_party/nlohmann)
#add_definitions (-DBOOST_TEST_DYN_LINK)
add_executable(${BINARY} main.cpp model_test.cpp sql_types_test.cpp xml_test.cpp hello_world.cpp)
target_link_libraries(${BINARY}
    dbm
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    )
add_test(NAME ${BINARY} COMMAND ${BINARY})
