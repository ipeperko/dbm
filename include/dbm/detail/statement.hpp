#ifndef DBM_STATEMENT_HPP
#define DBM_STATEMENT_HPP

#include <sstream>

namespace dbm::detail {

class statement
{
public:
    statement() = default;

    template <typename T>
    statement& operator<< (const T& val)
    {
        os_ << val;
        return *this;
    }

    [[nodiscard]] std::string get() const
    {
        return os_.str();
    }

    void clear()
    {
        os_ = std::ostringstream{};
    }

private:
    std::ostringstream os_;
};

}

#endif //DBM_STATEMENT_HPP
