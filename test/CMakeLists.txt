set(BINARY ${CMAKE_PROJECT_NAME}_test)

find_package (Boost REQUIRED COMPONENTS system filesystem unit_test_framework)
include_directories (${Boost_INCLUDE_DIRS}
    ${PROJECT_BINARY_DIR}/include
    ../third_party/nlohmann)

message ("PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR}")

add_executable(${BINARY} main.cpp model_test.cpp sql_types_test.cpp xml_test.cpp)
target_link_libraries(${BINARY}
    dbm
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    )
add_test(NAME ${BINARY} COMMAND ${BINARY})
