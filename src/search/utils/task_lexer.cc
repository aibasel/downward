#include "task_lexer.h"

#include "system.h"

#include <cassert>
#include <sstream>

using namespace std;
using utils::ExitCode;

namespace utils {


TaskLexer::TaskLexer(istream &stream)
    : stream(stream), only_whitespaces("\\s*")  {
}

void TaskLexer::find_next_line(bool throw_error_on_failure) {
    assert(may_start_line()); // We probably forgot a confirm_end_of_line.
    string next_line;
    while (!stream.eof()) {
        getline(stream, next_line);
        ++line_number;
        if (!regex_match(next_line, only_whitespaces)) {
            line = next_line;
            return;
        }
    }
    if (throw_error_on_failure) {
        error("Unexpected end of task.");
    }
    line = "";
}

void TaskLexer::initialize_tokens() {
    assert(may_start_line());
    assert(token_number == 0);
    assert(line != "");
    istringstream stream(line);
    string word;
    while (!stream.eof()) {
        stream >> word;
        tokens.push_back(word);
    }
    assert(tokens.size() > 0);
}

bool TaskLexer::may_start_line() {
    return tokens.empty();
}

TraceBlock TaskLexer::trace_block(const string &block_name) {
    return TraceBlock(context, block_name);
}

string TaskLexer::read(const string &message) {
    if (may_start_line()) {
        find_next_line(true);
        initialize_tokens();
    }
    if (token_number >= tokens.size()) {
        error("Unexpected end of line. Message: " + message);
    }
    string token = tokens[token_number];
    ++token_number;
    return token;
}

string TaskLexer::read_line(const string &/*message*/) {
    find_next_line(true);
    return line;
}

void TaskLexer::confirm_end_of_line() {
    if (may_start_line()) {
        return;
    }
    if (token_number == tokens.size()) {
        token_number = 0;
        tokens.clear();
    } else {
        error("Expected end of line after token " + to_string(token_number)
              + " but line contains " + to_string(tokens.size()) + " tokens.");
    }
}

void TaskLexer::confirm_end_of_input() {
    find_next_line(false);
    if(line != "") {
        error("Expected end of task, found non-empty line " + line);
    }
}

void TaskLexer::error(const string &message) const {
    cerr << "Error reading task ";
    if (line_number > 0) {
        cerr <<  "line " << line_number;
    }
    cerr << "." << endl;
    cerr << context.str() << endl << endl
         << message << endl << "Exiting." << endl;
    utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
}
}
