#include "errors.h"

using namespace std;

namespace options {
ArgError::ArgError(std::string msg_) : msg(msg_) {
}

ParseError::ParseError(string m, ParseTree pt)
    : msg(m),
      parse_tree(pt) {
}

ParseError::ParseError(string m, ParseTree pt, string correct_substring)
    : msg(m),
      parse_tree(pt),
      substr(correct_substring) {
}
}
