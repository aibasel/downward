#ifndef UTILS_STRINGS_H
#define UTILS_STRINGS_H

#include <sstream>
#include <string>

namespace utils {
extern void lstrip(std::string &s);

extern void rstrip(std::string &s);

extern void strip(std::string &s);

/*
  Splits a given string at the first occurrence of splitter and optionally
  strips the whitespace around those fragments.
 */
extern std::pair<std::string, std::string> split(
    const std::string &str, const std::string &separator, bool strip);

extern bool startswith(const std::string &str, const std::string &prefix);

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
