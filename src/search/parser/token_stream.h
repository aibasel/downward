#ifndef PARSER_TOKEN_STREAM_H
#define PARSER_TOKEN_STREAM_H

#include <string>
#include <vector>

namespace utils {
class Context;
}

namespace parser {
enum class TokenType {
    OPENING_PARENTHESIS,
    CLOSING_PARENTHESIS,
    OPENING_BRACKET,
    CLOSING_BRACKET,
    COMMA,
    EQUALS,
    LET,
    BOOLEAN,
    STRING,
    INTEGER,
    FLOAT,
    IDENTIFIER
};

struct Token {
    std::string content;
    TokenType type;

    Token(const std::string &content, TokenType type);
};

class TokenStream {
    std::vector<Token> tokens;
    int pos;
public:
    explicit TokenStream(std::vector<Token> &&tokens);

    bool has_tokens(int n) const;
    Token peek(const utils::Context &context, int n = 0) const;
    Token pop(const utils::Context &context);
    Token pop(const utils::Context &context, TokenType expected_type);

    int get_position() const;
    int size() const;
    std::string str(int from, int to) const;
};

extern std::string token_type_name(TokenType token_type);
extern std::ostream &operator<<(std::ostream &out, TokenType token_type);
extern std::ostream &operator<<(std::ostream &out, const Token &token);
}
namespace std {
template<>
struct hash<parser::TokenType> {
    size_t operator()(const parser::TokenType &t) const {
        return size_t(t);
    }
};
}
#endif
