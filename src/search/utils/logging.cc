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

class MemoryContext : public Context {
    // The following constants affect the formatting of output.
    static const int MEM_FIELD_WIDTH = 7;
    static const int TIME_FIELD_WIDTH = 7;
public:
    // TODO: necessary?
//    MemoryContext();
//    ~MemoryContext();

    virtual string decorate_block_name(const string &block_name) const override;
};

static MemoryContext _tracer;

ContextError::ContextError(const std::string &msg)
    : Exception(msg) {
}

Context::Context(const Context &context)
    : initial_stack_size(context.block_stack.size()),
      block_stack(context.block_stack) {
}

Context::~Context() {
    if (block_stack.size() > initial_stack_size) {
        cerr << str() << endl;
        ABORT("The context was destructed with an non-empty stack.");
    }
}

string Context::decorate_block_name(const string &block_name) const {
    return block_name;
}

void Context::enter_block(const string &block_name) {
    block_stack.push_back(block_name);
}

void Context::leave_block(const string &block_name) {
    if (block_stack.empty() || block_stack.back() != block_name)
        ABORT("oops!");
    block_stack.pop_back();
}

string Context::str() const {
    ostringstream message;
    ostringstream indent;
    for (size_t i = 0; i < block_stack.size() * INDENT_AMOUNT; ++i)
        indent << ' ';
    message << "Traceback:" << endl;
    if (block_stack.empty()) {
        message << indent.str() << "Empty";
    } else {
        message << indent.str()
                << utils::join(block_stack,"\n" + indent.str() + "-> ");
    }
    return message.str();
}

void Context::error(const string &message) const {
    throw ContextError(str() + "\n\n" + message);
}

void Context::warn(const std::string &message) const {
    utils::g_log << str() << endl << endl << message;
}

string MemoryContext::decorate_block_name(const string &msg) const {
    ostringstream decorated_msg;
    decorated_msg << "[TRACE] "
                  << setw(TIME_FIELD_WIDTH) << g_timer << " "
                  << setw(MEM_FIELD_WIDTH) << get_peak_memory_in_kb() << " KB";
    for (size_t i = 0; i < block_stack.size() * INDENT_AMOUNT; ++i)
        decorated_msg << ' ';
    decorated_msg << ' ' << msg << endl;
    return decorated_msg.str();
}


TraceBlock::TraceBlock(Context &context, const string &block_name)
    : context(context),
      block_name(context.decorate_block_name(block_name)) {
    context.enter_block(this->block_name);
}


TraceBlock::~TraceBlock() {
    context.leave_block(block_name);
}


void trace(const string &msg) {
    g_log << _tracer.decorate_block_name(msg);
}

static plugins::TypedEnumPlugin<Verbosity> _enum_plugin({
        {"silent", "only the most basic output"},
        {"normal", "relevant information to monitor progress"},
        {"verbose", "full output"},
        {"debug", "like verbose with additional debug output"}
    });
}
