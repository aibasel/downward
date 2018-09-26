#ifndef OPTIONS_ERRORS_H
#define OPTIONS_ERRORS_H

#include "parse_tree.h"

#include <ostream>
#include <string>

namespace options {
struct ArgError {
    std::string msg;

    ArgError(const std::string &msg);

    friend std::ostream &operator<<(std::ostream &out, const ArgError &err);
};


struct ParseError {
    std::string msg;
    ParseTree parse_tree;
    std::string substring;

    ParseError(const std::string &msg, ParseTree parse_tree);
    ParseError(
        const std::string &msg, const ParseTree &parse_tree, const std::string &substring);

    friend std::ostream &operator<<(std::ostream &out, const ParseError &parse_error);
};
}

#endif
