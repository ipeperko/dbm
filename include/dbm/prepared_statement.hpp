#ifndef DBM_PREPARED_STATEMENT_HPP
#define DBM_PREPARED_STATEMENT_HPP

#include <dbm/container.hpp>

//namespace dbm {
//class session;
//}

namespace dbm::kind {

class prepared_statement
{
    friend class prepared_statement_manipulator;
public:

    template<typename ...Args>
    explicit prepared_statement(std::string stmt, Args&&... args)
        : stmt_(std::move(stmt))
    {
        push_helper(std::forward<Args>(args)...);
    }

    prepared_statement(prepared_statement&&) = default;

    ~prepared_statement() = default;

    prepared_statement& operator=(prepared_statement&&) = default;

    auto const& statement() const { return stmt_; }

    void push(container* p)
    {
        parms_.push_back(p);
    }

    void push(container_ptr&& p)
    {
        parms_.push_back(p.get());
        parms_local_.push_back(std::move(p));
    }

    auto& parms() { return parms_; }

    auto const& parms() const { return parms_; }

    container* param(size_t idx)
    {
        return parms_.at(idx);
    }

    container* const param(size_t idx) const
    {
        return parms_.at(idx);
    }

    auto size() const { return parms_.size(); }

    auto native_handle() { return native_handle_; }

    template<typename DBType>
    prepared_statement& operator>>(DBType& s);

private:
    void push_helper()
    {}

    template<typename Head, typename... Tail>
    void push_helper(Head&& head, Tail&&... tail)
    {
        push(std::forward<Head>(head));
        push_helper(std::forward<Tail>(tail)...);
    }

    std::string stmt_; // TODO: std::shared_ptr<std::string const> ?
    std::vector<container*> parms_;
    std::vector<container_ptr> parms_local_;
    void* native_handle_ {nullptr};
};

class prepared_statement_manipulator
{
public:
    explicit prepared_statement_manipulator(prepared_statement& ps)
        : ps_(ps)
    {}

    void set_native_handle(void* p) { ps_.native_handle_ = p; }

private:
    prepared_statement& ps_;
};

} // namespace kind::dbm

#endif //DBM_PREPARED_STATEMENT_HPP
