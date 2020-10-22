#ifndef DBM_DBM_HPP
#define DBM_DBM_HPP

#include "dbm_config.hpp"
#include "dbm_common.hpp"
#include "sql_types.hpp"
#include "model.hpp"
#include "session.hpp"
#include "model.ipp"
#include "session.ipp"
#include "serializer.ipp"

namespace dbm {

using key = kind::key;
using tag = kind::tag;
using primary = kind::primary;
using required = kind::required;
using dbtype = kind::dbtype;
using null = kind::null;
using taggable = kind::taggable;
using direction = kind::direction;

#ifdef DBM_EXPERIMENTAL_BLOB
using blob = kind::blob;
#endif
using variant = kind::variant;

using sql_row = kind::sql_row;
using sql_rows = kind::sql_rows;
using sql_rows_dump = kind::sql_rows_dump;

/*!
 * Get library version string
 *
 * @return version string
 */
inline std::string version()
{
    return std::to_string(DBM_VERSION_MAJOR) + "." + std::to_string(DBM_VERSION_MINOR) + "." + std::to_string(DBM_VERSION_PATCH);
}

}

#endif //DBM_DBM_HPP
