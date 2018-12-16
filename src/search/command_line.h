#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <memory>
#include <stdexcept>
#include <string>

namespace options {
class Registry;
}

class SearchEngine;

class ArgError : public std::runtime_error {
public:
    explicit ArgError(const std::string &msg);
};

extern std::shared_ptr<SearchEngine> parse_cmd_line(
    int argc, const char **argv, options::Registry &registry, bool dry_run,
    bool is_unit_cost);

extern std::string usage(const std::string &progname);

#endif
