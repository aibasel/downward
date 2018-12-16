#include "errors.h"

#include <sstream>

using namespace std;

namespace options {
OptionParserError::OptionParserError(const string &msg)
    : msg("option parser error: " + msg) {
}

const char *OptionParserError::what() const noexcept {
    return msg.c_str();
}


ParseError::ParseError(
    const string &error, const ParseTree &parse_tree, const string &substring) {
    stringstream out;
    out << "parse error: " << endl
        << error << " at: " << endl;
    kptree::print_tree_bracketed<ParseNode>(parse_tree, out);
    if (!substring.empty()) {
        out << " (cannot continue parsing after \"" << substring << "\")" << endl;
    }
    msg = out.str();
}

const char *ParseError::what() const noexcept {
    return msg.c_str();
}


string get_demangling_hint(const string &type_name) {
    return "To retrieve the demangled C++ type for gcc/clang, you can call \n"
           "c++filt -t " + type_name;
}
}
