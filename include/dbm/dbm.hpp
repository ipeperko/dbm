#ifndef DBM_DBM_HPP
#define DBM_DBM_HPP

#include <dbm/dbm_common.hpp>
#include <dbm/sql_types.hpp>
#include <dbm/model.hpp>
#include <dbm/session.hpp>
#include <dbm/model.ipp>
#include <dbm/session.ipp>
#include <dbm/serializer.ipp>

namespace dbm {

using key = kind::key;
using tag = kind::tag;
using primary = kind::primary;
using required = kind::required;
using taggable = kind::taggable;
using not_null = kind::not_null;
using auto_increment = kind::auto_increment;
using create = kind::create;
using direction = kind::direction;
using defaultc = kind::defaultc;
using valquotes = kind::valquotes;
using custom_data_type = kind::custom_data_type;

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
DBM_EXPORT inline std::string version()
{
    return std::to_string(DBM_VERSION_MAJOR) + "." + std::to_string(DBM_VERSION_MINOR) + "." + std::to_string(DBM_VERSION_PATCH);
}

}

#endif //DBM_DBM_HPP
