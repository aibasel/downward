#include "decorated_abstract_syntax_tree.h"

#include "../plugins/options.h"
#include "../plugins/types.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/memory.h"

#include <limits>

using namespace std;

namespace parser {
void ConstructContext::set_variable(const string &name, const plugins::Any &value) {
    variables[name] = value;
}

void ConstructContext::remove_variable(const string &name) {
    variables.erase(name);
}

bool ConstructContext::has_variable(const string &name) const {
    return variables.count(name);
}

plugins::Any ConstructContext::get_variable(const string &name) const {
    plugins::Any variable = variables.at(name);
    return variable;
}

LazyValue::LazyValue(const DecoratedASTNode &node, const ConstructContext &context)
    : context(context), node(node.clone()) {
}

LazyValue::LazyValue(const LazyValue &other)
    : context(other.context), node(other.node->clone()) {
}

plugins::Any LazyValue::construct_any() const {
    ConstructContext clean_context = context;
    utils::TraceBlock block(clean_context, "Delayed construction of LazyValue");
    return node->construct(clean_context);
}

vector<LazyValue> LazyValue::construct_lazy_list() {
    utils::TraceBlock block(context, "Delayed construction of a list");
    const DecoratedListNode *list_node =
        dynamic_cast<const DecoratedListNode *>(node.get());
    if (!list_node) {
        context.error(
            "Delayed construction of a list failed because the parsed element "
            "was no list.");
    }

    vector<LazyValue> elements;
    elements.reserve(list_node->get_elements().size());
    int elem = 1;
    for (const DecoratedASTNodePtr &element : list_node->get_elements()) {
        utils::TraceBlock(context,
                          "Create LazyValue for " + to_string(elem) +
                          ". list element");
        elements.emplace_back(LazyValue(*element, context));
        elem++;
    }
    return elements;
}

plugins::Any DecoratedASTNode::construct() const {
    ConstructContext context;
    utils::TraceBlock block(context, "Constructing parsed object");
    return construct(context);
}

FunctionArgument::FunctionArgument(const string &key, DecoratedASTNodePtr value,
                                   bool lazy_construction)
    : key(key), value(move(value)), lazy_construction(lazy_construction) {
}

string FunctionArgument::get_key() const {
    return key;
}

const DecoratedASTNode &FunctionArgument::get_value() const {
    return *value;
}

void FunctionArgument::dump(string indent) const {
    cout << indent << key << " = " << endl;
    value->dump("| " + indent);
}

bool FunctionArgument::is_lazily_constructed() const {
    return lazy_construction;
}

DecoratedLetNode::DecoratedLetNode(
    const string &variable_name,
    DecoratedASTNodePtr variable_definition,
    DecoratedASTNodePtr nested_value)
    : variable_name(variable_name),
      variable_definition(move(variable_definition)),
      nested_value(move(nested_value)) {
}

plugins::Any DecoratedLetNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing let-expression");
    plugins::Any variable_value;
    {
        utils::TraceBlock block(context, "Constructing variable '" + variable_name + "'");
        variable_value = variable_definition->construct(context);
    }
    plugins::Any result;
    {
        utils::TraceBlock block(context, "Constructing nested value");
        context.set_variable(variable_name, variable_value);
        result = nested_value->construct(context);
        context.remove_variable(variable_name);
    }
    return result;
}

void DecoratedLetNode::dump(string indent) const {
    cout << indent << "LET:" << variable_name << " = " << endl;
    indent = "| " + indent;
    variable_definition->dump(indent);
    cout << indent << "IN:" << endl;
    nested_value->dump("| " + indent);
}

DecoratedFunctionCallNode::DecoratedFunctionCallNode(
    shared_ptr<const plugins::Feature> feature, vector<FunctionArgument> &&arguments,
    const string &unparsed_config)
    : feature(feature), arguments(move(arguments)), unparsed_config(unparsed_config) {
}

plugins::Any DecoratedFunctionCallNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing feature '" + feature->get_key() + "': " +
                            unparsed_config);
    plugins::Options opts;
    opts.set_unparsed_config(unparsed_config);
    for (const FunctionArgument &arg : arguments) {
        utils::TraceBlock block(context, "Constructing argument '" + arg.get_key() + "'");
        if (arg.is_lazily_constructed()) {
            opts.set(arg.get_key(), LazyValue(arg.get_value(), context));
        } else {
            opts.set(arg.get_key(), arg.get_value().construct(context));
        }
    }
    return feature->construct(opts, context);
}

void DecoratedFunctionCallNode::dump(string indent) const {
    cout << indent << "FUNC:" << feature->get_title()
         << " (returns " << feature->get_type().name() << ")" << endl;
    indent = "| " + indent;
    cout << indent << "ARGUMENTS:" << endl;
    for (const FunctionArgument &arg : arguments) {
        arg.dump("| " + indent);
    }
}

DecoratedListNode::DecoratedListNode(vector<DecoratedASTNodePtr> &&elements)
    : elements(move(elements)) {
}

plugins::Any DecoratedListNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing list");
    vector<plugins::Any> result;
    int i = 0;
    for (const DecoratedASTNodePtr &element : elements) {
        utils::TraceBlock block(context, "Constructing element " + to_string(i));
        result.push_back(element->construct(context));
        ++i;
    }
    return result;
}

void DecoratedListNode::dump(string indent) const {
    cout << indent << "LIST:" << endl;
    indent = "| " + indent;
    for (const DecoratedASTNodePtr &element : elements) {
        element->dump(indent);
    }
}

VariableNode::VariableNode(const string &name)
    : name(name) {
}

plugins::Any VariableNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Looking up variable '" + name + "'");
    if (!context.has_variable(name)) {
        context.error("Variable '" + name + "' is not defined.");
    }
    return context.get_variable(name);
}

void VariableNode::dump(string indent) const {
    cout << indent << "VAR: " << name << endl;
}

BoolLiteralNode::BoolLiteralNode(const string &value)
    : value(value) {
}

plugins::Any BoolLiteralNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing bool value from '" + value + "'");
    istringstream stream(value);
    bool x;
    if ((stream >> boolalpha >> x).fail()) {
        ABORT("Could not parse bool constant '" + value + "'"
              " (this should have been caught before constructing this node).");
    }
    return x;
}

void BoolLiteralNode::dump(string indent) const {
    cout << indent << "BOOL: " << value << endl;
}

IntLiteralNode::IntLiteralNode(const string &value)
    : value(value) {
}

plugins::Any IntLiteralNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing int value from '" + value + "'");
    if (value.empty()) {
        ABORT("Empty value in int constant '" + value + "'"
              " (this should have been caught before constructing this node).");
    } else if (value == "infinity") {
        return numeric_limits<int>::max();
    }

    char suffix = value.back();
    string prefix = value;
    int factor = 1;
    if (isalpha(suffix)) {
        suffix = static_cast<char>(tolower(suffix));
        if (suffix == 'k') {
            factor = 1000;
        } else if (suffix == 'm') {
            factor = 1000000;
        } else if (suffix == 'g') {
            factor = 1000000000;
        } else {
            ABORT("Invalid suffix in int constant '" + value + "'"
                  " (this should have been caught before constructing this node).");
        }
        prefix.pop_back();
    }

    istringstream stream(prefix);
    int x;
    stream >> noskipws >> x;
    if (stream.fail() || !stream.eof()) {
        ABORT("Could not parse int constant '" + value + "'"
              " (this should have been caught before constructing this node).");
    }

    int min_int = numeric_limits<int>::min();
    // Reserve highest value for "infinity".
    int max_int = numeric_limits<int>::max() - 1;
    if (!utils::is_product_within_limits(x, factor, min_int, max_int)) {
        context.error(
            "Absolute value of integer constant too large: '" + value + "'");
    }
    return x * factor;
}

void IntLiteralNode::dump(string indent) const {
    cout << indent << "INT: " << value << endl;
}

FloatLiteralNode::FloatLiteralNode(const string &value)
    : value(value) {
}

plugins::Any FloatLiteralNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing float value from '" + value + "'");
    if (value == "infinity") {
        return numeric_limits<double>::infinity();
    } else {
        istringstream stream(value);
        double x;
        stream >> noskipws >> x;
        if (stream.fail() || !stream.eof()) {
            ABORT("Could not parse double constant '" + value + "'"
                  " (this should have been caught before constructing this node).");
        }
        return x;
    }
}

void FloatLiteralNode::dump(string indent) const {
    cout << indent << "FLOAT: " << value << endl;
}

SymbolNode::SymbolNode(const string &value)
    : value(value) {
}

plugins::Any SymbolNode::construct(ConstructContext &) const {
    return plugins::Any(value);
}

void SymbolNode::dump(string indent) const {
    cout << indent << "SYMBOL: " << value << endl;
}

ConvertNode::ConvertNode(
    DecoratedASTNodePtr value, const plugins::Type &from_type,
    const plugins::Type &to_type)
    : value(move(value)), from_type(from_type), to_type(to_type) {
}

plugins::Any ConvertNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing value that requires conversion");
    plugins::Any constructed_value;
    {
        utils::TraceBlock block(
            context, "Constructing value of type '" + from_type.name() + "'");
        constructed_value = value->construct(context);
    }
    plugins::Any converted_value;
    {
        utils::TraceBlock block(context, "Converting constructed value from '" + from_type.name() +
                                "' to '" + to_type.name() + "'");
        converted_value = plugins::convert(constructed_value, from_type,
                                           to_type, context);
    }
    return converted_value;
}

void ConvertNode::dump(string indent) const {
    cout << indent << "CONVERT: "
         << from_type.name() << " to " << to_type.name() << endl;
    value->dump("| " + indent);
}

CheckBoundsNode::CheckBoundsNode(
    DecoratedASTNodePtr value, DecoratedASTNodePtr min_value, DecoratedASTNodePtr max_value)
    : value(move(value)), min_value(move(min_value)), max_value(move(max_value)) {
}

template<typename T>
static bool satisfies_bounds(const plugins::Any &v_, const plugins::Any &min_,
                             const plugins::Any &max_) {
    T v = plugins::any_cast<T>(v_);
    T min = plugins::any_cast<T>(min_);
    T max = plugins::any_cast<T>(max_);
    return (min <= v) && (v <= max);
}

plugins::Any CheckBoundsNode::construct(ConstructContext &context) const {
    utils::TraceBlock block(context, "Constructing value with bounds");
    plugins::Any v;
    {
        utils::TraceBlock block(context, "Constructing value");
        v = value->construct(context);
    }
    plugins::Any min;
    {
        utils::TraceBlock block(context, "Constructing lower bound");
        min = min_value->construct(context);
    }
    plugins::Any max;
    {
        utils::TraceBlock block(context, "Constructing upper bound");
        max = max_value->construct(context);
    }
    {
        utils::TraceBlock block(context, "Checking bounds");
        const type_info &type = v.type();
        if (min.type() != type || max.type() != type) {
            ABORT("Types of bounds (" +
                  string(min.type().name()) + ", " + max.type().name() +
                  ") do not match type of value (" + type.name() + ")" +
                  " (this should have been caught before constructing this node).");
        }

        bool bounds_satisfied = true;
        if (type == typeid(int)) {
            bounds_satisfied = satisfies_bounds<int>(v, min, max);
        } else if (type == typeid(double)) {
            bounds_satisfied = satisfies_bounds<double>(v, min, max);
        } else {
            ABORT("Bounds are only supported for arguments of type int or double.");
        }
        if (!bounds_satisfied) {
            context.error("Value is not in bounds.");
        }
    }
    return v;
}

void CheckBoundsNode::dump(string indent) const {
    cout << indent << "CHECK-BOUNDS: " << endl;
    value->dump("| " + indent);
    min_value->dump("| " + indent);
    max_value->dump("| " + indent);
}

// We are keeping all copy functionality together because it should be removed soon.
FunctionArgument::FunctionArgument(const FunctionArgument &other)
    : key(other.key), value(other.value->clone()),
      lazy_construction(other.lazy_construction) {
}

DecoratedLetNode::DecoratedLetNode(const DecoratedLetNode &other)
    : variable_name(other.variable_name),
      variable_definition(other.variable_definition->clone()),
      nested_value(other.nested_value->clone()) {
}

shared_ptr<DecoratedASTNode> DecoratedLetNode::clone_shared() const {
    return make_shared<DecoratedLetNode>(*this);
}

unique_ptr<DecoratedASTNode> DecoratedLetNode::clone() const {
    return utils::make_unique_ptr<DecoratedLetNode>(*this);
}

DecoratedFunctionCallNode::DecoratedFunctionCallNode(
    const DecoratedFunctionCallNode &other)
    : feature(other.feature), arguments(other.arguments),
      unparsed_config(other.unparsed_config) {
}

shared_ptr<DecoratedASTNode> DecoratedFunctionCallNode::clone_shared() const {
    return make_shared<DecoratedFunctionCallNode>(*this);
}

unique_ptr<DecoratedASTNode> DecoratedFunctionCallNode::clone() const {
    return utils::make_unique_ptr<DecoratedFunctionCallNode>(*this);
}

DecoratedListNode::DecoratedListNode(const DecoratedListNode &other) {
    elements.reserve(other.elements.size());
    for (const DecoratedASTNodePtr &element : other.elements) {
        elements.push_back(element->clone());
    }
}

unique_ptr<DecoratedASTNode> DecoratedListNode::clone() const {
    return utils::make_unique_ptr<DecoratedListNode>(*this);
}

shared_ptr<DecoratedASTNode> DecoratedListNode::clone_shared() const {
    return make_shared<DecoratedListNode>(*this);
}

VariableNode::VariableNode(const VariableNode &other)
    : name(other.name) {
}

unique_ptr<DecoratedASTNode> VariableNode::clone() const {
    return utils::make_unique_ptr<VariableNode>(*this);
}

shared_ptr<DecoratedASTNode> VariableNode::clone_shared() const {
    return make_shared<VariableNode>(*this);
}

BoolLiteralNode::BoolLiteralNode(const BoolLiteralNode &other)
    : value(other.value) {
}

unique_ptr<DecoratedASTNode> BoolLiteralNode::clone() const {
    return utils::make_unique_ptr<BoolLiteralNode>(*this);
}

shared_ptr<DecoratedASTNode> BoolLiteralNode::clone_shared() const {
    return make_shared<BoolLiteralNode>(*this);
}

IntLiteralNode::IntLiteralNode(const IntLiteralNode &other)
    : value(other.value) {
}

unique_ptr<DecoratedASTNode> IntLiteralNode::clone() const {
    return utils::make_unique_ptr<IntLiteralNode>(*this);
}

shared_ptr<DecoratedASTNode> IntLiteralNode::clone_shared() const {
    return make_shared<IntLiteralNode>(*this);
}

FloatLiteralNode::FloatLiteralNode(const FloatLiteralNode &other)
    : value(other.value) {
}

unique_ptr<DecoratedASTNode> FloatLiteralNode::clone() const {
    return utils::make_unique_ptr<FloatLiteralNode>(*this);
}

shared_ptr<DecoratedASTNode> FloatLiteralNode::clone_shared() const {
    return make_shared<FloatLiteralNode>(*this);
}

SymbolNode::SymbolNode(const SymbolNode &other)
    : value(other.value) {
}

unique_ptr<DecoratedASTNode> SymbolNode::clone() const {
    return utils::make_unique_ptr<SymbolNode>(*this);
}

shared_ptr<DecoratedASTNode> SymbolNode::clone_shared() const {
    return make_shared<SymbolNode>(*this);
}

ConvertNode::ConvertNode(const ConvertNode &other)
    : value(other.value->clone()), from_type(other.from_type),
      to_type(other.to_type) {
}

unique_ptr<DecoratedASTNode> ConvertNode::clone() const {
    return utils::make_unique_ptr<ConvertNode>(*this);
}

shared_ptr<DecoratedASTNode> ConvertNode::clone_shared() const {
    return make_shared<ConvertNode>(*this);
}

CheckBoundsNode::CheckBoundsNode(const CheckBoundsNode &other)
    : value(other.value->clone()), min_value(other.min_value->clone()),
      max_value(other.max_value->clone()) {
}

unique_ptr<DecoratedASTNode> CheckBoundsNode::clone() const {
    return utils::make_unique_ptr<CheckBoundsNode>(*this);
}

shared_ptr<DecoratedASTNode> CheckBoundsNode::clone_shared() const {
    return make_shared<CheckBoundsNode>(*this);
}
}
