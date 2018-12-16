#include "errors.h"

#include <sstream>

using namespace std;

namespace options {
OptionParserError::OptionParserError(const string &msg)
    : runtime_error("option parser error: " + msg) {
}


static string get_parse_error_message(
    const string &error, const ParseTree &parse_tree, const string &substring) {
    stringstream out;
    out << "parse error: " << endl
        << error << " at: " << endl;
    kptree::print_tree_bracketed<ParseNode>(parse_tree, out);
    if (!substring.empty()) {
        out << " (cannot continue parsing after \"" << substring << "\")" << endl;
    }
    return out.str();
}

ParseError::ParseError(
    const string &error, const ParseTree &parse_tree, const string &substring)
    : runtime_error(get_parse_error_message(error, parse_tree, substring)) {
}


string get_demangling_hint(const string &type_name) {
    return "To retrieve the demangled C++ type for gcc/clang, you can call \n"
           "c++filt -t " + type_name;
}
}
