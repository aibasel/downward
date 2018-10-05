#ifndef UTILS_STRINGS_H
#define UTILS_STRINGS_H

#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace utils {
std::string sanitize_string(std::string s);

void ltrim(std::string &s);

void rtrim(std::string &s);

void trim(std::string &s);

std::pair<std::string, std::string> split(
    const std::string &arg, const std::string &splitter = "=");

template<typename Collection>
std::string join(const Collection &collection, const std::string &delimiter) {
    std::ostringstream oss;
    bool first_round = true;

    for (const auto &item : collection) {
        oss << (first_round ? "" : delimiter) << item;
        first_round = false;
    }
    return oss.str();
}
}
#endif
