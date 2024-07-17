#ifndef UTILS_INPUT_FILE_PARSER_H
#define UTILS_INPUT_FILE_PARSER_H

#include <memory>

namespace utils {
class InputFileLine;
class InputFileLineParser;
class InputFileToken;

class InputFileParser {
    std::istream &stream;
    std::string context;
    int line_number;
public:
    explicit InputFileParser(std::istream &stream);
    ~InputFileParser();
    void set_context(const std::string &context);
    InputFileLine read_line();
    InputFileLineParser parse_line();
    InputFileToken parse_single_token_line();
    void read_magic_line(const std::string &magic);
    void error(const std::string &message) const;
};

class InputFileLine {
    const std::shared_ptr<InputFileParser> file_parser;
    const std::string line;
    int line_number;
public:
    InputFileLine(const std::shared_ptr<InputFileParser> file_parser, const std::string &line, int line_number);
    ~InputFileLine();
    const std::string& get_line() const;
};

class InputFileLineParser {
    const std::shared_ptr<InputFileParser> file_parser;
    const std::shared_ptr<InputFileLine> line;
    std::vector<std::string> tokens;
    int token_number;
public:
    InputFileLineParser(const std::shared_ptr<InputFileParser> file_parser, const std::shared_ptr<InputFileLine> line);
    ~InputFileLineParser();
    const std::vector<std::string>& get_tokens() const;
    InputFileToken read_token();
    void check_last_token();
};

class InputFileToken {
    const std::shared_ptr<InputFileParser> file_parser;
    const std::shared_ptr<InputFileLine> line;
    const std::string token;
    int token_number;
public:
    InputFileToken(const std::shared_ptr<InputFileParser> file_parser, const std::shared_ptr<InputFileLine> line, const std::string &token, int token_number);
    ~InputFileToken();
    const std::string& get_token() const;
    int parse_int(const std::string &cause) const;
};
}
#endif
