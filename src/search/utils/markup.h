#ifndef UTILS_MARKUP_H
#define UTILS_MARKUP_H

#include <string>
#include <vector>

namespace utils {
extern std::string format_paper_reference(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &url, const std::string &conference,
    const std::string &pages, const std::string &publisher);
}

#endif
