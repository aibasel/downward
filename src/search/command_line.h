#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <memory>
#include <string>

class SearchAlgorithm;

extern std::shared_ptr<SearchAlgorithm> parse_cmd_line(
    int argc, const char **argv, bool is_unit_cost);
extern void handle_tab_completion(int argc, const char **argv);

extern std::string usage(const std::string &progname);

#endif
