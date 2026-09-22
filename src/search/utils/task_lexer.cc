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

static bool is_whitespace(const string &s) {
    for (char c : s) {
        if (!isspace(c))
            return false;
    }
    return true;
}

TaskParserError::TaskParserError(const string &msg) : Exception(msg) {
}

void TaskParserError::add_context(const string &line) {
    context.push_back(line);
}

void TaskParserError::print_with_context(ostream &out) const {
    // TODO: DRY: Based on code from Context::str().
    const string INDENT = "  ";
    out << "Context:" << endl;
    bool first_line = true;
    for (auto iter = context.rbegin(); iter != context.rend(); ++iter) {
        out << INDENT;
        if (first_line) {
            first_line = false;
        } else {
            out << "-> ";
        }
        out << *iter << endl;
    }
    out << get_message() << endl;
}

TaskLexer::TaskLexer(istream &stream) : stream(stream) {
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

void TaskLexer::initialize_tokens() {
    assert(tokens.empty());
    assert(token_number == 0);
    get_next_nonempty_line();
    if (!current_line.empty()) {
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
        error("Unexpected end of task.");
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

void TaskLexer::error(const string &message) const {
    throw TaskParserError(message);
}

string TaskLexer::read() {
    if (tokens.empty()) {
        initialize_tokens();
    }
    const string &token = pop_token();
    if (token == end_of_line_sentinel) {
        error("Unexpected end of line.");
    }
    return token;
}

string TaskLexer::read_line() {
    if (is_in_line_reading_mode()) {
        ABORT("Tried to read a line before confirming the end of "
              "the previous line.");
    }
    get_next_nonempty_line();
    if (current_line.empty()) {
        error("Unexpected end of task.");
    }
    return current_line;
}

void TaskLexer::confirm_end_of_line() {
    if (!is_in_line_reading_mode()) {
        ABORT("Tried to confirm end of line while not reading a line "
              "as tokens.");
    }
    const string &token = pop_token();
    if (token == end_of_line_sentinel) {
        token_number = 0;
        tokens.clear();
    } else {
        error(
            "Expected end of line after token " + to_string(token_number - 1) +
            " but line contains " + to_string(tokens.size() - 1) + " tokens.");
    }
}

void TaskLexer::confirm_end_of_input() {
    if (is_in_line_reading_mode()) {
        ABORT("Tried to confirm end of input while reading a line as tokens.");
    }
    get_next_nonempty_line();
    if (!current_line.empty()) {
        error("Expected end of task, found non-empty line " + current_line);
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
