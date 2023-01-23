#include "syntax_analyzer.h"

#include "errors.h"
#include "token_stream.h"

#include <functional>
#include <unordered_set>

using namespace std;

namespace parser {
class SyntaxAnalyzerContext {
    Traceback traceback;
    const TokenStream &tokens;
    const int lookahead;
public:
    SyntaxAnalyzerContext(TokenStream &tokens, int lookahead)
        : tokens(tokens),
          lookahead(lookahead) {
    }

    void push_layer(const string &layer) {
        int pos = tokens.get_position();
        string message = layer + ": " + tokens.str(pos, pos + lookahead);
        if (pos + lookahead < tokens.size())
            message += " ...";
        traceback.push(message);
    }

    void pop_layer() {
        traceback.pop();
    }

    NO_RETURN
    void parser_error(const string &message) const {
        ostringstream message_with_tokens;
        string all_tokens = tokens.str(0, tokens.size());
        string remaining_tokens = tokens.str(tokens.get_position(), tokens.size());
        message_with_tokens << all_tokens << endl
                            << string(all_tokens.size() - remaining_tokens.size(), ' ')
                            << "^" << endl
                            << message;
        throw ParserError(message_with_tokens.str(), traceback);
    }
};

static ASTNodePtr parse_node(TokenStream &tokens, SyntaxAnalyzerContext &);

static void parse_argument(TokenStream &tokens,
                           vector<ASTNodePtr> &positional_arguments,
                           unordered_map<string, ASTNodePtr> &keyword_arguments,
                           SyntaxAnalyzerContext &context) {
    if (tokens.has_tokens(2)
        && tokens.peek(0).type == TokenType::IDENTIFIER
        && tokens.peek(1).type == TokenType::EQUALS) {
        string argument_name = tokens.pop().content;
        tokens.pop(TokenType::EQUALS);
        if (keyword_arguments.count(argument_name)) {
            context.parser_error("Multiple definitions of the same keyword "
                                 "argument '" + argument_name + "'.");
        }
        keyword_arguments[argument_name] = parse_node(tokens, context);
    } else {
        if (!keyword_arguments.empty()) {
            context.parser_error("Positional arguments have to be defined before "
                                 "any keyword arguments.");
        }
        positional_arguments.push_back(parse_node(tokens, context));
    }
}

static ASTNodePtr parse_let(TokenStream &tokens, SyntaxAnalyzerContext &context) {
    context.push_layer("Let");
    tokens.pop(TokenType::LET);
    tokens.pop(TokenType::OPENING_PARENTHESIS);

    context.push_layer("Parsing variable name");
    string variable_name = tokens.pop(TokenType::IDENTIFIER).content;
    context.pop_layer();
    context.push_layer("Found variable name '" + variable_name + "'");

    context.push_layer("Expecting separation of variable name and definition");
    tokens.pop(TokenType::COMMA);
    context.pop_layer();

    context.push_layer("Parsing variable definition");
    ASTNodePtr variable_definition = parse_node(tokens, context);
    context.pop_layer();

    context.push_layer("Expecting separation of variable definition and expression");
    tokens.pop(TokenType::COMMA);
    context.pop_layer();

    context.push_layer("Parsing expression");
    ASTNodePtr nested_value = parse_node(tokens, context);
    context.pop_layer();

    context.push_layer("Expecting closing of let");
    tokens.pop(TokenType::CLOSING_PARENTHESIS);
    context.pop_layer();
    context.pop_layer();
    context.pop_layer();
    return utils::make_unique_ptr<LetNode>(
        variable_name, move(variable_definition), move(nested_value));
}

static void parse_sequence(
    TokenStream &tokens, SyntaxAnalyzerContext &context,
    TokenType terminal_token, function<void(void)> func) {
    context.push_layer("Parsing arguments");
    int num_argument = 1;
    while (tokens.peek().type != terminal_token) {
        context.pop_layer();

        context.push_layer(to_string(num_argument) + ". argument");
        func();
        context.pop_layer();

        context.push_layer(
            "Parsed " + to_string(num_argument++) + ". argument");
        TokenType next_type = tokens.peek().type;
        if (next_type != TokenType::COMMA &&
            next_type != terminal_token) {
            context.parser_error(
                "Read unexpected token type '" +
                token_type_name(next_type) + "'. Expected token types '" +
                token_type_name(terminal_token) + "' or '" +
                token_type_name(TokenType::COMMA));
        } else if (next_type == TokenType::COMMA) {
            if (tokens.peek(1).type == terminal_token) {
                context.parser_error("Trailing commas are forbidden.");
            } else {
                tokens.pop();
            }
        }
    }
}

static ASTNodePtr parse_function(TokenStream &tokens,
                                 SyntaxAnalyzerContext &context) {
    context.push_layer("Reading plugin name");
    int start = tokens.get_position();
    Token name_token = tokens.pop(TokenType::IDENTIFIER);
    string name = name_token.content;
    context.pop_layer();

    context.push_layer("Plugin '" + name + "'");
    vector<ASTNodePtr> positional_arguments;
    unordered_map<string, ASTNodePtr> keyword_arguments;
    auto callback = [&]() -> void {
            parse_argument(tokens, positional_arguments, keyword_arguments, context);
        };
    tokens.pop(TokenType::OPENING_PARENTHESIS);
    parse_sequence(tokens, context, TokenType::CLOSING_PARENTHESIS, callback);
    tokens.pop(TokenType::CLOSING_PARENTHESIS);
    context.pop_layer();
    context.pop_layer();

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

static ASTNodePtr parse_literal(TokenStream &tokens, SyntaxAnalyzerContext &context) {
    context.push_layer("Literal");
    Token token = tokens.pop();
    if (!literal_tokens.count(token.type)) {
        ostringstream message;
        message << "Token " << token << " cannot be parsed as literal";
        context.parser_error(message.str());
    }
    context.pop_layer();
    return utils::make_unique_ptr<LiteralNode>(token);
}

static ASTNodePtr parse_list(TokenStream &tokens, SyntaxAnalyzerContext &context) {
    context.push_layer("List");
    vector<ASTNodePtr> elements;
    auto callback = [&]() -> void {
            elements.push_back(parse_node(tokens, context));
        };
    tokens.pop(TokenType::OPENING_BRACKET);
    parse_sequence(tokens, context, TokenType::CLOSING_BRACKET, callback);
    tokens.pop(TokenType::CLOSING_BRACKET);
    context.pop_layer();
    context.pop_layer();
    return utils::make_unique_ptr<ListNode>(move(elements));
}

static vector<TokenType> PARSE_NODE_TOKEN_TYPES = {
    TokenType::LET, TokenType::IDENTIFIER, TokenType::BOOLEAN,
    TokenType::INTEGER, TokenType::FLOAT, TokenType::OPENING_BRACKET};

static ASTNodePtr parse_node(TokenStream &tokens,
                             SyntaxAnalyzerContext &context) {
    Token token = tokens.peek();
    if (find(PARSE_NODE_TOKEN_TYPES.begin(),
             PARSE_NODE_TOKEN_TYPES.end(),
             token.type) == PARSE_NODE_TOKEN_TYPES.end()) {
        ostringstream message;
        message << "Unexpected token '" << token
                << "'. Expected any of the following token types: "
                << utils::join(PARSE_NODE_TOKEN_TYPES, ", ");
        context.parser_error(message.str());
    }

    switch (token.type) {
    case TokenType::LET:
        return parse_let(tokens, context);
    case TokenType::IDENTIFIER:
        if (tokens.has_tokens(2)
            && tokens.peek(1).type == TokenType::OPENING_PARENTHESIS) {
            return parse_function(tokens, context);
        } else {
            return parse_literal(tokens, context);
        }
    case TokenType::BOOLEAN:
    case TokenType::INTEGER:
    case TokenType::FLOAT:
        return parse_literal(tokens, context);
    case TokenType::OPENING_BRACKET:
        return parse_list(tokens, context);
    default:
        ABORT("Unknown token type '" + token_type_name(token.type) + "'.");
    }
}

ASTNodePtr parse(TokenStream &tokens) {
    SyntaxAnalyzerContext context(tokens, 10);
    context.push_layer("Start Syntactical Parsing");
    if (!tokens.has_tokens(1)) {
        context.parser_error("Input is empty");
    }
    ASTNodePtr node;
    try {
        node = parse_node(tokens, context);
    } catch (TokenStreamError &error) {
        context.parser_error(error.get_message());
    }
    if (tokens.has_tokens(1)) {
        context.parser_error(
            "Syntax analysis terminated with unparsed tokens: " +
            tokens.str(tokens.get_position(), tokens.size()));
    }
    context.pop_layer();
    return node;
}
}
