#ifndef DBM_PREPARED_STATEMENT_HPP
#define DBM_PREPARED_STATEMENT_HPP

#include <dbm/container.hpp>

namespace dbm::kind {

class prepared_statement
{
public:
    prepared_statement() = default;

    explicit prepared_statement(std::string stmt)
        : stmt_(std::move(stmt))
    {}

    ~prepared_statement() = default;

    void set_statement(std::string stmt)
    {
        stmt_ = std::move(stmt);
        parms_.clear();
    }

    auto const& statement() const { return stmt_; }

    void push(dbm::container_ptr&& p)
    {
        parms_.push_back(std::move(p));
    }

    auto& param(size_t idx)
    {
        return *parms_.at(idx);
    }

    auto const& param(size_t idx) const
    {
        return *parms_.at(idx);
    }

    auto size() const { return parms_.size(); }

private:
    std::string stmt_;
    std::vector<dbm::container_ptr> parms_;
};

} // namespace dbm

#endif //DBM_PREPARED_STATEMENT_HPP
