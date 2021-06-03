cmake_minimum_required(VERSION 3.10)
project(dbm VERSION 0.0.32 DESCRIPTION "Database table models with serialization " LANGUAGES CXX)

option(DBM_BUILD_TESTS "Build dbm tests" OFF)

# --------------------------------------------------------------------------------
# Library
# --------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
endif()

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

# Sources
set(SOURCES
    src/xml.cpp
    src/drivers/mysql/mysql_session.cpp
    src/drivers/sqlite/sqlite_session.cpp
    )

# Headers
set(HEADERS
    include/dbm/container.hpp
    include/dbm/dbm_common.hpp
    include/dbm/dbm.hpp
    include/dbm/dbm_qt.hpp
    include/dbm/model.hpp
    include/dbm/model_item.hpp
    include/dbm/nlohmann_json_serializer.hpp
    include/dbm/serializer.hpp
    include/dbm/session.hpp
    include/dbm/sql_row.hpp
    include/dbm/sql_rows_dump.hpp
    include/dbm/sql_rows.hpp
    include/dbm/sql_types.hpp
    include/dbm/sql_value.hpp
    include/dbm/xml.hpp
    include/dbm/xml_serializer.hpp

    include/dbm/impl/model.ipp
    include/dbm/impl/model_item.ipp
    include/dbm/impl/serializer.ipp
    include/dbm/impl/session.ipp

    include/dbm/detail/container_impl.hpp
    include/dbm/detail/default_constraint.hpp
    include/dbm/detail/model_query_helper.hpp
    include/dbm/detail/named_type.hpp
    include/dbm/detail/statement.hpp
    include/dbm/detail/timestamp2u_converter.hpp
    include/dbm/detail/utils.hpp

    include/dbm/detail/impl/container_impl.ipp

    include/dbm/drivers/mysql/mysql_session.hpp
    include/dbm/drivers/sqlite/sqlite_session.hpp
    )

# Objects
add_library(DBM_OBJECTS
    OBJECT
    ${SOURCES}
    ${HEADERS}
    )
set_target_properties(DBM_OBJECTS
    PROPERTIES
    POSITION_INDEPENDENT_CODE 1
    )
set(DBM_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/exports>
    $<INSTALL_INTERFACE:include>
    )
target_include_directories(DBM_OBJECTS PUBLIC
    ${DBM_INCLUDE_DIRS}
    )

# Compile static library
add_library(dbm_static
    STATIC
    $<TARGET_OBJECTS:DBM_OBJECTS>
    )
target_link_libraries(dbm_static ${LIBS})
set_target_properties(dbm_static
    PROPERTIES
    OUTPUT_NAME dbm
    )

# Compile shared library
add_library(dbm
    SHARED
    $<TARGET_OBJECTS:DBM_OBJECTS>
    )
target_link_libraries(dbm ${LIBS})
set_target_properties(dbm
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    CXX_VISIBILITY_PRESET hidden
    )

# Create export header file
include(GenerateExportHeader)
generate_export_header(dbm
    EXPORT_FILE_NAME exports/dbm/dbm_export.hpp)

# --------------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------------

add_library(dbm::dbm ALIAS dbm)

install(TARGETS dbm EXPORT dbm-targets DESTINATION
    LIBRARY  DESTINATION lib
    ARCHIVE  DESTINATION lib
    RUNTIME  DESTINATION bin
    INCLUDES DESTINATION include
    )

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dbm ${PROJECT_BINARY_DIR}/exports/dbm
    DESTINATION include
    )

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/dbm/dbm-config-version.cmake"
    VERSION ${dbm_VERSION}
    COMPATIBILITY AnyNewerVersion
    )

export(EXPORT dbm-targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/dbm/dbm-targets.cmake"
    NAMESPACE dbm::
    )

configure_file(cmake/dbm-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/dbm/dbm-config.cmake"
    COPYONLY
    )

set(ConfigPackageLocation lib/cmake/dbm)

install(
    EXPORT      dbm-targets
    FILE        dbm-targets.cmake
    NAMESPACE   dbm::
    DESTINATION ${ConfigPackageLocation}
    )

install(
    FILES
    cmake/dbm-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/dbm/dbm-config-version.cmake"
    DESTINATION ${ConfigPackageLocation}
    )

# --------------------------------------------------------------------------------
# Test
# --------------------------------------------------------------------------------

if (DBM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

