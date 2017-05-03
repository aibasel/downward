#include "errors.h"

using namespace std;

namespace options {
ArgError::ArgError(const string &msg)
    : msg(msg) {
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
}
