#ifndef UTILS_TASK_LEXER_H
#define UTILS_TASK_LEXER_H

#include "logging.h"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace utils {
/*
  TODO: The following class is used directly by both the task lexer,
  which lives in utils, and the task parser, which lives in tasks.
  This is probably a sign that they should live together.

  We could also have two separate error classes, but then the handling
  code gets awkward unless they have a common base class that
  implements the functionality we need. And that's probably overkill
  because we don't really need to distinguish lexer errors from parser
  errors.
*/
class TaskParserError : public utils::Exception {
    std::vector<std::string> context;
public:
    explicit TaskParserError(const std::string &msg);
    void add_context(const std::string &line);
    void print_with_context(std::ostream &out) const;
};

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
    std::string current_line;
    size_t token_number = 0;
    std::vector<std::string> tokens;
    void get_next_nonempty_line();
    void initialize_tokens();
    const std::string &pop_token();
    bool is_in_line_reading_mode() const;
    void error(const std::string &message) const;
public:
    explicit TaskLexer(std::istream &stream);

    /*
      Read a single token within a line. Tokens within a line are
      separated by arbitrary whitespaces. Report error if the current
      line does not contain a token after the cursor position. Set
      cursor to the end of the read token. Afterwards the lexer is in
      line reading mode, in which only read(), confirm_end_of_line() and
      get_line_number() are allowed.
    */
    std::string read();
    /*
      Read a complete line as a single string token. Report an error if
      the cursor is not at the beginning of a line before reading. Set
      cursor to the beginning of the next line. Not allowed in line
      reading mode.
    */
    std::string read_line();
    /*
      Check that the end of the line has been reached and set cursor to
      the beginning of the next line. Report error otherwise.
      This method is only allowed in line reading mode and will end the mode.
    */
    void confirm_end_of_line();
    /*
      Check that the end of the file has been reached. Report error otherwise.
      Not allowed in line reading mode.
    */
    void confirm_end_of_input();

    /*
      Return the line number of the cursor.
    */
    int get_line_number() const;
};
}
#endif
