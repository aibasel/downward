#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <memory>
#include <string>

namespace options {
class Registry;
}

class SearchEngine;

struct ArgError {
    std::string msg;

    ArgError(const std::string &msg);

    friend std::ostream &operator<<(std::ostream &out, const ArgError &err);
};

extern std::shared_ptr<SearchEngine> parse_cmd_line(
    int argc, const char **argv, options::Registry &registry, bool dry_run,
    bool is_unit_cost);

extern std::string usage(const std::string &progname);

#endif
