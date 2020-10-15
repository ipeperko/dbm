#ifndef DBM_DBM_QT_HPP
#define DBM_DBM_QT_HPP

#include "dbm.hpp"
#include <QDebug>

template<typename Kind, typename = typename std::enable_if_t<std::is_base_of_v<dbm::kind::detail::named_type_base, Kind>>>
QDebug operator<<(QDebug os, Kind const& obj)
{
    using value_type = typename Kind::value_type;

    if constexpr (std::is_same_v<std::string, value_type>) {
        os << obj.get().c_str();
    }
    else {
        os << obj.get();
    }

    return os;
}

#endif //DBM_DBM_QT_HPP
