#ifndef UTILS_TASK_LEXER_H
#define UTILS_TASK_LEXER_H

#include "logging.h"

#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace utils {
/*
  Split a task encoding in the translator output format into tokens.
  Read a complete line as a single token *or* split a line into
  whitespace-separated tokens and read them one by one. The latter
  requires to confirm the end of the line manually before reading
  the next line.
*/
class TaskLexer {
    std::istream &stream;
    /*
      Line number in the input stream. When we read a line as tokens,
      the input stream already is at the start of the next line, so the
      line number of the cursor can be different.
      Note that line numbers start at 1 not 0 as we do not use them
      as indices.
    */
    int stream_line_number = 1;
    size_t token_number = 0;
    std::vector<std::string> tokens;
    std::optional<std::string> get_next_nonempty_line();
    void initialize_tokens(const Context &context);
    const std::string &pop_token();
    bool is_in_line_reading_mode() const;
public:
    explicit TaskLexer(std::istream &stream);

    /*
      Read a single token within a line. Tokens within a line are
      separated by arbitrary whitespaces. Report error if the current
      line does not contain a token after the cursor position. Set
      cursor to the end of the read token. Afterwards the lexer is in
      line reading mode, in which only read() and confirm_end_of_line()
      are allowed.
    */
    std::string read(const Context &context);
    /*
      Read a complete line as a single string token. Report an error if
      the cursor is not at the beginning of a line before reading. Set
      cursor to the beginning of the next line. Not allowed in line
      reading mode.
    */
    std::string read_line(const Context &context);
    /*
      Check that the end of the line has been reached and set cursor to
      the beginning of the next line. Report error otherwise.
      This method is only allowed in line reading mode and will end the mode.
    */
    void confirm_end_of_line(const Context &context);
    /*
      Check that the end of the file has been reached. Report error otherwise.
      Not allowed in line reading mode.
    */
    void confirm_end_of_input(const Context &context);

    /*
      Return the line number of the cursor.
    */
    int get_line_number() const;
};
}
#endif
