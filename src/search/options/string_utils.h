#ifndef OPTIONS_STRING_UTILS_H
#define OPTIONS_STRING_UTILS_H

#include <functional>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

namespace options {
std::string sanitize_string(std::string s);

int parse_int_arg(const std::string &name, const std::string &value);

void ltrim(std::string &s);

void rtrim(std::string &s);

void trim(std::string &s);

std::pair<std::string, std::string> split(
    const std::string &arg, const std::string &splitter = "=");
}

template<typename InputIt>
std::string join(InputIt first, InputIt last, const std::string &delim = ", ") {
    return join<InputIt, std::string>(first, last, [](const std::string &str) {return str;}, delim);
}

template<typename InputIt, typename T>
std::string join(InputIt first, InputIt last, std::function<std::string(const T &)> f, const std::string &delim = ", ") {
    std::ostringstream oss;
    bool first_round = true;

    for (; first != last; ++first) {
        oss << (first_round ? "" : delim) << f(*first);
        first_round = false;
    }
    return oss.str();
}
#endif
