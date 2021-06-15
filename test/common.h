#ifndef DBM_COMMON_H
#define DBM_COMMON_H

#include <string>
#include <optional>
#include <boost/test/unit_test.hpp>

inline std::optional<std::string_view> get_cmlopt_option(std::string_view opt, std::optional<std::string_view> default_val = std::nullopt)
{
    using namespace boost::unit_test;

    int argc = framework::master_test_suite().argc;

    for (int i = 0; i < argc - 1; i++) {
        if (std::string_view(framework::master_test_suite().argv[i]) == opt && i < argc - 1) {
            return framework::master_test_suite().argv[i + 1];
        }
    }

    return default_val ? default_val : std::nullopt;
}

inline int get_cmlopt_nun_runs(int default_val)
{
    if (auto vals = get_cmlopt_option("--repeat-runs"))
        return std::stoi(vals->data());
    else
        return default_val;
}


#endif //DBM_COMMON_H
