cmake_minimum_required(VERSION 3.10)
project(dbm VERSION 0.0.14 DESCRIPTION "Database table models with serialization " LANGUAGES CXX)

option(DBM_BUILD_TESTS "Build dbm tests" OFF)

# --------------------------------------------------------------------------------
# Library
# --------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
endif()

# Sources
set(SOURCES
    src/xml.cpp
    src/drivers/mysql/mysql_session.cpp
    src/drivers/sqlite/sqlite_session.cpp)

# Libraries
set (LIBS)

# MySQL library
find_library(MYSQL_LIB mysqlclient)
if (MYSQL_LIB)
    message("found libmysqlclient")
    add_definitions("-DDBM_MYSQL")
    list(APPEND LIBS "mysqlclient")
else ()
    message("libmysqlclient not found")
endif()

# SQLite library
find_library(SQLITE3_LIB sqlite3)
if (SQLITE3_LIB)
    message("found libsqlite3")
    add_definitions("-DDBM_SQLITE3")
    list(APPEND LIBS "sqlite3")
else ()
    message("libsqlite3 not found")
endif()

# Config header file
configure_file(dbm_config.hpp.in exports/dbm/dbm_config.hpp)

# Compile static library
add_library(dbm_static STATIC ${SOURCES})
target_link_libraries(dbm_static ${LIBS})
set_target_properties(dbm_static PROPERTIES OUTPUT_NAME dbm)
target_include_directories(dbm_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/exports>
    $<INSTALL_INTERFACE:include>
    )

# Compile shared library
add_library(dbm SHARED ${SOURCES})
target_link_libraries(dbm ${LIBS})
set_target_properties(dbm PROPERTIES
    VERSION ${PROJECT_VERSION}
    CXX_VISIBILITY_PRESET hidden
    )
target_include_directories(dbm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/exports>
    $<INSTALL_INTERFACE:include>
    )

# Create export header file
include(GenerateExportHeader)
generate_export_header(dbm
    EXPORT_FILE_NAME exports/dbm/dbm_export.hpp)

# --------------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------------

install(TARGETS dbm dbm_static DESTINATION lib)
install(DIRECTORY include/dbm ${PROJECT_BINARY_DIR}/exports/dbm DESTINATION include)

# --------------------------------------------------------------------------------
# Test
# --------------------------------------------------------------------------------

if (DBM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

