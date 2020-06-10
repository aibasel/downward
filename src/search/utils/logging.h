#ifndef UTILS_LOGGING_H
#define UTILS_LOGGING_H

#include "system.h"
#include "timer.h"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
// See add_log_options_to_parser for documentation.
enum class Verbosity {
    SILENT,
    NORMAL,
    VERBOSE,
    DEBUG
};

class Log {
    std::ostream &stream;
    const Verbosity verbosity;

public:
    explicit Log(Verbosity verbosity)
        : stream(std::cout), verbosity(verbosity) {
    }

    std::ostream &get_stream() {
        return stream;
    }

    Verbosity get_verbosity() const {
        return verbosity;
    }
};

/*
  Simple logger that prepends time and peak memory info to messages.
  Logs are written to stdout.

  Usage:
        utils::g_log << "States: " << num_states << endl;
*/
class LogProxy {
private:
    std::shared_ptr<Log> log;
    bool line_has_started;

public:
    explicit LogProxy(const std::shared_ptr<Log> &log)
        : log(log), line_has_started(false) {
    }

    template<typename T>
    LogProxy &operator<<(const T &elem) {
        if (!line_has_started) {
            line_has_started = true;
            log->get_stream() << "[t=" << g_timer << ", "
                              << get_peak_memory_in_kb() << " KB] ";
        }

        log->get_stream() << elem;
        return *this;
    }

    using manip_function = std::ostream &(*)(std::ostream &);
    LogProxy &operator<<(manip_function f) {
        if (f == static_cast<manip_function>(&std::endl)) {
            line_has_started = false;
        }

        log->get_stream() << f;
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
};

extern LogProxy g_log;

extern void add_log_options_to_parser(options::OptionParser &parser);

extern LogProxy get_log_from_options(const options::Options &options);

extern LogProxy make_silent_log();

class TraceBlock {
    std::string block_name;
public:
    explicit TraceBlock(const std::string &block_name);
    ~TraceBlock();
};

extern void trace(const std::string &msg = "");
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
