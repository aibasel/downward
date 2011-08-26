#ifndef UTILITIES_H
#define UTILITIES_H

extern void register_event_handlers();

extern int get_peak_memory_in_kb();
extern void print_peak_memory();

template<class Sequence>
size_t hash_number_sequence(Sequence data, size_t length) {
    // hash function adapted from Python's hash function for tuples.
    size_t hash_value = 0x345678;
    size_t mult = 1000003;
    for (int i = length - 1; i >= 0; --i) {
        hash_value = (hash_value ^ data[i]) * mult;
        mult += 82520 + i + i;
    }
    hash_value += 97531;
    return hash_value;
}

#endif
