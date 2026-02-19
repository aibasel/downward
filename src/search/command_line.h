#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <memory>
#include <string>

class AbstractTask;
class SearchAlgorithm;

extern std::shared_ptr<SearchAlgorithm> parse_cmd_line(
    int argc, const char **argv, bool is_unit_cost,
    const std::shared_ptr<AbstractTask> &task);

extern std::string get_revision_info();
extern std::string get_usage(const std::string &progname);

#endif
