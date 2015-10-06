#include "utilities_memory.h"

#include <cassert>
#include <iostream>

using namespace std;


static char *continuing_memory_padding = nullptr;

// Save previous out-of-memory handler.
static void (*previous_out_of_memory_handler)(void) = nullptr;

void continuing_out_of_memory_handler() {
    release_continuing_memory_padding();
    cout << "Failed to allocate memory. Released continuing memory "
            "padding." << endl;
}

void reserve_continuing_memory_padding(int memory_in_mb) {
    assert(!continuing_memory_padding);
    continuing_memory_padding = new char[memory_in_mb * 1024 * 1024];
    previous_out_of_memory_handler = set_new_handler(continuing_out_of_memory_handler);
}

void release_continuing_memory_padding() {
    assert(continuing_memory_padding);
    delete[] continuing_memory_padding;
    continuing_memory_padding = nullptr;
    assert(previous_out_of_memory_handler);
    set_new_handler(previous_out_of_memory_handler);
}

bool continuing_memory_padding_is_reserved() {
    return continuing_memory_padding;
}
