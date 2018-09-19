#ifndef OPTIONS_STRING_UTILS_H
#define OPTIONS_STRING_UTILS_H

#include <string>
#include <utility>

namespace options {
std::string sanitize_string(std::string s);

int parse_int_arg(const std::string &name, const std::string &value);

void ltrim(std::string &s);

void rtrim(std::string &s);

void trim(std::string &s);

std::pair<std::string, std::string> split(
    const std::string &arg, const std::string &splitter = "=");
}
#endif
