#include "input_file_parser.h"

#include "system.h"

#include <cassert>
#include <istream>
#include <sstream>
#include <vector>

using namespace std;
using utils::ExitCode;

namespace utils {


InputFileParser::InputFileParser(istream &stream)
: stream(stream), context(""), only_whitespaces("\\s*")  {
}

string InputFileParser::find_next_line(bool throw_error_on_failure) {
    assert(may_start_line); // We probably forgot a confirm_end_of_line.
    string next_line;
    while (!stream.eof()) {
        getline(stream, next_line);
        ++line_number;
        if (!regex_match(next_line, only_whitespaces)) {
            return next_line;
        }
    }
    if (throw_error_on_failure) {
        error("Unexpected end of file.");
    }
    return "";
}

void InputFileParser::initialize_tokens() {
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

bool InputFileParser::may_start_line() {
    return tokens.empty();
}

int InputFileParser::parse_int(const string &str, const string &cause) {
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

void InputFileParser::set_context(const string &context) {
    this->context = context;
}

string InputFileParser::read(const string &message) {
    if (may_start_line()) {
        line = find_next_line(true);
        initialize_tokens();
    }
    if (token_number >= tokens.size()) {
        error("Unexpected end of line. Message: " + message);
    }
    string token = tokens[token_number];
    ++token_number;
    return token;
}

int InputFileParser::read_int(const string &message) {
    string token = read(message);
    return parse_int(token, message);
}

string InputFileParser::read_line(const string &message) {
    line = find_next_line(true);
    return line;
}

int InputFileParser::read_line_int(const string &message) {
    string line = read_line(message);
    return parse_int(line, message);
}

void InputFileParser::read_magic_line(const string &magic) {
    string line = read_line("read magic line");
    if (line != magic) {
        error("Expected magic line " + magic + ", got " + line + ".");
    }
}

void InputFileParser::confirm_end_of_line() {
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

void InputFileParser::confirm_end_of_file() {
    string next_line = find_next_line(false);
    if(next_line != "") {
        error("Expected end of file, found non-empty line " + next_line);
    }
}

void InputFileParser::error(const string &message) const {
    cerr << "Error reading input file ";
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
