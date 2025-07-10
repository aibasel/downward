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

TaskLexer::TaskLexer(istream &stream)
    : stream(stream) {
}

static bool is_whitespace(const string &s) {
    for (char c : s) {
        if (!isspace(c))
            return false;
    }
    return true;
}

void TaskLexer::get_next_nonempty_line() {
    while (true) {
        if (stream.eof()) {
            current_line = "";
            break;
        }
        getline(stream, current_line);
        ++stream_line_number;
        if (!is_whitespace(current_line)) {
            break;
        }
    }
}

void TaskLexer::initialize_tokens(const Context &context) {
    assert(tokens.empty());
    assert(token_number == 0);
    get_next_nonempty_line();
    if (current_line != "") {
        istringstream stream(current_line);
        string word;
        /* NOTE: The following ignores whitespace within and in
           particular at the end of the line. */
        while (stream >> word) {
            tokens.push_back(word);
        }
        assert(!tokens.empty());
        tokens.push_back(end_of_line_sentinel);
    } else {
        context.error_new("Unexpected end of task.");
    }
}

const string &TaskLexer::pop_token() {
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
        context.error_new("Unexpected end of line.");
    }
    return token;
}

string TaskLexer::read_line(const Context &context) {
    if (is_in_line_reading_mode()) {
        ABORT("Tried to read a line before confirming the end of "
              "the previous line.");
    }
    get_next_nonempty_line();
    if (current_line == "") {
        context.error_new("Unexpected end of task.");
    }
    return current_line;
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
        context.error_new("Expected end of line after token "
                      + to_string(token_number - 1) + " but line contains "
                      + to_string(tokens.size() - 1) + " tokens.");
    }
}

void TaskLexer::confirm_end_of_input(const Context &context) {
    if (is_in_line_reading_mode()) {
        ABORT("Tried to confirm end of input while reading a line as tokens.");
    }
    get_next_nonempty_line();
    if (current_line != "") {
        context.error_new("Expected end of task, found non-empty line " + current_line);
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
