#ifndef OPTIONS_ERRORS_H
#define OPTIONS_ERRORS_H

#include "parse_tree.h"

#include <iostream>
#include <string>

namespace options {
struct ArgError {
    std::string msg;

    ArgError(const std::string &msg);

    friend std::ostream &operator<<(std::ostream &out, const ArgError &err) {
        return out << "argument error: " << err.msg;
    }
};


struct ParseError {
    std::string msg;
    ParseTree parse_tree;
    std::string substring;

    ParseError(const std::string &msg, ParseTree parse_tree);
    ParseError(
        const std::string &msg, const ParseTree &parse_tree, const std::string &substring);

    friend std::ostream &operator<<(std::ostream &out, const ParseError &parse_error) {
        out << "parse error: " << std::endl
            << parse_error.msg << " at: " << std::endl;
        kptree::print_tree_bracketed<ParseNode>(parse_error.parse_tree, out);
        if (!parse_error.substring.empty()) {
            out << " (cannot continue parsing after \"" << parse_error.substring
                << "\")" << std::endl;
        }
        return out;
    }
};
}

#endif
