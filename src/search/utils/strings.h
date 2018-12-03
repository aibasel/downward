#ifndef UTILS_STRINGS_H
#define UTILS_STRINGS_H


#include <sstream>
#include <string>

namespace utils {
void lstrip(std::string &s);

void rstrip(std::string &s);

void strip(std::string &s);

std::pair<std::string, std::string> split(
    const std::string &arg, const std::string &splitter = "=");

template<typename Collection>
std::string join(const Collection &collection, const std::string &delimiter) {
    std::ostringstream oss;
    bool first_item = true;

    for (const auto &item : collection) {
        if (first_item)
            first_item = false;
        else
            oss << delimiter;
        oss << item;
    }
    return oss.str();
}
}
#endif
