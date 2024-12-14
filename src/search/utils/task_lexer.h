#ifndef UTILS_TASK_LEXER_H
#define UTILS_TASK_LEXER_H

#include "language.h"
#include "logging.h"

#include <istream>
#include <regex>
#include <string>
#include <vector>

namespace utils {

/*
  Split a task encoding in the translator output format into tokens.
  Read a complete line as a single token *or* split a line into
  whitespace-separated tokens and read them one by one. The latter
  requires to confirm the end of the line manually. A line or a token
  can be parsed as an integer if it is a string of digits. A line can
  also be read and immediately compared to a given *magic* string.
*/
class TaskLexer {
    std::istream &stream;
    int line_number = 0;
    size_t token_number = 0;
    std::string line;
    std::vector<std::string> tokens;
    const std::regex only_whitespaces;
    void find_next_line(bool throw_error_on_failure=true);
    void initialize_tokens();
    bool may_start_line();
    int parse_int(const std::string &str, const std::string &cause);
    Context context;
public:
    explicit TaskLexer(std::istream &stream);

    /*
      Set context for error reporting.
    */
    TraceBlock trace_block(const std::string &block_name);
    /*
      Read a single token within a line. Tokens within a line are
      separated by arbitrary whitespaces. Report error if the current
      line does not contain a token after the cursor position. Set
      cursor to the end of the read token.
    */
    std::string read(const std::string &message);
    /*
      Read a complete line as a single string token. Report an error if
      the cursor is not at the beginning of a line before reading. Set
      cursor to the beginning of the next line.
    */
    std::string read_line(const std::string &message);
    /*
      Read a complete line and compare it to a *magic* string. Report an
      error if the cursor is not at the beginning of a line before
      reading. Report an error if the content of the line is not equal
      to the *magic* string.
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
    void confirm_end_of_input();
    // TODO: Should this be public at all? Or should we add a get_line method?
    NO_RETURN void error(const std::string &message) const;
};
}
#endif
