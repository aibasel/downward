#ifndef PARSER_SYNTAX_ANALYZER_H
#define PARSER_SYNTAX_ANALYZER_H

#include "abstract_syntax_tree.h"

namespace parser {
class TokenStream;

extern ASTNodePtr parse(TokenStream &tokens);
}
#endif
