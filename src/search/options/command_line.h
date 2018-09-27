#ifndef OPTIONS_COMMAND_LINE_H
#define OPTIONS_COMMAND_LINE_H

#include "registries.h"

#include <memory>
#include <string>

class SearchEngine;

namespace options {
std::shared_ptr<SearchEngine> parse_cmd_line(
    int argc, const char **argv, Registry &registry, bool dry_run, bool is_unit_cost);

std::string usage(const std::string &progname);
}
#endif
