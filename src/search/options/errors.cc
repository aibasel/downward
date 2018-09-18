#include "errors.h"

using namespace std;

namespace options {
ArgError::ArgError(const string &msg)
    : msg(msg) {
}

std::ostream &operator<<(std::ostream &out, const ArgError &err) {
    return out << "argument error: " << err.msg;
}


ParseError::ParseError(const string &msg, ParseTree parse_tree)
    : msg(msg),
      parse_tree(parse_tree) {
}

ParseError::ParseError(
    const string &msg, const ParseTree &parse_tree, const string &substring)
    : msg(msg),
      parse_tree(parse_tree),
      substring(substring) {
}

std::ostream &operator<<(std::ostream &out, const ParseError &parse_error) {
    out << "parse error: " << std::endl
        << parse_error.msg << " at: " << std::endl;
    kptree::print_tree_bracketed<ParseNode>(parse_error.parse_tree, out);
    if (!parse_error.substring.empty()) {
        out << " (cannot continue parsing after \"" << parse_error.substring
            << "\")" << std::endl;
    }
    return out;
}

}
