#include "abstract_syntax_tree.h"

#include "lexical_analyzer.h"
#include "syntax_analyzer.h"
#include "token_stream.h"

#include "../plugins/plugin.h"
#include "../plugins/registry.h"
#include "../plugins/types.h"
#include "../utils/logging.h"

#include <cassert>
#include <unordered_map>
#include <vector>

using namespace std;

namespace parser {
class DecorateContext : public utils::Context {
    const plugins::Registry registry;
    unordered_map<string, const plugins::Type *> variables;

public:
    DecorateContext()
        : registry(plugins::RawRegistry::instance()->construct_registry()) {
    }

    void add_variable(const string &name, const plugins::Type &type) {
        if (has_variable(name))
            error("Variable '" + name + "' is already defined in the "
                  "current scope. Shadowing variables is not supported.");
        variables.insert({name, &type});
    }

    void remove_variable(const string &name) {
        variables.erase(name);
    }

    bool has_variable(const string &name) const {
        return variables.count(name);
    }

    const plugins::Type &get_variable_type(const string &name) {
        assert(has_variable(name));
        return *variables[name];
    }

    const plugins::Registry &get_registry() const {return registry;}
};

template<typename T, typename K>
static vector<T> get_keys(const unordered_map<T, K> &map) {
    vector<T> keys;
    keys.reserve(map.size());
    for (const auto &key_value : map) {
        keys.push_back(key_value.first);
    }
    return keys;
}

static ASTNodePtr parse_ast_node(const string &definition, DecorateContext &) {
    TokenStream tokens = split_tokens(definition);
    return parse(tokens);
}

DecoratedASTNodePtr ASTNode::decorate() const {
    DecorateContext context;
    utils::TraceBlock block(context, "Start semantic analysis");
    return decorate(context);
}

LetNode::LetNode(const string &variable_name, ASTNodePtr variable_definition,
                 ASTNodePtr nested_value)
    : variable_name(variable_name), variable_definition(move(variable_definition)),
      nested_value(move(nested_value)) {
}

DecoratedASTNodePtr LetNode::decorate(DecorateContext &context) const {
    utils::TraceBlock block(context, "Checking Let: " + variable_name);
    const plugins::Type &var_type = variable_definition->get_type(context);
    if (!var_type.supports_variable_binding()) {
        context.error(
            "The value of variable '" + variable_name +
            "' is not permitted to be assigned to a variable.");
    }
    DecoratedASTNodePtr decorated_definition;
    {
        utils::TraceBlock block(context, "Check variable definition");
        decorated_definition = variable_definition->decorate(context);
    }
    DecoratedASTNodePtr decorated_nested_value;
    {
        utils::TraceBlock block(context, "Check nested expression.");
        context.add_variable(variable_name, var_type);
        decorated_nested_value = nested_value->decorate(context);
        context.remove_variable(variable_name);
    }
    return utils::make_unique_ptr<DecoratedLetNode>(
        variable_name, move(decorated_definition), move(decorated_nested_value));
}

void LetNode::dump(string indent) const {
    cout << indent << "LET:" << variable_name << " = " << endl;
    indent = "| " + indent;
    variable_definition->dump(indent);
    cout << indent << "IN:" << endl;
    nested_value->dump("| " + indent);
}

const plugins::Type &LetNode::get_type(DecorateContext &context) const {
    const plugins::Type &variable_type = variable_definition->get_type(context);
    context.add_variable(variable_name, variable_type);
    const plugins::Type &nested_type = nested_value->get_type(context);
    context.remove_variable(variable_name);
    return nested_type;
}

FunctionCallNode::FunctionCallNode(const string &name,
                                   vector<ASTNodePtr> &&positional_arguments,
                                   unordered_map<string, ASTNodePtr> &&keyword_arguments,
                                   const string &unparsed_config)
    : name(name), positional_arguments(move(positional_arguments)),
      keyword_arguments(move(keyword_arguments)), unparsed_config(unparsed_config) {
}

static DecoratedASTNodePtr decorate_and_convert(
    const ASTNode &node, const plugins::Type &target_type, DecorateContext &context) {
    const plugins::Type &node_type = node.get_type(context);
    DecoratedASTNodePtr decorated_node = node.decorate(context);

    if (node_type != target_type) {
        utils::TraceBlock block(context, "Adding casting node");
        if (node_type.can_convert_into(target_type)) {
            return utils::make_unique_ptr<ConvertNode>(
                move(decorated_node), node_type, target_type);
        } else {
            ostringstream message;
            message << "Cannot convert from type '"
                    << node_type.name() << "' to type '" << target_type.name()
                    << "'" << endl;
            context.error(message.str());
        }
    }
    return decorated_node;
}

bool FunctionCallNode::collect_argument(
    const ASTNode &arg, const plugins::ArgumentInfo &arg_info,
    DecorateContext &context, CollectedArguments &arguments) const {
    string key = arg_info.key;
    if (arguments.count(key)) {
        return false;
    }

    DecoratedASTNodePtr decorated_arg = decorate_and_convert(
        arg, arg_info.type, context);

    if (arg_info.bounds.has_bound()) {
        DecoratedASTNodePtr decorated_min_node;
        {
            utils::TraceBlock block(context, "Handling lower bound");
            ASTNodePtr min_node = parse_ast_node(arg_info.bounds.min, context);
            decorated_min_node = decorate_and_convert(
                *min_node, arg_info.type, context);
        }
        DecoratedASTNodePtr decorated_max_node;
        {
            utils::TraceBlock block(context, "Handling upper bound");
            ASTNodePtr max_node = parse_ast_node(arg_info.bounds.max, context);
            decorated_max_node = decorate_and_convert(
                *max_node, arg_info.type, context);
        }
        decorated_arg = utils::make_unique_ptr<CheckBoundsNode>(
            move(decorated_arg), move(decorated_min_node), move(decorated_max_node));
    }
    FunctionArgument function_arg(key, move(decorated_arg), arg_info.lazy_construction);
    arguments.insert({key, move(function_arg)});
    return true;
}

void FunctionCallNode::collect_keyword_arguments(
    const vector<plugins::ArgumentInfo> &argument_infos,
    DecorateContext &context, CollectedArguments &arguments) const {
    unordered_map<string, plugins::ArgumentInfo> argument_infos_by_key;
    for (const plugins::ArgumentInfo &arg_info : argument_infos) {
        assert(!argument_infos_by_key.contains(arg_info.key));
        argument_infos_by_key.insert({arg_info.key, arg_info});
    }

    for (const auto &key_and_arg : keyword_arguments) {
        const string &key = key_and_arg.first;
        const ASTNode &arg = *key_and_arg.second;
        utils::TraceBlock block(context, "Checking the keyword argument '" + key + "'.");
        if (!argument_infos_by_key.count(key)) {
            vector<string> valid_keys = get_keys<string>(argument_infos_by_key);
            ostringstream message;
            message << "Unknown keyword argument: " << key << endl
                    << "Valid keyword arguments are: "
                    << utils::join(valid_keys, ", ") << endl;
            context.error(message.str());
        }
        const plugins::ArgumentInfo &arg_info = argument_infos_by_key.at(key);
        bool success = collect_argument(arg, arg_info, context, arguments);
        if (!success) {
            ABORT("Multiple keyword definitions using the same key '"
                  + key + "'. This should be impossible here because we "
                  + "sort by key earlier.");
        }
    }
}


/* This function has to be called *AFTER* collect_keyword_arguments. */
void FunctionCallNode::collect_positional_arguments(
    const vector<plugins::ArgumentInfo> &argument_infos,
    DecorateContext &context, CollectedArguments &arguments) const {
    // Check if too many arguments are specified for the plugin
    int num_pos_args = positional_arguments.size();
    int num_kwargs = keyword_arguments.size();
    if (num_pos_args + num_kwargs > static_cast<int>(argument_infos.size())) {
        vector<string> allowed_keys;
        allowed_keys.reserve(argument_infos.size());
        for (const auto &arg_info: argument_infos) {
            allowed_keys.push_back(arg_info.key);
        }
        vector<string> given_positional_keys;
        for (size_t i = 0; i < positional_arguments.size(); ++i) {
            if (i < argument_infos.size()) {
                given_positional_keys.push_back(argument_infos[i].key);
            } else {
                given_positional_keys.push_back("?");
            }
        }
        vector<string> given_keyword_keys = get_keys(keyword_arguments);

        ostringstream message;
        message << "Too many parameters specified!" << endl
                << "Allowed parameters: " << utils::join(allowed_keys, ", ")
                << endl
                << "Given positional parameters: "
                << utils::join(given_positional_keys, ", ") << endl
                << "Given keyword parameters: "
                << utils::join(given_keyword_keys, ", ") << endl;
        context.error(message.str());
    }

    for (int i = 0; i < num_pos_args; ++i) {
        const ASTNode &arg = *positional_arguments[i];
        const plugins::ArgumentInfo &arg_info = argument_infos[i];
        utils::TraceBlock block(
            context,
            "Checking the " + to_string(i + 1) +
            ". positional argument (" + arg_info.key + ")");
        bool success = collect_argument(arg, arg_info, context, arguments);
        if (!success) {
            ostringstream message;
            message << "The argument '" << arg_info.key
                    << "' is defined by the " << (i + 1)
                    << ". positional argument and by a keyword argument."
                    << endl;
            context.error(message.str());
        }
    }
}

/* This function has to be called *AFTER* collect_positional_arguments. */
void FunctionCallNode::collect_default_values(
    const vector<plugins::ArgumentInfo> &argument_infos,
    DecorateContext &context, CollectedArguments &arguments) const {
    for (const plugins::ArgumentInfo &arg_info : argument_infos) {
        const string &key = arg_info.key;
        if (!arguments.count(key)) {
            utils::TraceBlock block(context, "Checking the default for argument '" + key + "'.");
            if (arg_info.has_default()) {
                ASTNodePtr arg;
                {
                    utils::TraceBlock block(context, "Parsing default value");
                    arg = parse_ast_node(arg_info.default_value, context);
                }
                bool success = collect_argument(*arg, arg_info, context, arguments);
                if (!success) {
                    ABORT("Default argument for '" + key + "' set although "
                          + "value for keyword exists. This should be impossible.");
                }
            } else if (!arg_info.is_optional()) {
                context.error("Missing argument is mandatory!");
            }
        }
    }
}

DecoratedASTNodePtr FunctionCallNode::decorate(DecorateContext &context) const {
    utils::TraceBlock block(context, "Checking Plugin: " + name);
    const plugins::Registry &registry = context.get_registry();
    if (!registry.has_feature(name)) {
        context.error("Plugin '" + name + "' is not defined.");
    }
    shared_ptr<const plugins::Feature> feature = registry.get_feature(name);
    const vector<plugins::ArgumentInfo> &argument_infos = feature->get_arguments();

    CollectedArguments arguments_by_key;
    collect_keyword_arguments(argument_infos, context, arguments_by_key);
    collect_positional_arguments(argument_infos, context, arguments_by_key);
    collect_default_values(argument_infos, context, arguments_by_key);

    vector<FunctionArgument> arguments;
    for (auto &key_and_arg : arguments_by_key) {
        arguments.push_back(move(key_and_arg.second));
    }
    return utils::make_unique_ptr<DecoratedFunctionCallNode>(feature, move(arguments),
                                                             unparsed_config);
}

void FunctionCallNode::dump(string indent) const {
    cout << indent << "FUNC:" << name << endl;
    indent = "| " + indent;
    cout << indent << "POSITIONAL ARGS:" << endl;
    for (const ASTNodePtr &node : positional_arguments) {
        node->dump("| " + indent);
    }
    cout << indent << "KEYWORD ARGS:" << endl;
    for (const auto &pair : keyword_arguments) {
        cout << indent << pair.first << " = " << endl;
        pair.second->dump("| " + indent);
    }
}

const plugins::Type &FunctionCallNode::get_type(DecorateContext &context) const {
    const plugins::Registry &registry = context.get_registry();
    if (!registry.has_feature(name)) {
        context.error("No feature defined for FunctionCallNode '" + name + "'.");
    }
    const shared_ptr<const plugins::Feature> &feature = registry.get_feature(name);
    return feature->get_type();
}

ListNode::ListNode(vector<ASTNodePtr> &&elements)
    : elements(move(elements)) {
}

DecoratedASTNodePtr ListNode::decorate(DecorateContext &context) const {
    utils::TraceBlock block(context, "Checking list");
    vector<DecoratedASTNodePtr> decorated_elements;
    if (!elements.empty()) {
        const plugins::Type *common_element_type = get_common_element_type(context);
        if (!common_element_type) {
            vector<string> element_type_names;
            element_type_names.reserve(elements.size());
            for (const ASTNodePtr &element : elements) {
                const plugins::Type &element_type = element->get_type(context);
                element_type_names.push_back(element_type.name());
            }
            context.error("List contains elements of different types: ["
                          + utils::join(element_type_names, ", ") + "].");
        }
        for (size_t i = 0; i < elements.size(); i++) {
            utils::TraceBlock block(context, "Checking " + to_string(i) + ". element");
            const plugins::Type &element_type = elements[i]->get_type(context);
            DecoratedASTNodePtr decorated_element_node = elements[i]->decorate(context);
            if (element_type != *common_element_type) {
                assert(element_type.can_convert_into(*common_element_type));
                decorated_element_node = utils::make_unique_ptr<ConvertNode>(
                    move(decorated_element_node), element_type, *common_element_type);
            }
            decorated_elements.push_back(move(decorated_element_node));
        }
    }
    return utils::make_unique_ptr<DecoratedListNode>(move(decorated_elements));
}

void ListNode::dump(string indent) const {
    cout << indent << "LIST:" << endl;
    indent = "| " + indent;
    for (const ASTNodePtr &node : elements) {
        node->dump(indent);
    }
}

const plugins::Type *ListNode::get_common_element_type(
    DecorateContext &context) const {
    const plugins::Type *common_element_type = nullptr;
    for (const ASTNodePtr &element : elements) {
        const plugins::Type *element_type = &element->get_type(context);
        if ((!common_element_type)
            || (!element_type->can_convert_into(*common_element_type) &&
                common_element_type->can_convert_into(*element_type))) {
            common_element_type = element_type;
        } else if (!element_type->can_convert_into(*common_element_type)) {
            return nullptr;
        }
    }
    return common_element_type;
}

const plugins::Type &ListNode::get_type(DecorateContext &context) const {
    if (elements.empty()) {
        return plugins::TypeRegistry::EMPTY_LIST_TYPE;
    } else {
        const plugins::Type *element_type = get_common_element_type(context);
        if (!element_type)
            context.error("List elements cannot be converted to common type.");
        return plugins::TypeRegistry::instance()->create_list_type(*element_type);
    }
}

LiteralNode::LiteralNode(const Token &value)
    : value(value) {
}

DecoratedASTNodePtr LiteralNode::decorate(DecorateContext &context) const {
    utils::TraceBlock block(context, "Checking Literal: " + value.content);
    if (context.has_variable(value.content)) {
        if (value.type != TokenType::IDENTIFIER) {
            ABORT("A non-identifier token was defined as variable.");
        }
        string variable_name = value.content;
        return utils::make_unique_ptr<VariableNode>(variable_name);
    }

    switch (value.type) {
    case TokenType::BOOLEAN:
        return utils::make_unique_ptr<BoolLiteralNode>(value.content);
    case TokenType::STRING:
        return utils::make_unique_ptr<StringLiteralNode>(value.content);
    case TokenType::INTEGER:
        return utils::make_unique_ptr<IntLiteralNode>(value.content);
    case TokenType::FLOAT:
        return utils::make_unique_ptr<FloatLiteralNode>(value.content);
    case TokenType::IDENTIFIER:
        return utils::make_unique_ptr<SymbolNode>(value.content);
    default:
        ABORT("LiteralNode has unexpected token type '" +
              token_type_name(value.type) + "'.");
    }
}

void LiteralNode::dump(string indent) const {
    cout << indent << token_type_name(value.type) << ": "
         << value.content << endl;
}

const plugins::Type &LiteralNode::get_type(DecorateContext &context) const {
    switch (value.type) {
    case TokenType::BOOLEAN:
        return plugins::TypeRegistry::instance()->get_type<bool>();
    case TokenType::STRING:
        return plugins::TypeRegistry::instance()->get_type<string>();
    case TokenType::INTEGER:
        return plugins::TypeRegistry::instance()->get_type<int>();
    case TokenType::FLOAT:
        return plugins::TypeRegistry::instance()->get_type<double>();
    case TokenType::IDENTIFIER:
        if (context.has_variable(value.content)) {
            return context.get_variable_type(value.content);
        }
        return plugins::TypeRegistry::SYMBOL_TYPE;
    default:
        ABORT("LiteralNode has unexpected token type '" +
              token_type_name(value.type) + "'.");
    }
}
}
