#include "system.h"

#include <sstream>
#include <istream>
#include <vector>
#include "input_file_parser.h"

using namespace std;
using utils::ExitCode;

namespace utils {
static vector<string> split_line(const string &line) {
    istringstream stream(line);
    vector<string> words;
    while (!stream.eof()) {
        string word;
        stream >> word;
        words.push_back(word);
    }
    return words;
}

InputFileParser::InputFileParser(istream &stream) : stream(stream) {
    context = "";
    line_number = 0;
}

InputFileParser::~InputFileParser() {
}

void InputFileParser::set_context(const string &context) {
    this->context = context;
}

InputFileLine InputFileParser::read_line() {
    if (stream.eof()) {
        error("Unexpected end of file.");
    }
    string line;
    getline(stream, line);
    ++line_number;
    return InputFileLine(make_shared<InputFileParser>(*this), line, line_number);
}

InputFileLineParser InputFileParser::parse_line() {
    shared_ptr<InputFileLine> line = make_shared<InputFileLine>(read_line());
    return InputFileLineParser(make_shared<InputFileParser>(*this), line);
}

InputFileToken InputFileParser::parse_single_token_line() {
    InputFileLineParser line_parser = parse_line();
    InputFileToken token = line_parser.read_token();
    line_parser.check_last_token();
    return token;
}

void InputFileParser::read_magic_line(const string &magic) { // TODO: name?
    InputFileLine line = read_line();
    if (line.get_line() != magic) {
        error("Expected magic line " + magic + ", got " + line.get_line() + ".");
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
    cerr << message << endl
         << "Exiting." << endl;
    utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
}
    
InputFileLine::InputFileLine(const shared_ptr<InputFileParser> file_parser, const string &line, int line_number)
: file_parser(file_parser), line(line) {
    this->line_number = line_number;
}

InputFileLine::~InputFileLine() {
}

const string& InputFileLine::get_line() const {
    return line;
}

InputFileLineParser::InputFileLineParser(const shared_ptr<InputFileParser>file_parser, const shared_ptr<InputFileLine> line)
: file_parser(file_parser), line(line) {
    tokens = split_line(line->get_line());
    token_number = 0;
}

InputFileLineParser::~InputFileLineParser() {
}

const vector<string>& InputFileLineParser::get_tokens() const {
    return tokens;
}
    
InputFileToken InputFileLineParser::read_token() {
    if (token_number > tokens.size()) {
        file_parser->error("unexpected end of line");
    }
    ++token_number;
    return InputFileToken(file_parser, line, tokens[token_number - 1], token_number);
}

void InputFileLineParser::check_last_token() {
    if (token_number > tokens.size()) {
        file_parser->error("expected end of line");
    }
}

InputFileToken::InputFileToken(const shared_ptr<InputFileParser> file_parser, const shared_ptr<InputFileLine> line, const string &token, int token_number)
    : file_parser(file_parser), line(line), token(token) {
    this->token_number = token_number;
}

InputFileToken::~InputFileToken() {
}

const string& InputFileToken::get_token() const {
    return token;
}

int InputFileToken::parse_int(const string &cause) const {
    try {
        string::size_type parsed_length;
        int number = stoi(token, &parsed_length);
        if (parsed_length == token.size()) {
            return number;
        }
    } catch (exception &e) {
    }
    file_parser->error("expected number; cause: " + cause);
}
}
