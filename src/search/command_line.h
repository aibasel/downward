#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include "utils/exceptions.h"

#include <memory>
#include <string>

namespace plugins {
class Registry;
}

class SearchEngine;

class ArgError : public utils::Exception {
public:
    using Exception::Exception;

    virtual void print() const override;
};

extern std::shared_ptr<SearchEngine> parse_cmd_line(
    int argc, const char **argv, plugins::Registry &registry, bool dry_run,
    bool is_unit_cost);

extern std::string usage();
extern std::string g_program_name;

#endif
