#ifndef UTILS_STRINGS_H
#define UTILS_STRINGS_H

#include "exceptions.h"

#include <sstream>
#include <string>
#include <vector>

namespace utils {
extern void lstrip(std::string &s);
extern void rstrip(std::string &s);
extern void strip(std::string &s);

/*
  Split a given string at the first max_splits occurrence of separator. Use
  the default of -1 for an unlimited amount of splits.
*/
extern std::vector<std::string> split(
    const std::string &s, const std::string &separator, int max_splits = -1);

extern bool startswith(const std::string &s, const std::string &prefix);

extern std::string tolower(std::string s);

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

extern bool is_alpha_numeric(const std::string &s);
}
#endif
