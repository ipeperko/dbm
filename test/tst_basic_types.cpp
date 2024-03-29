#include <dbm/dbm.hpp>
#include <dbm/xml_serializer.hpp>
#include <iomanip>

#define BOOST_TEST_MODULE dbm_basic_types
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;

namespace dbm {

std::ostream& boost_test_print_type(std::ostream& os, const dbm::model_item& item)
{
    os << "key: '" << item.key().get() << "' tag: '" << item.tag().get() << "'";
    os << " value: ";
    if (item.get().is_null()) {
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


BOOST_AUTO_TEST_SUITE(TstBasicTypes)

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

BOOST_AUTO_TEST_CASE(container_test, *tolerance(0.00001))
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
    local_int->set((int64_t)(333));
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

    auto loc = dbm::local<short, dbm::container::no_narrow_cast>();
    BOOST_REQUIRE_THROW(loc->set(1000), std::exception);

    BOOST_REQUIRE_NO_THROW(dbm::local<double>()->set(1));
    auto loc2 = dbm::local<double, dbm::container::no_int_to_double>();
    BOOST_REQUIRE_THROW(loc2->set(1000), std::exception);

    // Set double value to integer container
    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set(1.1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned int>()->set(1.1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int64_t>()->set(1.1));
    BOOST_REQUIRE_NO_THROW(dbm::local<uint64_t>()->set(1.1));
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set(1.1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned short>()->set(1.1));

    // short container - set value of a different integer type
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set(true));
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set((int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set((int64_t)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set((unsigned short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set((unsigned int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<short>()->set((uint64_t)1));

    // int container - set value of a different integer type
    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set(true));
    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set((short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set((int64_t)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set((unsigned short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set((unsigned int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int>()->set((uint64_t)1));

    // int container - set value of a different integer type
    BOOST_REQUIRE_NO_THROW(dbm::local<int64_t>()->set(true));
    BOOST_REQUIRE_NO_THROW(dbm::local<int64_t>()->set((short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int64_t>()->set((int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int64_t>()->set((unsigned short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int64_t>()->set((unsigned int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<int64_t>()->set((uint64_t)1));

    // unsigned short container - set value of a different integer type
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned short>()->set(true));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned short>()->set((short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned short>()->set((int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned short>()->set((int64_t)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned short>()->set((unsigned int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned short>()->set((uint64_t)1));

    // int container - set value of a different integer type
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned int>()->set(true));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned int>()->set((short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned int>()->set((int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned int>()->set((int64_t)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned int>()->set((unsigned short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<unsigned int>()->set((uint64_t)1));

    // int container - set value of a different integer type
    BOOST_REQUIRE_NO_THROW(dbm::local<uint64_t>()->set(true));
    BOOST_REQUIRE_NO_THROW(dbm::local<uint64_t>()->set((short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<uint64_t>()->set((int)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<uint64_t>()->set((int64_t)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<uint64_t>()->set((unsigned short)1));
    BOOST_REQUIRE_NO_THROW(dbm::local<uint64_t>()->set((unsigned int)1));
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

namespace {
auto int_validator = [](const int& val) {
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

    dbm::local<int>(int_validator);
    dbm::local<int>(1, int_validator);

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
        BOOST_REQUIRE_THROW([[maybe_unused]] auto r = dc.is_null(), std::exception);
        BOOST_REQUIRE_THROW([[maybe_unused]] auto r = dc.string_value(), std::exception);
    }

    // Constructor with nullopt
    {
        dbm::defaultc dc(std::nullopt);
        BOOST_TEST(!dc.has_value());
        BOOST_REQUIRE_THROW([[maybe_unused]] auto r = dc.is_null(), std::exception);
        BOOST_REQUIRE_THROW([[maybe_unused]] auto r = dc.string_value(), std::exception);
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

BOOST_AUTO_TEST_CASE(timestamp2u_test)
{
    using dbm::timestamp2u_converter;

    {
        // Default constructor
        timestamp2u_converter tu;
        BOOST_TEST((tu == 0));
        BOOST_TEST(tu.get() == 0);
        BOOST_TEST(tu.has_reference() == false);

        // Assign value
        tu = 42;
        BOOST_TEST((tu == 42));
        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference() == false);

        // Assign value 2
        tu = time_t(13);
        BOOST_TEST(tu.get() == 13);
        BOOST_TEST(tu.has_reference() == false);
    }

    {
        // Construct from time_t
        timestamp2u_converter tu(42);
        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference() == false);
    }

    {
        // Construct from time_t reference
        time_t storage = 42;
        timestamp2u_converter tu(storage);
        BOOST_TEST((tu == 42));
        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference());

        storage = 13;
        BOOST_TEST(tu.get() == 13);

        tu = 14;
        BOOST_TEST(tu.get() == 14);
        BOOST_TEST(storage == 14);
    }

    {
        // Copy construct local storage
        timestamp2u_converter tu(42);
        timestamp2u_converter tu2(tu);

        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference() == false);
        BOOST_TEST(tu2.get() == 42);
        BOOST_TEST(tu2.has_reference() == false);

        tu = 43;
        BOOST_TEST(tu.get() == 43);
        BOOST_TEST(tu2.get() == 42);
    }

    {
        // Copy construct reference storage
        time_t storage = 42;
        timestamp2u_converter tu(storage);
        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference());

        timestamp2u_converter tu2 = tu;

        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference());
        BOOST_TEST(tu2.get() == 42);
        BOOST_TEST(tu2.has_reference());

        // Modify external value
        storage = 43;
        BOOST_TEST(tu.get() == 43);
        BOOST_TEST(tu2.get() == 43);

        // Assignment
        tu = 44;
        BOOST_TEST(storage == 44);
        BOOST_TEST(tu.get() == 44);
        BOOST_TEST(tu2.get() == 44);
    }

    {
        // Move construct local storage
        timestamp2u_converter tu(42);
        timestamp2u_converter tu2(std::move(tu));

        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference() == false);
        BOOST_TEST(tu2.get() == 42);
        BOOST_TEST(tu2.has_reference() == false);
    }

    {
        // Move construct reference storage
        time_t storage = 42;
        timestamp2u_converter tu(storage);
        BOOST_TEST(tu.get() == 42);
        BOOST_TEST(tu.has_reference());

        timestamp2u_converter tu2 = std::move(tu);

        BOOST_TEST(tu.has_reference() == false);
        BOOST_TEST(tu2.get() == 42);
        BOOST_TEST(tu2.has_reference());

        // Modify external value
        storage = 43;
        BOOST_TEST(tu2.get() == 43);

        // Assignment
        tu2 = 44;
        BOOST_TEST(storage == 44);
        BOOST_TEST(tu2.get() == 44);
    }

    {
        // Model item set/get value
        dbm::model_item mitem(dbm::timestamp(11));
        BOOST_TEST(mitem.value<dbm::timestamp2u_converter>().get() == 11);
        BOOST_TEST(mitem.value<int64_t>() == 11);
        mitem.set_value(12);
        BOOST_TEST(mitem.value<int64_t>() == 12);
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
    BOOST_TEST(mitem_int.get().is_null());
    mitem_int.set_value(13);
    BOOST_TEST(!mitem_int.get().is_null());
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

BOOST_AUTO_TEST_CASE(query_test)
{
    dbm::statement q;
    q << "SELECT * FROM whatever WHERE fancy=" << 1;

    // Test move ctor
    dbm::statement q2(std::move(q));
    BOOST_TEST(q.get() == "");
    BOOST_TEST(q2.get() == "SELECT * FROM whatever WHERE fancy=1");

    // Test move assign
    dbm::statement q3;
    q3 = std::move(q2);
    BOOST_TEST(q2.get() == "");
    BOOST_TEST(q3.get() == "SELECT * FROM whatever WHERE fancy=1");

    q3.clear();
    BOOST_TEST(q3.get() == "");
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

BOOST_AUTO_TEST_CASE(timer_timeouts, * tolerance(0.1))
{
    using Clock = std::chrono::steady_clock;
    using Timer = dbm::utils::timer<Clock>;

    auto t = Timer();
    bool flag;
    Timer::time_point_t tp1, tp2;
    std::chrono::duration<double, std::milli> dt;

    // Expires after
    flag = false;
    tp1 = Clock::now();
    t.expires_after(std::chrono::milliseconds (1000), [&]() {
        flag = true;
        tp2 = Clock::now();
    });

    BOOST_TEST(flag == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    BOOST_TEST(flag == false);
    t.wait();
    dt = tp2 - tp1;
    BOOST_TEST(flag == true);
    BOOST_TEST(dt.count() == 1000.0);

    // Expires at
    flag = false;
    tp1 = Clock::now();
    t.expires_at(tp1 + std::chrono::milliseconds (1000), [&]() {
        flag = true;
        tp2 = Clock::now();
    });

    BOOST_TEST(flag == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    BOOST_TEST(flag == false);
    t.wait();
    dt = tp2 - tp1;
    BOOST_TEST(flag == true);
    BOOST_TEST(dt.count() == 1000.0);

    // Cancel
    flag = false;
    tp1 = tp2 = Clock::now();
    t.expires_after(std::chrono::seconds(2), [&]() {
        flag = true;
        tp2 = Clock::now();
    });

    BOOST_TEST(flag == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    t.cancel();
    BOOST_TEST(flag == false);
    std::this_thread::sleep_for(std::chrono::milliseconds (500 + 200));
    dt = tp2 - tp1;
    BOOST_TEST(flag == false); // did not set the flag to true
    BOOST_TEST(dt.count() == 0);
}

class TimerRepeat
{
public:

    ~TimerRepeat()
    {
        timer.cancel();

        if (thr.joinable())
            thr.join();
        BOOST_TEST_MESSAGE("TimerRepeat bye");
    }

    void run()
    {
        timer.expires_after(interval, nullptr);

        if (thr.joinable())
            thr.join();

        thr = std::thread([this]{
            while (!timer.canceled()) {
                timer.wait();
                if (timer.canceled())
                    break;
                on_timeout();
                timer.expires_after(interval, nullptr);
            }
        });
    }


    void on_timeout()
    {
        BOOST_TEST_MESSAGE("TimerRepeat::on_timeout " << ++count);
    }

    dbm::utils::timer<std::chrono::steady_clock> timer;
    std::chrono::seconds interval {1};
    std::atomic<int> count {0};
    int count_max {2};
    std::thread thr;
};

BOOST_AUTO_TEST_CASE(timer_repeat)
{
    TimerRepeat rep;
    rep.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    BOOST_TEST(rep.count == 3);
    rep.timer.cancel();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    BOOST_TEST(rep.count == 3);

    rep.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    BOOST_TEST(rep.count == 6);
    rep.timer.cancel();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    BOOST_TEST(rep.count == 6);
}

BOOST_AUTO_TEST_CASE(execute_at_exit)
{
    int val = 0;
    {
        dbm::utils::execute_at_exit ex([&]{
            val = 2;
        });

        val = 1;
    }

    BOOST_TEST(val == 2);
}

BOOST_AUTO_TEST_SUITE_END()