#ifndef DBM_PREPARED_STATEMENT_IPP
#define DBM_PREPARED_STATEMENT_IPP

namespace dbm::kind {

template<typename DBType>
DBM_INLINE prepared_statement& prepared_statement::operator>>(DBType& s)
{
    s.query(*this);
    return *this;
}

} // namespace kind::dbm

#endif //DBM_PREPARED_STATEMENT_IPP
