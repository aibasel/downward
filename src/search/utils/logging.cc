#include "logging.h"

#include "system.h"
#include "timer.h"

#include "../plugins/plugin.h"

#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

namespace utils {
/*
  NOTE: When adding more options to Log, make sure to adapt the if block in
  get_log_from_options below to test for *all* default values used for
  global_log here. Also add the options to dump_options().
*/

static shared_ptr<Log> global_log = make_shared<Log>(Verbosity::NORMAL);

LogProxy g_log(global_log);

void add_log_options_to_feature(plugins::Feature &feature) {
    feature.add_option<Verbosity>(
        "verbosity",
        "Option to specify the verbosity level.",
        "normal");
}

LogProxy get_log_from_options(const plugins::Options &options) {
    /* NOTE: We return (a proxy to) the global log if all options match the
       default values of the global log. */
    if (options.get<Verbosity>("verbosity") == Verbosity::NORMAL) {
        return LogProxy(global_log);
    }
    return LogProxy(make_shared<Log>(options.get<Verbosity>("verbosity")));
}

LogProxy get_silent_log() {
    plugins::Options opts;
    opts.set<utils::Verbosity>("verbosity", utils::Verbosity::SILENT);
    return utils::get_log_from_options(opts);
}

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
    g_log << "[TRACE] "
          << setw(TIME_FIELD_WIDTH) << g_timer << " "
          << setw(MEM_FIELD_WIDTH) << get_peak_memory_in_kb() << " KB";
    for (size_t i = 0; i < block_stack.size() * INDENT_AMOUNT; ++i)
        g_log << ' ';
    g_log << ' ' << msg << endl;
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

static plugins::TypedEnumPlugin<Verbosity> _enum_plugin({
        {"silent", "only the most basic output"},
        {"normal", "relevant information to monitor progress"},
        {"verbose", "full output"},
        {"debug", "like verbose with additional debug output"}
    });
}
