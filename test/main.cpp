#include <dbm/dbm.hpp>
#include <dbm/xml_serializer.hpp>
#include "no_driver_session.h"
#include <iomanip>

#define BOOST_TEST_MODULE dbm_test_module
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;

namespace dbm {

std::ostream& boost_test_print_type(std::ostream& os, const dbm::model_item& item)
{
    os << "key: '" << item.key().get() << "' tag: '" << item.tag().get() << "'";
    os << " value: ";
    if (item.get_container().is_null()) {
        os << "NULL";
    }
    else {
        os << "'" << item.to_string() << "'";
    }

    return os;
}

#ifdef DBM_EXPERIMENTAL_BLOB
std::ostream& operator<<(std::ostream& os, const dbm::blob& v)
{
    for (auto c : v) {
        os << c;
    }

    return os;
}
#endif

} // namespace dbm

BOOST_AUTO_TEST_CASE(narrow_cast_test)
{
    int v2 = 100000;
    BOOST_REQUIRE_THROW(dbm::utils::narrow_cast<short>(v2), std::exception);
}

BOOST_AUTO_TEST_CASE(named_type_operator_bool)
{
    struct param;
    typedef dbm::kind::detail::named_type<bool,
                                          param,
                                          dbm::kind::detail::printable,
                                          dbm::kind::detail::operator_bool> mybool;

    mybool v(true);
    BOOST_TEST(v.get());
    BOOST_TEST(v);

    bool b1(v);
    BOOST_TEST(b1);

    v = mybool(false);
    BOOST_TEST(!v.get());
    BOOST_TEST(!v);

    bool b2(v);
    BOOST_TEST(!b2);
}

BOOST_AUTO_TEST_CASE(key_test)
{
    dbm::key key1("key1");
    dbm::key key2(std::move(key1));
    BOOST_TEST(key1.empty());

    dbm::model model("dummy",
                     { { dbm::local<int>(), dbm::tag("tag_int"), dbm::key("key_int") }, { dbm::local<int>(), dbm::tag("tag_int2"), dbm::key("key_int2") } });

    model.at(dbm::key("key_int")).set_value(123);
    BOOST_TEST(model.at(dbm::key("key_int")).value<int>() == 123);
    BOOST_TEST(model.at(dbm::key("key_int")).is_null() == false);
    BOOST_TEST(model.at(dbm::key("key_int2")).is_null());
}

BOOST_AUTO_TEST_CASE(container_test)
{
    struct Test
    {
        int test_int { -1 };
        double test_double { -1.1 };
        std::string test_string;
    };

    Test test_struct;

    auto local_int = dbm::local<int /*, dbm::container::no_int_to_double*/>();
    BOOST_TEST(local_int->is_null());
    BOOST_TEST(not local_int->is_defined());
    local_int->from_string("111");
    BOOST_TEST(not local_int->is_null());
    BOOST_TEST(local_int->is_defined());
    BOOST_TEST(local_int->to_string() == "111");
    BOOST_TEST(std::get<int>(local_int->get()) == 111);
    local_int->set(nullptr);
    BOOST_TEST(local_int->is_null());
    BOOST_TEST(local_int->is_defined());
    local_int->set(222);
    BOOST_TEST(not local_int->is_null());
    BOOST_TEST(std::get<int>(local_int->get()) == 222);
    local_int->set(long(333));
    BOOST_TEST(std::get<int>(local_int->get()) == 333);
    local_int->set(short(444));
    BOOST_TEST(std::get<int>(local_int->get()) == 444);
    local_int->set(true);
    BOOST_TEST(std::get<int>(local_int->get()) == 1);
    local_int->set(false);
    BOOST_TEST(std::get<int>(local_int->get()) == 0);
    BOOST_TEST(local_int->get<int>() == local_int->clone()->get<int>());
    BOOST_REQUIRE_THROW(local_int->from_string("abc"), std::exception);
    BOOST_TEST(local_int->is_null());
    BOOST_TEST(not local_int->is_defined());
    local_int->set(3.14);
    BOOST_TEST(std::get<int>(local_int->get()) == 3);
    BOOST_TEST(not local_int->is_null());
    BOOST_TEST(local_int->is_defined());

    auto local_short = dbm::local<short /*, dbm::container::no_int_to_double*/>();
    local_short->from_string("2");
    BOOST_TEST(local_short->to_string() == "2");
    BOOST_TEST(local_short->get<short>() == 2);
    BOOST_TEST(std::get<short>(local_short->get()) == 2);
    BOOST_REQUIRE_THROW(local_short->set(int(100000)), std::exception);
    BOOST_TEST(local_short->get<short>() == local_short->clone()->get<short>());

    auto local_double = dbm::local<double>();
    local_double->from_string("3.14");
    BOOST_TEST(local_double->get<double>() == 3.14);
    local_double->set(6.66);
    BOOST_TEST(local_double->get<double>() == 6.66);
    local_double->set(42);
    BOOST_TEST(local_double->get<double>() == 42);
    BOOST_TEST(local_double->get<double>() == local_double->clone()->get<double>());

    auto bind_int = dbm::binding(test_struct.test_int);
    BOOST_TEST(!bind_int->is_null());
    test_struct.test_int = 333;
    bind_int->set_null(false);
    BOOST_TEST(std::get<int>(bind_int->get()) == 333);
    bind_int->clone()->set(444);
    BOOST_TEST(test_struct.test_int == 444);
    BOOST_TEST(bind_int->get<int>() == 444);
    BOOST_TEST(bind_int->clone()->get<int>() == 444);
    BOOST_TEST(bind_int->get<int>() == bind_int->clone()->get<int>());

    auto local_string = dbm::local<std::string>();
    const char* local_string_value = "local_string value";
    local_string->from_string(local_string_value);
    BOOST_TEST(local_string->to_string() == local_string_value);
    BOOST_TEST(std::get<std::string>(local_string->get()) == local_string_value);
    BOOST_TEST(local_string->to_string() == local_string->clone()->to_string());
}

BOOST_AUTO_TEST_CASE(default_item_value_test)
{
    auto local_int = dbm::local<int>();
    BOOST_TEST(local_int->is_null());
    BOOST_TEST(! local_int->is_defined());

    auto local_int2 = dbm::local<int>(123);
    BOOST_TEST(local_int2->get<int>() == 123);
    BOOST_TEST(local_int2->is_defined());
    BOOST_TEST(! local_int2->is_null());

    int my_int = 1;
    auto bnd = dbm::binding(my_int);
    BOOST_TEST(! bnd->is_null());
    BOOST_TEST(bnd->is_defined());

    auto bnd2 = dbm::binding(my_int, nullptr, dbm::init_null::null, dbm::init_defined::not_defined);
    BOOST_TEST(bnd2->is_null());
    BOOST_TEST(! bnd2->is_defined());

    // Value set
    local_int->set(11);
    BOOST_TEST(! local_int->is_null());
    BOOST_TEST(local_int->is_defined());

    BOOST_REQUIRE_THROW(local_int->set("must fail"), std::exception);
    BOOST_TEST(local_int->is_null());
    BOOST_TEST(! local_int->is_defined());

    local_int->set_null(true);
    local_int->set_defined(false);
    BOOST_TEST(local_int->is_null());
    BOOST_TEST(! local_int->is_defined());
    local_int->set_null(false);
    BOOST_TEST(! local_int->is_null());
    BOOST_TEST(! local_int->is_defined());
    local_int->set_defined(true);
    BOOST_TEST(! local_int->is_null());
    BOOST_TEST(local_int->is_defined());

}

BOOST_AUTO_TEST_CASE(item_narrow_cast_test)
{
    BOOST_REQUIRE_THROW(dbm::local<short>()->set(100000), std::exception);
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set(30000));
    dbm::container::no_narrow_cast;

    auto loc = dbm::local<short, dbm::container::no_narrow_cast>();
    BOOST_REQUIRE_THROW(loc->set(1000), std::exception);

    BOOST_REQUIRE_NO_THROW(dbm::local<double>()->set(1));
    auto loc2 = dbm::local<double, dbm::container::no_int_to_double>();
    BOOST_REQUIRE_THROW(loc2->set(1000), std::exception);

    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set(1.1));
}

BOOST_AUTO_TEST_CASE(item_time_t_test)
{
    time_t t = 0;
    auto local_time_t = dbm::local<time_t>(t);
    BOOST_TEST(local_time_t->is_defined());
    BOOST_TEST(!local_time_t->is_null());
}

class EnumClass
{
public:
    typedef int MyEnum;
    enum
    {
        one = 1,
        two,
        three
    };
};

BOOST_AUTO_TEST_CASE(enum_container_test)
{
    typedef int MyEnum;
    enum : int
    {
        one = 1,
        two,
        three
    };

    enum class RealEnumClass : int
    {
        one = 1,
        two,
        three
    };

    {
        MyEnum num { one };
        auto bind = dbm::binding(num);
        bind->set(2);
        BOOST_TEST(num == 2);
    }

    {
        EnumClass::MyEnum num { EnumClass::one };
        auto bind = dbm::binding(num);
        bind->set(2);
        BOOST_TEST(num == 2);
    }

    {
        RealEnumClass num { RealEnumClass::one };
        auto bind = dbm::binding((std::underlying_type_t<RealEnumClass>&) (num));
        bind->set(2);
        BOOST_TEST(static_cast<std::underlying_type_t<RealEnumClass>>(num) == 2);
    }
}

BOOST_AUTO_TEST_CASE(container_test_string_type)
{
    auto cont = dbm::local<std::string>();
    cont->set(std::string("foo"));
    BOOST_TEST(cont->get<std::string>() == "foo");
    cont->set("bar");
    BOOST_TEST(cont->get<std::string>() == "bar");
    std::string_view view = "view";
    cont->set(view);
    BOOST_TEST(cont->get<std::string>() == "view");

    // copy string value
    std::string s = "astring";
    cont->set(s);
    BOOST_TEST(s == "astring");
    BOOST_TEST(cont->get<std::string>() == "astring");

    // move string value
    s = "bstring";
    cont->set(std::move(s));
    BOOST_TEST(s.empty());
    BOOST_TEST(cont->get<std::string>() == "bstring");
}

std::function<bool(const int&)> int_validator()
{
    return [](const int& val) {
        return val > 0;
    };
}

BOOST_AUTO_TEST_CASE(container_test_validator)
{
    // local container
    auto local_int = dbm::local<int>(0, [](const int& v) { return v >= 0 && v < 10; });
    BOOST_TEST(local_int->get<int>() == 0);
    BOOST_TEST(local_int->is_defined());
    BOOST_TEST(! local_int->is_null());
    local_int->set(1);
    BOOST_TEST(local_int->get<int>() == 1);
    BOOST_REQUIRE_THROW(local_int->set(20), std::exception);
    BOOST_TEST(local_int->is_null());

    dbm::local<int>(int_validator());
    dbm::local<int>(1, int_validator());

    // string container
    auto local_string = dbm::local<std::string>([](const std::string& s) { return s == "xxx"; });
    local_string->set("xxx");
    BOOST_TEST(local_string->get<std::string>() == "xxx");
    BOOST_REQUIRE_THROW(local_string->set("yyy"), std::exception);
    BOOST_TEST(local_string->is_null());

    // bind containers
    int val = 1;
    auto validator = [](const int& v)->bool { return v > 0 && v < 10; };
    auto bind = dbm::binding(val, validator);
    bind->set(5);
    BOOST_TEST(bind->get<int>() == 5);
    BOOST_REQUIRE_THROW(bind->set(20), std::exception);
    BOOST_TEST(bind->is_null());

    auto bind2 = dbm::binding<int>(val, [](const int& v) { return v > 0 && v < 10; });
    bind2->set(5);
    BOOST_TEST(bind2->get<int>() == 5);
    BOOST_REQUIRE_THROW(bind2->set(20), std::exception);
    BOOST_TEST(bind2->is_null());
}

BOOST_AUTO_TEST_CASE(default_constraint_test)
{
    // Default constructor
    {
        dbm::defaultc dc;
        BOOST_TEST(!dc.has_value());
        BOOST_REQUIRE_THROW(dc.is_null(), std::exception);
        BOOST_REQUIRE_THROW(dc.string_value(), std::exception);
    }

    // Constructor with nullopt
    {
        dbm::defaultc dc(std::nullopt);
        BOOST_TEST(!dc.has_value());
        BOOST_REQUIRE_THROW(dc.is_null(), std::exception);
        BOOST_REQUIRE_THROW(dc.string_value(), std::exception);
    }

    // Constructor with nullptr
    {
        dbm::defaultc dc(nullptr);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(dc.is_null());
    }

    // Constructor with std::string const reference
    {
        std::string c("INT");
        dbm::defaultc dc(c);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "INT");
    }

    // Constructor with std::string rvalue reference
    {
        dbm::defaultc dc(std::string("INT"));
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "INT");
    }

    // Constructor with const char*
    {
        const char* c = "INT";
        dbm::defaultc dc(c);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "INT");
    }

    // Constructor with std::string_view
    {
        std::string_view c = "INT";
        dbm::defaultc dc(c);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "INT");
    }

    // Constructor with int
    {
        int c = 42;
        dbm::defaultc dc(c);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "42");
    }

    // Constructor with int rvalue
    {
        dbm::defaultc dc(0);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "0");
    }

    // Constructor with double
    {
        double c = 3.14;
        dbm::defaultc dc(c);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(std::stod(dc.string_value()) == 3.14, boost::test_tools::tolerance(0.01));
    }

    // Constructor with double rvalue
    {
        dbm::defaultc dc(3.14);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(std::stod(dc.string_value()) == 3.14, boost::test_tools::tolerance(0.01));
    }

    // Constructor with boolean
    {
        dbm::defaultc dc(true);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "1");
    }

    // Constructor with boolean
    {
        dbm::defaultc dc(false);
        BOOST_TEST(dc.has_value());
        BOOST_TEST(!dc.is_null());
        BOOST_TEST(dc.string_value() == "0");
    }
}

BOOST_AUTO_TEST_CASE(item_test)
{
    static_assert(dbm::xml::utils::is_string_type<std::string>::value);
    static_assert(dbm::xml::utils::is_string_type<std::string_view>::value);
    static_assert(dbm::xml::utils::is_string_type<const char*>::value);

    //
    // Item test - integer container
    //
    BOOST_TEST_CHECKPOINT("Integer container with local storage");
    dbm::model_item mitem_int(dbm::local<int>(), dbm::tag("mitem_int_tag"), dbm::key("mitem_int"));
    BOOST_TEST(mitem_int.key().get() == "mitem_int");
    BOOST_TEST(mitem_int.tag().get() == "mitem_int_tag");
    BOOST_TEST(mitem_int.get_container().is_null());
    mitem_int.set_value(13);
    BOOST_TEST(!mitem_int.get_container().is_null());
    BOOST_TEST(mitem_int.value<int>() == 13);
    BOOST_TEST(std::get<int>(mitem_int.value()) == 13);
    mitem_int.from_string("1234");
    BOOST_TEST(mitem_int.value<int>() == 1234);
    BOOST_TEST(mitem_int.to_string() == "1234");

    // Primary key
    mitem_int.set(dbm::primary(true));
    BOOST_TEST(mitem_int.conf().primary());
    mitem_int.set(dbm::primary(false));
    BOOST_TEST(!mitem_int.conf().primary());

    // Required flag
    mitem_int.set(dbm::required(true));
    BOOST_TEST(mitem_int.conf().required());
    mitem_int.set(dbm::required(false));
    BOOST_TEST(!mitem_int.conf().required());

    // Taggable flag
    mitem_int.set(dbm::taggable (true));
    BOOST_TEST(mitem_int.conf().taggable());
    mitem_int.set(dbm::taggable (false));
    BOOST_TEST(!mitem_int.conf().taggable());

    // Item copy
    mitem_int.set(dbm::primary(!mitem_int.conf().primary()));
    mitem_int.set(dbm::required(!mitem_int.conf().required()));
    mitem_int.set(dbm::taggable(!mitem_int.conf().taggable()));
    dbm::model_item mitem_int_copy(mitem_int);
    BOOST_TEST(mitem_int.value<int>() == mitem_int_copy.value<int>());
    BOOST_TEST(mitem_int.conf().primary() == mitem_int_copy.conf().primary());
    BOOST_TEST(mitem_int.conf().required() == mitem_int_copy.conf().required());
    BOOST_TEST(mitem_int.conf().taggable() == mitem_int_copy.conf().taggable());

    // Item move
    dbm::model_item mitem_int_move(std::move(mitem_int));
    BOOST_TEST(mitem_int_move.to_string() == "1234");
    BOOST_TEST(mitem_int.is_null());
    BOOST_TEST(mitem_int.key().empty());
    BOOST_TEST(mitem_int.tag().empty());
    BOOST_TEST(!mitem_int_move.is_null());
}

class CustomException : public std::exception
{
    std::string e;
public:
    CustomException(const std::string& what)
        : e(what)
    {}

    const char* what() const noexcept override
    {
        return e.c_str();
    }
};

class CustomException_2
{
public:
    CustomException_2(const std::string& what)
    {}
};

BOOST_AUTO_TEST_CASE(custom_exception)
{
    try {
        dbm::config::set_custom_exception([](std::string const& what) {
            throw CustomException(what);
        });

        BOOST_REQUIRE_THROW(dbm::throw_exception("Test custom exception"), CustomException);

        dbm::config::set_custom_exception([](std::string const& what) {
            throw CustomException_2(what);
        });

        BOOST_REQUIRE_THROW(dbm::throw_exception("Test custom exception 2"), CustomException_2);
    }
    catch (...)
    {}

    dbm::config::set_custom_exception(nullptr);
}
