#ifndef DBM_DBM_HPP
#define DBM_DBM_HPP

#include "dbm_common.hpp"
#include "sql_types.hpp"
#include "model.hpp"

namespace dbm {

using key = kind::key;
using tag = kind::tag;
using primary = kind::primary;
using required = kind::required;
using dbtype = kind::dbtype;
using null = kind::null;
using taggable = kind::taggable;

#ifdef DBM_EXPERIMENTAL_BLOB
using blob = kind::blob;
#endif
using variant = kind::variant;

using sql_row = kind::sql_row;
using sql_rows = kind::sql_rows;
using sql_rows_dump = kind::sql_rows_dump;

}

#endif //DBM_DBM_HPP
