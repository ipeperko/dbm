#ifndef DBM_PREPARED_STATEMENT_HPP
#define DBM_PREPARED_STATEMENT_HPP

#include <dbm/container.hpp>

class session;

namespace dbm::kind {

class prepared_statement
{
    friend class prepared_statement_manipulator;
public:
//    prepared_statement() = default;

    explicit prepared_statement(std::string stmt)
        : stmt_(std::move(stmt))
    {
//        prepare();
    }

    ~prepared_statement() = default;

//    void set_statement(std::string stmt)
//    {
//        stmt_ = std::move(stmt);
//        parms_.clear();
//    }

//    void query();
//
//    kind::sql_rows select();

    auto const& statement() const { return stmt_; }

//    auto& get_model() { return model_; }
//
//    auto const& get_model() const { return model_; }
//
//    auto& get_session() { return session_; }
//
//    auto const& get_session() const { return session_; }


    void push(container* p)
    {
        parms_.push_back(p);
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

//    void set_native_handle(void* p) { native_handle_ = p; } // TODO: hide this

private:
//    void prepare();

//    model& model_;
//    session& session_;
    std::string stmt_; // TODO: std::shared_ptr<std::string const>
    std::vector<container*> parms_;
    //std::vector<container_ptr> parms_local_; // TODO: local storage optional
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

} // namespace dbm

#endif //DBM_PREPARED_STATEMENT_HPP
