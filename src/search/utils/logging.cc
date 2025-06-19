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

tuple<Verbosity> get_log_arguments_from_options(
    const plugins::Options &opts) {
    return make_tuple<Verbosity>(opts.get<Verbosity>("verbosity"));
}

LogProxy get_log_for_verbosity(const Verbosity &verbosity) {
    if (verbosity == Verbosity::NORMAL) {
        return LogProxy(global_log);
    }
    return LogProxy(make_shared<Log>(verbosity));
}

LogProxy get_silent_log() {
    return utils::get_log_for_verbosity(utils::Verbosity::SILENT);
}

ContextError::ContextError(const string &msg)
    : Exception(msg) {
}

const string Context::INDENT = "  ";

Context::Context()
    : initial_stack_size(0) {
}

Context::Context(const Context &context)
    : initial_stack_size(context.block_stack.size()),
      block_stack(context.block_stack) {
}

Context::~Context() {
    if (block_stack.size() > initial_stack_size) {
        cerr << str() << endl;
        ABORT("A context was destructed with an non-empty stack.");
    }
}

string Context::decorate_block_name(const string &block_name) const {
    return block_name;
}

void Context::enter_block(const string &block_name) {
    block_stack.push_back(block_name);
}

void Context::leave_block(const string &block_name) {
    if (block_stack.empty() || block_stack.back() != block_name) {
        cerr << str() << endl;
        ABORT("Tried to pop a block '" + block_name +
              "' from an empty stack or the block to remove "
              "is not on the top of the stack.");
    }
    block_stack.pop_back();
}

string Context::str() const {
    ostringstream message;
    message << "Traceback:" << endl;
    if (block_stack.empty()) {
        message << INDENT << "Empty";
    } else {
        message << INDENT
                << utils::join(block_stack, "\n" + INDENT + "-> ");
    }
    return message.str();
}

void Context::error(const string &message) const {
    throw ContextError(str() + "\n\n" + message);
}

void Context::warn(const string &message) const {
    utils::g_log << str() << endl << endl << message;
}

TraceBlock::TraceBlock(Context &context, const string &block_name)
    : context(context),
      block_name(context.decorate_block_name(block_name)) {
    context.enter_block(this->block_name);
}

TraceBlock::~TraceBlock() {
    context.leave_block(block_name);
}

MemoryContext _memory_context;

string MemoryContext::decorate_block_name(const string &msg) const {
    ostringstream decorated_msg;
    decorated_msg << "[TRACE] "
                  << setw(TIME_FIELD_WIDTH) << g_timer << " "
                  << setw(MEM_FIELD_WIDTH) << get_peak_memory_in_kb() << " KB";
    for (size_t i = 0; i < block_stack.size(); ++i)
        decorated_msg << INDENT;
    decorated_msg << ' ' << msg << endl;
    return decorated_msg.str();
}

void trace_memory(const string &msg) {
    g_log << _memory_context.decorate_block_name(msg);
}

static plugins::TypedEnumPlugin<Verbosity> _enum_plugin({
        {"silent", "only the most basic output"},
        {"normal", "relevant information to monitor progress"},
        {"verbose", "full output"},
        {"debug", "like verbose with additional debug output"}
    });

void Log::add_prefix() const {
    stream << "[t=";
    streamsize previous_precision = cout.precision(TIMER_PRECISION);
    ios_base::fmtflags previous_flags = stream.flags();
    stream.setf(ios_base::fixed, ios_base::floatfield);
    stream << g_timer;
    stream.flags(previous_flags);
    cout.precision(previous_precision);
    stream << ", "
           << get_peak_memory_in_kb() << " KB] ";
}
}
