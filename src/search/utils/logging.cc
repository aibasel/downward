#include "logging.h"

#include "system.h"
#include "timer.h"

#include "../option_parser.h"

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

void add_log_options_to_parser(options::OptionParser &parser) {
    vector<string> verbosity_levels;
    vector<string> verbosity_level_docs;
    verbosity_levels.push_back("silent");
    verbosity_level_docs.push_back(
        "only the most basic output");
    verbosity_levels.push_back("normal");
    verbosity_level_docs.push_back(
        "relevant information to monitor progress");
    verbosity_levels.push_back("verbose");
    verbosity_level_docs.push_back(
        "full output");
    verbosity_levels.push_back("debug");
    verbosity_level_docs.push_back(
        "like verbose with additional debug output");
    parser.add_enum_option<Verbosity>(
        "verbosity",
        verbosity_levels,
        "Option to specify the verbosity level.",
        "normal",
        verbosity_level_docs);
}

LogProxy get_log_from_options(const options::Options &options) {
    /* NOTE: We return (a proxy to) the global log if all options match the
       default values of the global log. */
    if (options.get<Verbosity>("verbosity") == Verbosity::NORMAL) {
        return LogProxy(global_log);
    }
    return LogProxy(make_shared<Log>(options.get<Verbosity>("verbosity")));
}

LogProxy get_silent_log() {
    options::Options opts;
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
}
