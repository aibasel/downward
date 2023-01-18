#include "errors.h"

using namespace std;

namespace plugins {
void OptionParserError::print() const {
    cerr << "option parser error: " << msg << endl;
}

ParseError::ParseError(
    const string &msg, const ParseTree &parse_tree, const string &substring)
    : utils::Exception(msg),
      parse_tree(parse_tree),
      substring(substring) {
}

void ParseError::print() const {
    cerr << "parse error: " << endl
         << msg << " at: " << endl;
    kptree::print_tree_bracketed<ParseNode>(parse_tree, cerr);
    if (!substring.empty()) {
        cerr << " (cannot continue parsing after \"" << substring << "\")";
    }
    cerr << endl;
}


string get_demangling_hint(const string &type_name) {
    return "To retrieve the demangled C++ type for gcc/clang, you can call \n"
           "c++filt -t " + type_name;
}
}
