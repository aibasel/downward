#ifndef GLOBALS_H
#define GLOBALS_H

#include <istream>

namespace utils {
struct Log;
}

void read_everything(std::istream &in);
// TODO: move this to task_utils or a new file with dump methods for all proxy objects.
void dump_everything();

// The following function is deprecated. Use task_properties.h instead.
bool is_unit_cost();

extern utils::Log g_log;

#endif
