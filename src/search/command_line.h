#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <memory>
#include <string>

template<typename T>
class TaskIndependentComponentType;
class SearchAlgorithm;

extern std::shared_ptr<TaskIndependentComponentType<SearchAlgorithm>> parse_cmd_line(
    int argc, const char **argv, bool is_unit_cost);

extern std::string get_revision_info();
extern std::string get_usage(const std::string &progname);

#endif
