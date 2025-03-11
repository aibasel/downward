#include "syntax_analyzer.h"

#include "token_stream.h"

#include "../utils/logging.h"

#include <functional>
#include <unordered_set>

using namespace std;

namespace parser {
class SyntaxAnalyzerContext : public utils::Context {
    const TokenStream &tokens;
    const int lookahead;

public:
    SyntaxAnalyzerContext(TokenStream &tokens, int lookahead)
        : tokens(tokens),
          lookahead(lookahead) {
    }

    virtual string decorate_block_name(const string &block_name) const override {
        ostringstream decorated_block_name;
        int pos = tokens.get_position();
        decorated_block_name << block_name << ": "
                             << tokens.str(pos, pos + lookahead);
        if (pos + lookahead < tokens.size())
            decorated_block_name << " ...";
        return decorated_block_name.str();
    }

    virtual void error(const string &message) const override {
        ostringstream message_with_tokens;
        string all_tokens = tokens.str(0, tokens.size());
        string remaining_tokens = tokens.str(tokens.get_position(), tokens.size());
        message_with_tokens << all_tokens << endl
                            << string(all_tokens.size() - remaining_tokens.size(), ' ')
                            << "^" << endl
                            << message;
        throw utils::ContextError(str() + "\n\n" + message_with_tokens.str());
    }
};

static ASTNodePtr parse_node(TokenStream &tokens, SyntaxAnalyzerContext &);

static void parse_argument(TokenStream &tokens,
                           vector<ASTNodePtr> &positional_arguments,
                           unordered_map<string, ASTNodePtr> &keyword_arguments,
                           SyntaxAnalyzerContext &context) {
    if (tokens.has_tokens(2)
        && tokens.peek(context, 0).type == TokenType::IDENTIFIER
        && tokens.peek(context, 1).type == TokenType::EQUALS) {
        string argument_name = tokens.pop(context).content;
        tokens.pop(context, TokenType::EQUALS);
        if (keyword_arguments.count(argument_name)) {
            context.error("Multiple definitions of the same keyword "
                          "argument '" + argument_name + "'.");
        }
        keyword_arguments[argument_name] = parse_node(tokens, context);
    } else {
        if (!keyword_arguments.empty()) {
            context.error("Positional arguments have to be defined before "
                          "any keyword arguments.");
        }
        positional_arguments.push_back(parse_node(tokens, context));
    }
}

static ASTNodePtr parse_let(TokenStream &tokens, SyntaxAnalyzerContext &context) {
    utils::TraceBlock block(context, "Parsing Let");
    tokens.pop(context, TokenType::LET);
    tokens.pop(context, TokenType::OPENING_PARENTHESIS);
    string variable_name;
    {
        utils::TraceBlock block(context, "Parsing variable name");
        variable_name = tokens.pop(context, TokenType::IDENTIFIER).content;
    }
    {
        utils::TraceBlock block(context, "Parsing comma after variable name.");
        tokens.pop(context, TokenType::COMMA);
    }
    ASTNodePtr variable_definition;
    {
        utils::TraceBlock block(context, "Parsing variable definition");
        variable_definition = parse_node(tokens, context);
    }
    {
        utils::TraceBlock block(context, "Parsing comma after variable definition.");
        tokens.pop(context, TokenType::COMMA);
    }
    ASTNodePtr nested_value;
    {
        utils::TraceBlock block(context, "Parsing nested expression of let");
        nested_value = parse_node(tokens, context);
    }
    tokens.pop(context, TokenType::CLOSING_PARENTHESIS);
    return make_unique<LetNode>(
        variable_name, move(variable_definition), move(nested_value));
}

static void parse_sequence(
    TokenStream &tokens, SyntaxAnalyzerContext &context,
    TokenType terminal_token, const function<void(void)> &func) {
    utils::TraceBlock block(context, "Parsing sequence");
    int num_argument = 1;
    while (tokens.peek(context).type != terminal_token) {
        {
            utils::TraceBlock block(context, "Parsing " + to_string(num_argument) + ". argument");
            func();
        }
        {
            utils::TraceBlock block(context, "Parsing token after " + to_string(num_argument) + ". argument");
            TokenType next_type = tokens.peek(context).type;
            if (next_type == terminal_token) {
                return;
            } else if (next_type == TokenType::COMMA) {
                tokens.pop(context);
                if (tokens.peek(context).type == terminal_token) {
                    context.error("Trailing commas are forbidden.");
                }
            } else {
                context.error(
                    "Read unexpected token type '" +
                    token_type_name(next_type) + "'. Expected token types '" +
                    token_type_name(terminal_token) + "' or '" +
                    token_type_name(TokenType::COMMA));
            }
        }
        num_argument++;
    }
}

static ASTNodePtr parse_function(TokenStream &tokens,
                                 SyntaxAnalyzerContext &context) {
    int initial_token_stream_index = tokens.get_position();
    utils::TraceBlock block(context, "Parsing plugin");
    string plugin_name;
    {
        utils::TraceBlock block(context, "Parsing plugin name");
        Token name_token = tokens.pop(context, TokenType::IDENTIFIER);
        plugin_name = name_token.content;
    }
    tokens.pop(context, TokenType::OPENING_PARENTHESIS);
    vector<ASTNodePtr> positional_arguments;
    unordered_map<string, ASTNodePtr> keyword_arguments;
    {
        utils::TraceBlock block(context, "Parsing plugin arguments");
        auto callback = [&]() -> void {
                parse_argument(tokens, positional_arguments, keyword_arguments, context);
            };
        parse_sequence(tokens, context, TokenType::CLOSING_PARENTHESIS, callback);
    }
    tokens.pop(context, TokenType::CLOSING_PARENTHESIS);
    string unparsed_config = tokens.str(initial_token_stream_index, tokens.get_position());
    return make_unique<FunctionCallNode>(
        plugin_name, move(positional_arguments), move(keyword_arguments), unparsed_config);
}

static unordered_set<TokenType> literal_tokens {
    TokenType::BOOLEAN,
    TokenType::STRING,
    TokenType::INTEGER,
    TokenType::FLOAT,
    TokenType::IDENTIFIER
};

static ASTNodePtr parse_literal(TokenStream &tokens, SyntaxAnalyzerContext &context) {
    utils::TraceBlock block(context, "Parsing Literal");
    Token token = tokens.pop(context);
    if (!literal_tokens.count(token.type)) {
        ostringstream message;
        message << "Token " << token << " cannot be parsed as literal";
        context.error(message.str());
    }
    return make_unique<LiteralNode>(token);
}

static ASTNodePtr parse_list(TokenStream &tokens, SyntaxAnalyzerContext &context) {
    utils::TraceBlock block(context, "Parsing List");
    tokens.pop(context, TokenType::OPENING_BRACKET);
    vector<ASTNodePtr> elements;
    {
        utils::TraceBlock block(context, "Parsing list arguments");
        auto callback = [&]() -> void {
                elements.push_back(parse_node(tokens, context));
            };
        parse_sequence(tokens, context, TokenType::CLOSING_BRACKET, callback);
    }
    tokens.pop(context, TokenType::CLOSING_BRACKET);
    return make_unique<ListNode>(move(elements));
}

static vector<TokenType> parse_node_token_types = {
    TokenType::OPENING_BRACKET, TokenType::LET, TokenType::BOOLEAN,
    TokenType::STRING, TokenType::INTEGER, TokenType::FLOAT,
    TokenType::IDENTIFIER};

static ASTNodePtr parse_node(TokenStream &tokens,
                             SyntaxAnalyzerContext &context) {
    utils::TraceBlock block(context, "Identify node type");
    Token token = tokens.peek(context);
    if (find(parse_node_token_types.begin(),
             parse_node_token_types.end(),
             token.type) == parse_node_token_types.end()) {
        ostringstream message;
        message << "Unexpected token '" << token
                << "'. Expected any of the following token types: "
                << utils::join(parse_node_token_types, ", ");
        context.error(message.str());
    }

    switch (token.type) {
    case TokenType::OPENING_BRACKET:
        return parse_list(tokens, context);
    case TokenType::LET:
        return parse_let(tokens, context);
    case TokenType::BOOLEAN:
    case TokenType::STRING:
    case TokenType::INTEGER:
    case TokenType::FLOAT:
        return parse_literal(tokens, context);
    case TokenType::IDENTIFIER:
        if (tokens.has_tokens(2)
            && tokens.peek(context, 1).type == TokenType::OPENING_PARENTHESIS) {
            return parse_function(tokens, context);
        } else {
            return parse_literal(tokens, context);
        }
    default:
        ABORT("Unknown token type '" + token_type_name(token.type) + "'.");
    }
}

ASTNodePtr parse(TokenStream &tokens) {
    SyntaxAnalyzerContext context(tokens, 10);
    utils::TraceBlock block(context, "Start Syntactical Parsing");
    if (!tokens.has_tokens(1)) {
        context.error("Input is empty");
    }
    ASTNodePtr node = parse_node(tokens, context);
    if (tokens.has_tokens(1)) {
        context.error(
            "Syntax analysis terminated with unparsed tokens: " +
            tokens.str(tokens.get_position(), tokens.size()));
    }
    return node;
}
}
