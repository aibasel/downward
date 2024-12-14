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
    // Note that line numbers start at 1 not 0 as we do not use them as indices.
    int line_number = 1;
    size_t token_number = 0;
    std::string line;
    std::vector<std::string> tokens;
    const std::regex only_whitespaces;
    void find_next_line(const Context &context, bool throw_error_on_failure=true);
    void initialize_tokens();
    bool may_start_line();
public:
    explicit TaskLexer(std::istream &stream);

    /*
      Read a single token within a line. Tokens within a line are
      separated by arbitrary whitespaces. Report error if the current
      line does not contain a token after the cursor position. Set
      cursor to the end of the read token.
    */
    std::string read(const Context &context);
    /*
      Read a complete line as a single string token. Report an error if
      the cursor is not at the beginning of a line before reading. Set
      cursor to the beginning of the next line.
    */
    std::string read_line(const Context &context);
    /*
      Check that the end of the line has been reached and set cursor to
      the beginning of the next line. Report error otherwise.
    */
    void confirm_end_of_line(const Context &context);
    /*
      Check that the end of the file has been reached. Report error otherwise.
    */
    void confirm_end_of_input(const Context &context);

    /*
      Return the current line number.
    */
    int get_line_number() const;
};
}
#endif
