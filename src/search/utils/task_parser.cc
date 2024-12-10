#include "task_parser.h"

#include "system.h"

#include <cassert>
#include <sstream>

using namespace std;
using utils::ExitCode;

namespace utils {


TaskParser::TaskParser(istream &stream)
: stream(stream), context(""), only_whitespaces("\\s*")  {
}

void TaskParser::find_next_line(bool throw_error_on_failure) {
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

void TaskParser::initialize_tokens() {
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

bool TaskParser::may_start_line() {
    return tokens.empty();
}

int TaskParser::parse_int(const string &str, const string &cause) {
    try {
        string::size_type parsed_length;
        int number = stoi(str, &parsed_length);
        if (parsed_length == str.size()) {
            return number;
        }
    } catch (exception &e) {
    }
    error("expected number; cause: " + cause);
}

void TaskParser::set_context(const string &context) {
    this->context = context;
}

string TaskParser::read(const string &message) {
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

int TaskParser::read_int(const string &message) {
    string token = read(message);
    return parse_int(token, message);
}

string TaskParser::read_line(const string &/*message*/) {
    find_next_line(true);
    return line;
}

int TaskParser::read_line_int(const string &message) {
    string line = read_line(message);
    return parse_int(line, message);
}

void TaskParser::read_magic_line(const string &magic) {
    string line = read_line("read magic line");
    if (line != magic) {
        error("Expected magic line " + magic + ", got " + line + ".");
    }
}

void TaskParser::confirm_end_of_line() {
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

void TaskParser::confirm_end_of_input() {
    find_next_line(false);
    if(line != "") {
        error("Expected end of task, found non-empty line " + line);
    }
}

void TaskParser::error(const string &message) const {
    cerr << "Error reading task ";
    if (line_number > 0) {
        cerr <<  "line " << line_number;
    }
    cerr << "." << endl; 
    if (context != "") {
        cerr << "Context: " << context << endl;
    }
    cerr << message << endl << "Exiting." << endl;
    utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
}
}
