#include "logging.h"

#include "system.h"
#include "timer.h"

#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

namespace utils {
class MemoryTracer {
    // The following constants affect the formatting of output.
    static const int INDENT_AMOUNT = 2;
    static const int MEM_FIELD_WIDTH = 7;
    static const int TIME_FIELD_WIDTH = 7;

    vector<string> block_stack;
public:
    MemoryTracer();
    ~MemoryTracer();

    void enter_block(const string &block_name);
    void leave_block(const string &block_name);
    void print_trace_message(const string &msg);
};


static MemoryTracer _tracer;


MemoryTracer::MemoryTracer() {
}


MemoryTracer::~MemoryTracer() {
    if (!block_stack.empty())
        ABORT("oops!");
}


void MemoryTracer::enter_block(const string &block_name) {
    _tracer.print_trace_message("enter " + block_name);
    block_stack.push_back(block_name);
}


void MemoryTracer::leave_block(const string &block_name) {
    if (block_stack.empty() || block_stack.back() != block_name)
        ABORT("oops!");
    block_stack.pop_back();
    _tracer.print_trace_message("leave " + block_name);
}


void MemoryTracer::print_trace_message(const string &msg) {
    cout << "[TRACE] "
         << setw(TIME_FIELD_WIDTH) << g_timer << " "
         << setw(MEM_FIELD_WIDTH) << get_peak_memory_in_kb() << " KB";
    for (size_t i = 0; i < block_stack.size() * INDENT_AMOUNT; ++i)
        cout << ' ';
    cout << ' ' << msg << endl;
}


TraceBlock::TraceBlock(const string &block_name)
    : block_name(block_name) {
    _tracer.enter_block(block_name);
}


TraceBlock::~TraceBlock() {
    _tracer.leave_block(block_name);
}


void trace(const string &msg) {
    _tracer.print_trace_message(msg);
}
}
