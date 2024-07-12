#ifndef UTILS_LOGGING_H
#define UTILS_LOGGING_H

#include "exceptions.h"
#include "system.h"
#include "timer.h"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace plugins {
class Options;
class Feature;
}

namespace utils {
// See add_log_options_to_feature for documentation.
enum class Verbosity {
    SILENT,
    NORMAL,
    VERBOSE,
    DEBUG
};

/*
  Simple line-based logger that prepends time and peak memory info to each line
  of output. Lines should be eventually terminated by endl. Logs are written to
  stdout.

  Internal class encapsulated by LogProxy.
*/
class Log {
    static const int TIMER_PRECISION = 6;
    std::ostream &stream;
    const Verbosity verbosity;
    bool line_has_started;
    void add_prefix() const;

public:
    explicit Log(Verbosity verbosity)
        : stream(std::cout), verbosity(verbosity), line_has_started(false) {
    }

    template<typename T>
    Log &operator<<(const T &elem) {
        if (!line_has_started) {
            line_has_started = true;
            add_prefix();
        }
        stream << elem;
        return *this;
    }

    using manip_function = std::ostream &(*)(std::ostream &);
    Log &operator<<(manip_function f) {
        if (f == static_cast<manip_function>(&std::endl)) {
            line_has_started = false;
        }

        stream << f;
        return *this;
    }

    Verbosity get_verbosity() const {
        return verbosity;
    }
};

/*
  This class wraps Log which holds onto the used stream (currently hard-coded
  to be cout) and any further options for modifying output (currently only
  the verbosity level).

  The wrapped class Log is a line-based logger that prepends time and peak
  memory info to each line of output. Lines should be eventually terminated by
  endl. Logs are written to stdout.

  We need to have this wrapper because we want to be able to return plain
  objects (as opposed to pointers) in function get_log_from_options below,
  which internally returns a wrapper to the default Log if default options
  are specified or a new instance if non-default options are specified.
*/
class LogProxy {
private:
    std::shared_ptr<Log> log;

public:
    explicit LogProxy(const std::shared_ptr<Log> &log)
        : log(log) {
    }

    template<typename T>
    LogProxy &operator<<(const T &elem) {
        (*log) << elem;
        return *this;
    }

    using manip_function = std::ostream &(*)(std::ostream &);
    LogProxy &operator<<(manip_function f) {
        (*log) << f;
        return *this;
    }

    bool is_at_least_normal() const {
        return log->get_verbosity() >= Verbosity::NORMAL;
    }

    bool is_at_least_verbose() const {
        return log->get_verbosity() >= Verbosity::VERBOSE;
    }

    bool is_at_least_debug() const {
        return log->get_verbosity() >= Verbosity::DEBUG;
    }

    // TODO: implement an option for logging warnings.
    bool is_warning() const {
        return true;
    }
};

/*
  In the long term, this should not be global anymore. Instead, local LogProxy
  objects should be used everywhere. For classes constructed from the command
  line, they are parsed from Options. For other classes and functions, they
  must be passed in by the caller.
*/
extern LogProxy g_log;

extern void add_log_options_to_feature(plugins::Feature &feature);
extern std::tuple<Verbosity> get_log_arguments_from_options(
    const plugins::Options &opts);

extern LogProxy get_log_for_verbosity(const Verbosity &verbosity);
extern LogProxy get_silent_log();

class ContextError : public utils::Exception {
public:
    explicit ContextError(const std::string &msg);
};

class Context {
protected:
    static const std::string INDENT;
    size_t initial_stack_size = 0;  // TODO: Can be removed once we got rid of LazyValues
    std::vector<std::string> block_stack;

public:
    explicit Context() = default;
    Context(const Context &context);
    virtual ~Context();
    virtual std::string decorate_block_name(const std::string &block_name) const;
    void enter_block(const std::string &block_name);
    void leave_block(const std::string &block_name);
    std::string str() const;

    NO_RETURN
    virtual void error(const std::string &message) const;
    virtual void warn(const std::string &message) const;
};

class MemoryContext : public Context {
    // The following constants affect the formatting of output.
    static const int MEM_FIELD_WIDTH = 7;
    static const int TIME_FIELD_WIDTH = 7;
public:
    virtual std::string decorate_block_name(const std::string &block_name) const override;
};

extern MemoryContext _memory_context;

class TraceBlock {
    Context &context;
    std::string block_name;
public:
    explicit TraceBlock(Context &context, const std::string &block_name);
    ~TraceBlock();
};

extern void trace_memory(const std::string &msg = "");
}

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
