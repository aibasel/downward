#ifndef LOGGING_H
#define LOGGING_H

#include "globals.h"
#include "system.h"
#include "timer.h"

#include <ostream>

struct Log {
    template<typename T>
    std::ostream &operator<<(const T &elem) {
        return std::cout << "[t=" << g_timer << ", "
                         << get_peak_memory_in_kb() << " KB] " << elem;
    }
};

#endif
