#ifndef UTILS_LANGUAGE_H
#define UTILS_LANGUAGE_H

#include <string>
#include <typeindex>

#if defined(_MSC_VER)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

namespace utils {
template<typename T>
void unused_variable(const T &) {
}

template<typename T>
static std::string get_type_name() {
    bool unsupported_compiler = false;
#if defined(__clang__)
    std::string prefix = "[T = ";
    std::string suffix = "]";
    std::string function = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    std::string prefix = "with T = ";
    std::string suffix = "; ";
    std::string function = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    std::string prefix = "get_type_name<";
    std::string suffix = ">(void)";
    std::string function = __FUNCSIG__;
#else
    unsupported_compiler = true;
#endif
    if (unsupported_compiler) {
        return std::type_index(typeid(T)).name();
    } else {
        const auto start = function.find(prefix) + prefix.size();
        const auto end = function.find(suffix);
        const auto size = end - start;
        return function.substr(start, size);
    }
}
}

#endif
