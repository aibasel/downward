#ifndef UTILITIES_H
#define UTILITIES_H

#include <vector>
#include <ostream>

extern void register_event_handlers();

extern int get_peak_memory_in_kb();
extern void print_peak_memory();

namespace std {
    template<class T>
    ostream &operator<<(ostream &stream, const vector<T> &vec) {
        stream << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i != 0)
                stream << ", ";
            stream << vec[i];
        }
        stream << "]";
        return stream;
    }
}

#endif
