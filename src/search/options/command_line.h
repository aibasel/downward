#ifndef OPTIONS_COMMAND_LINE_H
#define OPTIONS_COMMAND_LINE_H

#include "../search_engine.h"

#include <memory>
#include <string>
#include <vector>

class SearchEngine;
namespace landmarks {
    class LandmarkFactory;
}

namespace options {

std::shared_ptr<SearchEngine> parse_cmd_line_aux(
    const std::vector<std::string> &args, bool dry_run);

std::shared_ptr<SearchEngine> parse_cmd_line(
    int argc, const char **argv, bool dry_run, bool is_unit_cost);

std::string usage(const std::string &progname);
}
#endif

