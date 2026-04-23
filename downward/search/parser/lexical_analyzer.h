#ifndef PARSER_LEXICAL_ANALYZER_H
#define PARSER_LEXICAL_ANALYZER_H

#include <string>

namespace parser {
class TokenStream;

extern TokenStream split_tokens(const std::string &text);
}
#endif
