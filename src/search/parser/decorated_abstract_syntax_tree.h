#ifndef PARSER_DECORATED_ABSTRACT_SYNTAX_TREE_H
#define PARSER_DECORATED_ABSTRACT_SYNTAX_TREE_H

#include "../plugins/any.h"
#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <memory>
#include <string>
#include <vector>

namespace plugins {
class Options;
}

namespace parser {
// TODO: if we can get rid of lazy values, this class could be moved to the cc file.
class ConstructContext : public utils::Context {
    std::unordered_map<std::string, plugins::Any> variables;
public:
    void set_variable(const std::string &name, const plugins::Any &value);
    void remove_variable(const std::string &name);
    bool has_variable(const std::string &name) const;
    plugins::Any get_variable(const std::string &name) const;
};

class DecoratedASTNode {
public:
    virtual ~DecoratedASTNode() = default;
    plugins::Any construct() const;
    virtual plugins::Any construct(ConstructContext &context) const = 0;
    virtual void dump(std::string indent = "+") const = 0;

    // TODO: This is here only for the iterated search. Once we switch to builders, we won't need it any more.
    virtual std::unique_ptr<DecoratedASTNode> clone() const = 0;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const = 0;
};
using DecoratedASTNodePtr = std::unique_ptr<DecoratedASTNode>;

class LazyValue {
    ConstructContext context;
    DecoratedASTNodePtr node;
    plugins::Any construct_any() const;

public:
    LazyValue(const DecoratedASTNode &node, const ConstructContext &context);
    LazyValue(const LazyValue &other);

    template<typename T>
    T construct() const {
        plugins::Any constructed = construct_any();
        return plugins::OptionsAnyCaster<T>::cast(constructed);
    }

    std::vector<LazyValue> construct_lazy_list();
};

class FunctionArgument {
    std::string key;
    DecoratedASTNodePtr value;

    // TODO: This is here only for the iterated search. Once we switch to builders, we won't need it any more.
    bool lazy_construction;
public:
    FunctionArgument(const std::string &key, DecoratedASTNodePtr value,
                     bool lazy_construction);

    std::string get_key() const;
    const DecoratedASTNode &get_value() const;
    void dump(std::string indent) const;

    // TODO: This is here only for the iterated search. Once we switch to builders, we won't need it any more.
    bool is_lazily_constructed() const;
    FunctionArgument(const FunctionArgument &other);
};

class DecoratedLetNode : public DecoratedASTNode {
    std::string variable_name;
    DecoratedASTNodePtr variable_definition;
    DecoratedASTNodePtr nested_value;
public:
    DecoratedLetNode(
        const std::string &variable_name,
        DecoratedASTNodePtr variable_definition,
        DecoratedASTNodePtr nested_value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    DecoratedLetNode(const DecoratedLetNode &other);
};

class DecoratedFunctionCallNode : public DecoratedASTNode {
    std::shared_ptr<const plugins::Feature> feature;
    std::vector<FunctionArgument> arguments;
    std::string unparsed_config;
public:
    DecoratedFunctionCallNode(
        std::shared_ptr<const plugins::Feature> feature,
        std::vector<FunctionArgument> &&arguments,
        const std::string &unparsed_config);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    DecoratedFunctionCallNode(const DecoratedFunctionCallNode &other);
};

class DecoratedListNode : public DecoratedASTNode {
    std::vector<DecoratedASTNodePtr> elements;
public:
    DecoratedListNode(std::vector<DecoratedASTNodePtr> &&elements);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    DecoratedListNode(const DecoratedListNode &other);
    const std::vector<DecoratedASTNodePtr> &get_elements() const {
        return elements;
    }
};

class VariableNode : public DecoratedASTNode {
    std::string name;
public:
    VariableNode(const std::string &name);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    VariableNode(const VariableNode &other);
};

class BoolLiteralNode : public DecoratedASTNode {
    std::string value;
public:
    BoolLiteralNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    BoolLiteralNode(const BoolLiteralNode &other);
};

class IntLiteralNode : public DecoratedASTNode {
    std::string value;
public:
    IntLiteralNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    IntLiteralNode(const IntLiteralNode &other);
};

class FloatLiteralNode : public DecoratedASTNode {
    std::string value;
public:
    FloatLiteralNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    FloatLiteralNode(const FloatLiteralNode &other);
};

class SymbolNode : public DecoratedASTNode {
    std::string value;
public:
    SymbolNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    SymbolNode(const SymbolNode &other);
};

class ConvertNode : public DecoratedASTNode {
    DecoratedASTNodePtr value;
    const plugins::Type &from_type;
    const plugins::Type &to_type;
public:
    ConvertNode(DecoratedASTNodePtr value,
                const plugins::Type &from_type,
                const plugins::Type &to_type);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    ConvertNode(const ConvertNode &other);
};

class CheckBoundsNode : public DecoratedASTNode {
    DecoratedASTNodePtr value;
    DecoratedASTNodePtr min_value;
    DecoratedASTNodePtr max_value;
public:
    CheckBoundsNode(DecoratedASTNodePtr value, DecoratedASTNodePtr min_value,
                    DecoratedASTNodePtr max_value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;

    // TODO: once we get rid of lazy construction, this should no longer be necessary.
    virtual std::unique_ptr<DecoratedASTNode> clone() const override;
    virtual std::shared_ptr<DecoratedASTNode> clone_shared() const override;
    CheckBoundsNode(const CheckBoundsNode &other);
};
}
#endif
