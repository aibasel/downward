#ifndef UTILS_INPUT_FILE_PARSER_H
#define UTILS_INPUT_FILE_PARSER_H

#include <memory>
#include <regex>

namespace utils {
class InputFileLine;
class InputFileLineParser;
class InputFileToken;

class InputFileParser {
    std::istream &stream;
    std::string context;
    int line_number = 0;
    size_t token_number;
    std::string line;
    std::vector<std::string> tokens;
    bool may_start_line = true;
    const std::regex only_whitespaces;
public:
    explicit InputFileParser(std::istream &stream);
    ~InputFileParser();

    void set_context(const std::string &context);
    std::string read(const std::string &message); // TODO: templates
    int read_int(const std::string &message); // TODO: templates
    std::string read_line(const std::string &message); // TODO: templates
    int read_line_int(const std::string &message); // TODO: templates
    void read_magic_line(const std::string &magic);
    void confirm_end_of_line();
    void confirm_end_of_file();
    void error(const std::string &message) const;
private:
    std::string find_next_line(bool throw_error_on_failure=true);
    void initialize_tokens();
    int parse_int(const std::string &str, const std::string &cause);
};
}
#endif
