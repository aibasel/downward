#include "syntax_analyzer.h"

#include "errors.h"
#include "token_stream.h"

#include <functional>
#include <unordered_set>

using namespace std;

namespace parser {
class SyntaxAnalyzerTraceback : public Traceback {
    const TokenStream &tokens;
    const int lookahead;
public:
    SyntaxAnalyzerTraceback(TokenStream &tokens, int lookahead)
        : tokens(tokens),
          lookahead(lookahead) {}

    virtual void push(const string &layer) override {
        int pos = tokens.get_position();
        string message = layer + ": " + tokens.str(pos, pos + lookahead);
        if (pos + lookahead < tokens.size())
            message += " ...";
        Traceback::push(message);
    }

    virtual string str() const override {
        ostringstream message;
        message << Traceback::str();
        string all_tokens = tokens.str(0, tokens.size());
        string remaining_tokens = tokens.str(tokens.get_position(), tokens.size());
        message << endl
                << all_tokens << endl
                << string(all_tokens.size() - remaining_tokens.size(), ' ')
                << "^";
        return message.str();
    }
};

static ASTNodePtr parse_node(TokenStream &tokens, SyntaxAnalyzerTraceback &traceback);

static void parse_argument(TokenStream &tokens,
                           vector<ASTNodePtr> &positional_arguments,
                           unordered_map<string, ASTNodePtr> &keyword_arguments,
                           SyntaxAnalyzerTraceback &traceback) {
    if (tokens.has_tokens(2)
        && tokens.peek(0).type == TokenType::IDENTIFIER
        && tokens.peek(1).type == TokenType::EQUALS) {
        string argument_name = tokens.pop().content;
        tokens.pop(TokenType::EQUALS);
        if (keyword_arguments.count(argument_name)) {
            throw ParserError(
                      "Multiple definitions of the same keyword argument '" +
                      argument_name + "'.",
                      traceback);
        }
        keyword_arguments[argument_name] = parse_node(tokens, traceback);
    } else {
        if (!keyword_arguments.empty()) {
            throw ParserError(
                      "Positional arguments have to be defined before any "
                      "keyword arguments.",
                      traceback);
        }
        positional_arguments.push_back(parse_node(tokens, traceback));
    }
}

static ASTNodePtr parse_let(TokenStream &tokens, SyntaxAnalyzerTraceback &traceback) {
    traceback.push("Let");
    tokens.pop(TokenType::LET);
    tokens.pop(TokenType::OPENING_PARENTHESIS);

    traceback.push("Parsing variable name");
    string variable_name = tokens.pop(TokenType::IDENTIFIER).content;
    traceback.pop();
    traceback.push("Found variable name '" + variable_name + "'");

    traceback.push("Expecting separation of variable name and definition");
    tokens.pop(TokenType::COMMA);
    traceback.pop();

    traceback.push("Parsing variable definition");
    ASTNodePtr variable_definition = parse_node(tokens, traceback);
    traceback.pop();

    traceback.push("Expecting separation of variable definition and expression");
    tokens.pop(TokenType::COMMA);
    traceback.pop();

    traceback.push("Parsing expression");
    ASTNodePtr nested_value = parse_node(tokens, traceback);
    traceback.pop();

    traceback.push("Expecting closing of let");
    tokens.pop(TokenType::CLOSING_PARENTHESIS);
    traceback.pop();
    traceback.pop();
    traceback.pop();
    return utils::make_unique_ptr<LetNode>(
        variable_name, move(variable_definition), move(nested_value));
}

static void parse_sequence(
    TokenStream &tokens, SyntaxAnalyzerTraceback &traceback,
    TokenType terminal_token, function<void(void)> func) {
    traceback.push("Parsing arguments");
    int num_argument = 1;
    while (tokens.peek().type != terminal_token) {
        traceback.pop();

        traceback.push(to_string(num_argument) + ". argument");
        func();
        traceback.pop();

        traceback.push(
            "Parsed " + to_string(num_argument++) + ". argument");
        TokenType next_type = tokens.peek().type;
        if (next_type != TokenType::COMMA &&
            next_type != terminal_token) {
            throw ParserError(
                      "Read unexpected token type '" +
                      token_type_name(next_type) + "'. Expected token types '" +
                      token_type_name(terminal_token) + "' or '" +
                      token_type_name(TokenType::COMMA),
                      traceback);
        } else if (next_type == TokenType::COMMA) {
            if (tokens.peek(1).type == terminal_token) {
                throw ParserError("Trailing commas are forbidden.",
                                  traceback);
            } else {
                tokens.pop();
            }
        }
    }
}

static ASTNodePtr parse_function(TokenStream &tokens, SyntaxAnalyzerTraceback &traceback) {
    traceback.push("Reading plugin name");
    int start = tokens.get_position();
    Token name_token = tokens.pop(TokenType::IDENTIFIER);
    string name = name_token.content;
    traceback.pop();

    traceback.push("Plugin '" + name + "'");
    vector<ASTNodePtr> positional_arguments;
    unordered_map<string, ASTNodePtr> keyword_arguments;
    auto callback = [&]() -> void {
            parse_argument(tokens, positional_arguments, keyword_arguments, traceback);
        };
    tokens.pop(TokenType::OPENING_PARENTHESIS);
    parse_sequence(tokens, traceback, TokenType::CLOSING_PARENTHESIS, callback);
    tokens.pop(TokenType::CLOSING_PARENTHESIS);
    traceback.pop();
    traceback.pop();

    string unparsed_config = tokens.str(start, tokens.get_position());
    return utils::make_unique_ptr<FunctionCallNode>(
        name, move(positional_arguments), move(keyword_arguments), unparsed_config);
}

static unordered_set<TokenType> literal_tokens {
    TokenType::FLOAT,
    TokenType::INTEGER,
    TokenType::BOOLEAN,
    TokenType::IDENTIFIER
};

static ASTNodePtr parse_literal(TokenStream &tokens, SyntaxAnalyzerTraceback &traceback) {
    traceback.push("Literal");
    Token token = tokens.pop();
    if (!literal_tokens.count(token.type)) {
        ostringstream message;
        message << "Token " << token << " cannot be parsed as literal";
        throw ParserError(message.str(), traceback);
    }
    traceback.pop();
    return utils::make_unique_ptr<LiteralNode>(token);
}

static ASTNodePtr parse_list(TokenStream &tokens, SyntaxAnalyzerTraceback &traceback) {
    traceback.push("List");
    vector<ASTNodePtr> elements;
    auto callback = [&]() -> void {
            elements.push_back(parse_node(tokens, traceback));
        };
    tokens.pop(TokenType::OPENING_BRACKET);
    parse_sequence(tokens, traceback, TokenType::CLOSING_BRACKET, callback);
    tokens.pop(TokenType::CLOSING_BRACKET);
    traceback.pop();
    traceback.pop();
    return utils::make_unique_ptr<ListNode>(move(elements));
}

static vector<TokenType> PARSE_NODE_TOKEN_TYPES = {
    TokenType::LET, TokenType::IDENTIFIER, TokenType::BOOLEAN,
    TokenType::INTEGER, TokenType::FLOAT, TokenType::OPENING_BRACKET};

static ASTNodePtr parse_node(TokenStream &tokens, SyntaxAnalyzerTraceback &traceback) {
    Token token = tokens.peek();
    if (find(PARSE_NODE_TOKEN_TYPES.begin(),
             PARSE_NODE_TOKEN_TYPES.end(),
             token.type) == PARSE_NODE_TOKEN_TYPES.end()) {
        ostringstream message;
        message << "Unexpected token '" << token
                << "'. Expected any of the following token types: "
                << utils::join(PARSE_NODE_TOKEN_TYPES, ", ");

        throw ParserError(message.str(), traceback);
    }

    switch (token.type) {
    case TokenType::LET:
        return parse_let(tokens, traceback);
    case TokenType::IDENTIFIER:
        if (tokens.has_tokens(2)
            && tokens.peek(1).type == TokenType::OPENING_PARENTHESIS) {
            return parse_function(tokens, traceback);
        } else {
            return parse_literal(tokens, traceback);
        }
    case TokenType::BOOLEAN:
    case TokenType::INTEGER:
    case TokenType::FLOAT:
        return parse_literal(tokens, traceback);
    case TokenType::OPENING_BRACKET:
        return parse_list(tokens, traceback);
    default:
        ABORT("Unknown token type '" + token_type_name(token.type) + "'.");
    }
}

ASTNodePtr parse(TokenStream &tokens) {
    SyntaxAnalyzerTraceback traceback(tokens, 10);
    traceback.push("Start Syntactical Parsing");
    if (!tokens.has_tokens(1)) {
        throw ParserError("Input is empty", traceback);
    }
    ASTNodePtr node;
    try {
        node = parse_node(tokens, traceback);
    } catch (TokenStreamError &error) {
        throw ParserError(error.get_message(), traceback);
    }
    if (tokens.has_tokens(1)) {
        throw ParserError(
                  "Syntax analysis terminated with unparsed tokens: " +
                  tokens.str(tokens.get_position(), tokens.size()),
                  traceback);
    }
    traceback.pop();
    return node;
}
}
