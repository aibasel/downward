#ifndef OPTIONS_STRING_UTILS_H
#define OPTIONS_STRING_UTILS_H

#include <functional>
#include <sstream>
#include <string>
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

template<typename From, typename To, typename Collection>
std::vector<To> map_to_vector(const Collection &collection,
                              const std::function<To(const From &)> &func) {
    std::vector<To> transformed;
    transformed.reserve(collection.size());
    for (const From &item : collection) {
        transformed.push_back(func(item));
    }
    return transformed;
}

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
#endif
