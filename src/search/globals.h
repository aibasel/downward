#ifndef GLOBALS_H
#define GLOBALS_H

#include <istream>

namespace utils {
struct Log;
}

void read_everything(std::istream &in);

// The following function is deprecated. Use task_properties.h instead.
bool is_unit_cost();

extern utils::Log g_log;

#endif
