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

    /*
      Set context for error reporting.
    */
    void set_context(const std::string &context);
    /*
      Read a single token within a line. Tokens within a line are
      separated by arbitrary whitespaces. Set curser to the end of the
      read token.
    */
    std::string read(const std::string &message); // TODO: templates
    /*
      Read a single token in a line as integer, analoguously to
      read(...). Report an error if the token is not a string of digits
      representing an int.
    */
    int read_int(const std::string &message); // TODO: templates
    /*
      Read a complete line as a single string. Set cursor to the
      beginning of the next line.
    */
    std::string read_line(const std::string &message); // TODO: templates
    /*
      Read a complete line as a single integer. Report an error if the
      line is not a string of digits representing an int.
    */
    int read_line_int(const std::string &message); // TODO: templates
    /*
      Read a complete line and compare it to a *magic* string.
    */
    void read_magic_line(const std::string &magic);
    /*
      Check that the end of the line has been reached and set cursor to
      the beginning of the next line. Report error otherwise.
    */
    void confirm_end_of_line();
    /*
      Check that the end of the file has been reached. Report error otherwise.
    */
    void confirm_end_of_file();
    // TODO: Should this be public at all? Or should we add a get_line method?
    void error(const std::string &message) const;
private:
    std::string find_next_line(bool throw_error_on_failure=true);
    void initialize_tokens();
    int parse_int(const std::string &str, const std::string &cause);
};
}
#endif
