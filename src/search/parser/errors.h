#ifndef PARSER_ERRORS_H
#define PARSER_ERRORS_H

#include "../utils/exceptions.h"
#include "../utils/language.h"

#include <string>
#include <vector>

namespace parser {
class TokenStreamError : public utils::Exception {
public:
    explicit TokenStreamError(const std::string &msg);
};

class ParserError : public utils::Exception {
public:
    explicit ParserError(const std::string &msg);
};
}

#endif
