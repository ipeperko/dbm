#ifndef DBM_PREPARED_STATEMENT_IPP
#define DBM_PREPARED_STATEMENT_IPP

namespace dbm::kind {

DBM_INLINE prepared_statement& prepared_statement::operator>>(session& s)
{
    s.query(*this);
    return *this;
}

} // namespace kind::dbm

#endif //DBM_PREPARED_STATEMENT_IPP
