#ifndef DBM_QUERY_HPP
#define DBM_QUERY_HPP

#include <sstream>

namespace dbm::detail {

class query
{
public:
    query() = default;

    template <typename T>
    query& operator<< (const T& val)
    {
        os_ << val;
        return *this;
    }

    [[nodiscard]] std::string get() const
    {
        return os_.str();
    }

private:
    std::ostringstream os_;
};

}

#endif //DBM_QUERY_HPP
