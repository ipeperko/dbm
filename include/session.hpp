#ifndef DBM_SESSION_HPP
#define DBM_SESSION_HPP

#include "dbm_common.hpp"
#include "sql_types.hpp"

namespace dbm {

class model;

class session
{
public:
    class transaction;

    session() = default;
    session(const session& oth);
    session(session&& oth) noexcept;
    session& operator=(const session& oth) ;
    session& operator=(session&& oth) noexcept;
    virtual ~session() = default;

    virtual void open() = 0;
    virtual void close() = 0;

    virtual void query(const std::string& statement);
    kind::sql_rows select(const std::string& statement);
    kind::sql_rows select(const std::vector<std::string>& what, const std::string& table, const std::string& criteria="");

    virtual std::string write_model_query(const model& m) const = 0;
    virtual std::string read_model_query(const model& m, const std::string& extra_condition="") const = 0;
    virtual std::string delete_model_query(const model& m) const = 0;
    virtual std::string create_table_query(const model& m, bool if_not_exists) const = 0;
    virtual std::string drop_table_query(const model& m, bool if_exists) const = 0;

    virtual void transaction_begin() = 0;
    virtual void transaction_commit() = 0;
    virtual void transaction_rollback() = 0;

    void create_database(const std::string& db_name, bool if_not_exists);
    void drop_database(const std::string& db_name, bool if_exists);
    void create_table(const model& m, bool if_not_exists);
    void create_table(const std::string& tbl_name, std::vector<std::string> fields, bool if_not_exists);
    void drop_table(const model& m, bool if_exists);
    void drop_table(const std::string& tbl_name, bool if_exists);

    std::string const& last_statement() const { return mstatement; }
    std::string last_statement_info() const;

    model& operator>>(model& m);
    model&& operator>>(model&& m);

protected:
    virtual kind::sql_rows select_rows(const std::string& statement) { return {}; }

    std::string mstatement;
};

inline session::session(const session& oth)
    : mstatement(oth.mstatement)
{
}

inline session::session(session&& oth) noexcept
    : mstatement(std::move(oth.mstatement))
{
}

inline session& session::operator=(const session& oth)
{
    if (this != &oth) {
        mstatement = oth.mstatement;
    }
    return *this;
}

inline session& session::operator=(session&& oth) noexcept
{
    if (this != &oth) {
        mstatement = std::move(oth.mstatement);
    }
    return *this;
}


class session::transaction
{
public:
    explicit transaction(session& db)
        : db_(db)
    {
        db_.transaction_begin();
    }

    ~transaction()
    {
        perform(false);
    }

    void commit()
    {
        perform(true);
    }

    void rollback()
    {
        perform(false);
    }


private:

    void perform(bool do_commit);

    dbm::session& db_;
    bool executed_ {false};
};

inline void session::transaction::perform(bool do_commit)
{
    if (!executed_) {
        if (do_commit) {
            db_.transaction_commit();
        }
        else {
            db_.transaction_rollback();
        }
        executed_ = true;
    }
}

}// namespace dbm

#endif//DBM_SESSION_HPP
