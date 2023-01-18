#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <memory>
#include <string>

class SearchEngine;

extern std::shared_ptr<SearchEngine> parse_cmd_line(
    int argc, const char **argv, bool is_unit_cost);

extern std::string usage();
extern std::string g_program_name;

#endif
