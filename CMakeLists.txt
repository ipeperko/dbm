cmake_minimum_required(VERSION 3.10)
project(dbm VERSION 0.0.11 DESCRIPTION "Database table models with serialization " LANGUAGES CXX)

option(DBM_BUILD_TESTS "Build dbm tests" OFF)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
endif()

configure_file(dbm.hpp.in include/dbm.hpp)

include_directories(PUBLIC include)

file(GLOB SOURCES
    src/*.cpp
    src/drivers/mysql/mysql_session.cpp
    src/drivers/sqlite/sqlite_session.cpp
    )

set (LIBS)

# MySQL library
find_library(MYSQL_LIB mysqlclient)
if (MYSQL_LIB)
    message("checking for libmysqlclient ... ok")
    add_definitions("-DDBM_MYSQL")
    list(APPEND LIBS "mysqlclient")
else ()
    message("checking for libmysqlclient ... not found")
endif()

# SQLite library
find_library(SQLITE3_LIB sqlite3)
if (SQLITE3_LIB)
    message("checking for libsqlite3 ... ok")
    add_definitions("-DDBM_SQLITE3")
    list(APPEND LIBS "sqlite3")
else ()
    message("checking for libsqlite3 ... not found")
endif()

# Compile static library
add_library(dbm_static STATIC ${SOURCES})
target_link_libraries(dbm_static ${LIBS})
set_target_properties(dbm_static PROPERTIES OUTPUT_NAME dbm)
target_include_directories(dbm_static PUBLIC
    "${PROJECT_BINARY_DIR}"
    )

# Compile shared library
add_library(dbm SHARED ${SOURCES})
target_link_libraries(dbm ${LIBS})
set_target_properties(dbm PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(dbm PUBLIC
    "${PROJECT_BINARY_DIR}"
    )
# Install library
install(TARGETS dbm dbm_static DESTINATION lib)
install(DIRECTORY include/ ${PROJECT_BINARY_DIR}/include/ DESTINATION include/dbm)

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "dbm")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "dbm - Database table models with serialization")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${dbm_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${dbm_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${dbm_VERSION_PATCH}")
include(CPack)

# Tests
if (DBM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

