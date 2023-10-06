#include "lexical_analyzer.h"

#include "token_stream.h"

#include "../utils/logging.h"
#include "../utils/strings.h"

#include <regex>
#include <sstream>

using namespace std;

namespace parser {
static regex build_regex(const string &s) {
    return regex("^\\s*(" + s + ")\\s*",
                 regex_constants::ECMAScript | regex_constants::icase);
}

static vector<pair<TokenType, regex>> construct_token_type_expressions() {
    vector<pair<TokenType, string>> token_type_str_expression = {
        {TokenType::OPENING_PARENTHESIS, R"(\()"},
        {TokenType::CLOSING_PARENTHESIS, R"(\))"},
        {TokenType::OPENING_BRACKET, R"(\[)"},
        {TokenType::CLOSING_BRACKET, R"(\])"},
        {TokenType::COMMA, R"(,)"},
        {TokenType::EQUALS, R"(=)"},
        {TokenType::FLOAT,
         R"([+-]?(((\d*\.\d+|\d+\.)(e[+-]?\d+|[kmg]\b)?)|\d+e[+-]?\d+))"},
        {TokenType::INTEGER,
         R"([+-]?(infinity|\d+([kmg]\b)?))"},
        {TokenType::BOOLEAN, R"(true|false)"},
        {TokenType::LET, R"(let)"},
        {TokenType::IDENTIFIER, R"([a-zA-Z_]\w*)"}
    };
    vector<pair<TokenType, regex>> token_type_expression;
    token_type_expression.reserve(token_type_str_expression.size());
    for (const auto &pair : token_type_str_expression) {
        token_type_expression.emplace_back(pair.first, build_regex(pair.second));
    }
    return token_type_expression;
}
static const vector<pair<TokenType, regex>> token_type_expressions =
    construct_token_type_expressions();


TokenStream split_tokens(const string &text) {
    utils::Context context;
    utils::TraceBlock block(context, "Splitting Tokens.");

    vector<Token> tokens;
    auto start = text.begin();
    auto end = text.end();

    while (start != end) {
        bool has_match = false;
        smatch match;

        for (const auto &type_and_expression : token_type_expressions) {
            TokenType token_type = type_and_expression.first;
            const regex &expression = type_and_expression.second;
            if (regex_search(start, end, match, expression, regex_constants::match_continuous)) {
                tokens.push_back({utils::tolower(match[1]), token_type});
                start += match[0].length();
                has_match = true;
                break;
            }
        }
        if (!has_match) {
            ostringstream error;
            error << "Unable to recognize next token:" << endl;
            int distance_to_error = start - text.begin();
            for (const string &line : utils::split(text, "\n")) {
                int line_length = line.size();
                bool error_in_line =
                    distance_to_error < line_length && distance_to_error >= 0;
                error << (error_in_line ? "> " : "  ") << line << endl;
                if (error_in_line)
                    error << string(distance_to_error + 2, ' ') << "^" << endl;

                distance_to_error -= line.size() + 1;
            }
            string message = error.str();
            utils::rstrip(message);
            context.error(message);
        }
    }
    return TokenStream(move(tokens));
}
}
