#ifndef LOGGING_H
#define LOGGING_H

#include "globals.h"
#include "system.h"
#include "timer.h"

#include <ostream>

/*
  Simple logger that prepends time and peak memory info to messages.

  Usage:
        Log() << "States: " << num_states << endl;
*/
struct Log {
    template<typename T>
    std::ostream &operator<<(const T &elem) {
        return std::cout << "[t=" << g_timer << ", "
                         << get_peak_memory_in_kb() << " KB] " << elem;
    }
};

#endif
