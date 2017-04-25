#include "errors.h"

using namespace std;

namespace options {
ArgError::ArgError(const string &msg)
    : msg(msg) {
}

ParseError::ParseError(const string &m, ParseTree pt)
    : msg(m),
      parse_tree(pt) {
}

ParseError::ParseError(const string &m, ParseTree pt, const string &correct_substring)
    : msg(m),
      parse_tree(pt),
      substr(correct_substring) {
}
}
