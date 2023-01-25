#include "errors.h"

#include "../utils/strings.h"

using namespace std;

namespace parser {
TokenStreamError::TokenStreamError(const string &msg)
    : utils::Exception(msg) {
}

ParserError::ParserError(const string &msg)
    : utils::Exception(msg) {
}
}
