#include "task_lexer.h"

#include "collections.h"
#include "system.h"

#include <cassert>
#include <regex>
#include <sstream>

using namespace std;
using utils::ExitCode;

namespace utils {
static const string end_of_line_sentinel("\n");
static const std::regex only_whitespaces("\\s*");

TaskLexer::TaskLexer(istream &stream)
    : stream(stream)  {
}

std::optional<std::string> TaskLexer::get_next_nonempty_line() {
    string next_line;
    while (!stream.eof()) {
        getline(stream, next_line);
        ++stream_line_number;
        if (!regex_match(next_line, only_whitespaces)) {
            return next_line;
        }
    }
    return nullopt;
}

void TaskLexer::initialize_tokens(const Context &context) {
    assert(tokens.empty());
    assert(token_number == 0);
    optional<string> line = get_next_nonempty_line();
    if (line.has_value()) {
        istringstream stream(line.value());
        string word;
        /* NOTE: The following ignores whitespace within and in
           particular at the end of the line. */
        while (stream >> word) {
            tokens.push_back(word);
        }
        assert(tokens.size() > 0);
        tokens.push_back(end_of_line_sentinel);
    } else {
        context.error("Unexpected end of task.");
    }
}

const string &TaskLexer::pop_token(){
    assert(is_in_line_reading_mode());
    assert(in_bounds(token_number, tokens));
    const string &token = tokens[token_number];
    ++token_number;
    return token;
}

bool TaskLexer::is_in_line_reading_mode() const {
    return !tokens.empty();
}

string TaskLexer::read(const Context &context) {
    if (tokens.empty()) {
        initialize_tokens(context);
    }
    const string &token = pop_token();
    if (token == end_of_line_sentinel) {
        context.error("Unexpected end of line.");
    }
    return token;
}

string TaskLexer::read_line(const Context &context) {
    if (is_in_line_reading_mode()) {
        ABORT("Tried to read a line before confirming the end of "
              "the previous line.");
    }
    optional<string> line = get_next_nonempty_line();
    if (!line.has_value()) {
        context.error("Unexpected end of task.");
    }
    return line.value();
}

void TaskLexer::confirm_end_of_line(const Context &context) {
    if (!is_in_line_reading_mode()) {
        ABORT("Tried to confirm end of line while not reading a line "
              "as tokens.");
    }
    const string &token = pop_token();
    if (token == end_of_line_sentinel) {
        token_number = 0;
        tokens.clear();
    } else {
        context.error("Expected end of line after token "
                      + to_string(token_number - 1) + " but line contains "
                      + to_string(tokens.size() - 1) + " tokens.");
    }
}

void TaskLexer::confirm_end_of_input(const Context &context) {
    if (is_in_line_reading_mode()) {
        ABORT("Tried to confirm end of input while reading a line as tokens.");
    }
    optional<string> line = get_next_nonempty_line();
    if (line.has_value()) {
        context.error("Expected end of task, found non-empty line " + line.value());
    }
}

int TaskLexer::get_line_number() const {
    if (is_in_line_reading_mode()) {
        /*
          When we read a line token by token, the input stream is
          already in the next line.
        */
        return stream_line_number - 1;
    } else {
        return stream_line_number;
    }
}
}
