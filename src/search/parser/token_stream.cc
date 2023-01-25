#include "token_stream.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/strings.h"
#include "../utils/system.h"

#include <cassert>
#include <sstream>

using namespace std;

namespace parser {
Token::Token(const string &content, TokenType type)
    : content(content), type(type) {
}

TokenStream::TokenStream(vector<Token> &&tokens)
    : tokens(move(tokens)), pos(0) {
}

bool TokenStream::has_tokens(int n) const {
    assert(n > 0);
    return utils::in_bounds(pos + n - 1, tokens);
}

Token TokenStream::peek(const utils::Context &context, int n) const {
    if (!has_tokens(n + 1)) {
        ostringstream message;
        message << (n + 1) << " token(s) required, but "
                << (tokens.size() - pos) << " remain.";
        context.error(message.str());
    }
    return tokens[pos + n];
}

Token TokenStream::pop(const utils::Context &context) {
    if (!has_tokens(1)) {
        context.error("Input stream exhausted.");
    }
    return tokens[pos++];
}

Token TokenStream::pop(const utils::Context &context, TokenType expected_type) {
    if (!has_tokens(1)) {
        context.error(
            "Input stream exhausted while expecting token of type '" +
            token_type_name(expected_type) + "'.");
    }
    Token token = pop(context);
    if (token.type != expected_type) {
        ostringstream message;
        message << "Got token " << token << ". Expected token of type '"
                << expected_type << "'.";
        context.error(message.str());
    }
    return token;
}

int TokenStream::get_position() const {
    return pos;
}

int TokenStream::size() const {
    return tokens.size();
}

string TokenStream::str(int from, int to) const {
    int curr_position = max(0, from);
    int max_position = min(static_cast<int>(tokens.size()), to);
    ostringstream message;
    while (curr_position < max_position) {
        message << tokens[curr_position].content;
        curr_position++;
    }
    return message.str();
}

string token_type_name(TokenType token_type) {
    switch (token_type) {
    case TokenType::OPENING_PARENTHESIS:
        return "(";
    case TokenType::CLOSING_PARENTHESIS:
        return ")";
    case TokenType::OPENING_BRACKET:
        return "[";
    case TokenType::CLOSING_BRACKET:
        return "]";
    case TokenType::COMMA:
        return ",";
    case TokenType::EQUALS:
        return "=";
    case TokenType::INTEGER:
        return "Integer";
    case TokenType::FLOAT:
        return "Float";
    case TokenType::BOOLEAN:
        return "Boolean";
    case TokenType::IDENTIFIER:
        return "Identifier";
    case TokenType::LET:
        return "Let";
    default:
        ABORT("Unknown token type.");
    }
}

ostream &operator<<(ostream &out, TokenType token_type) {
    out << token_type_name(token_type);
    return out;
}

ostream &operator<<(ostream &out, const Token &token) {
    out << "<Type: '" << token.type << "', Value: '" << token.content << "'>";
    return out;
}
}
