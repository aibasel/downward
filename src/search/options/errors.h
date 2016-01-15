#ifndef OPTIONS_ERRORS_H
#define OPTIONS_ERRORS_H

#include "parse_tree.h"

#include <iosfwd>
#include <string>

namespace options {
struct ArgError {
    ArgError(std::string msg);

    std::string msg;

    friend std::ostream &operator<<(std::ostream &out, const ArgError &err) {
        return out << "argument error: " << err.msg;
    }
};

struct ParseError {
    ParseError(std::string m, ParseTree pt);
    ParseError(std::string m, ParseTree pt, std::string correct_substring);

    std::string msg;
    ParseTree parse_tree;
    std::string substr;

    friend std::ostream &operator<<(std::ostream &out, const ParseError &pe) {
        out << "parse error: " << std::endl
            << pe.msg << " at: " << std::endl;
        kptree::print_tree_bracketed<ParseNode>(pe.parse_tree, out);
        if (pe.substr.size() > 0) {
            out << " (cannot continue parsing after \"" << pe.substr << "\")" << std::endl;
        }
        return out;
    }
};
}

#endif
