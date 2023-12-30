#ifndef PARSER_DECORATED_ABSTRACT_SYNTAX_TREE_H
#define PARSER_DECORATED_ABSTRACT_SYNTAX_TREE_H

#include "../plugins/any.h"
#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <memory>
#include <string>
#include <vector>

namespace parser {
class ConstructContext;

class DecoratedASTNode {
public:
    virtual ~DecoratedASTNode() = default;
    plugins::Any construct() const;
    virtual plugins::Any construct(ConstructContext &context) const = 0;
    virtual void dump(std::string indent = "+") const = 0;
};
using DecoratedASTNodePtr = std::unique_ptr<DecoratedASTNode>;



class FunctionArgument {
    std::string key;
    DecoratedASTNodePtr value;
public:
    FunctionArgument(const std::string &key, DecoratedASTNodePtr value);

    std::string get_key() const;
    const DecoratedASTNode &get_value() const;
    void dump(const std::string &indent) const;
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
};

class DecoratedFunctionCallNode : public DecoratedASTNode {
    std::shared_ptr<const plugins::Feature> feature;
    std::vector<FunctionArgument> arguments;
    std::string unparsed_config;
public:
    DecoratedFunctionCallNode(
        const std::shared_ptr<const plugins::Feature> &feature,
        std::vector<FunctionArgument> &&arguments,
        const std::string &unparsed_config);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;
};

class DecoratedListNode : public DecoratedASTNode {
    std::vector<DecoratedASTNodePtr> elements;
public:
    DecoratedListNode(std::vector<DecoratedASTNodePtr> &&elements);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;
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
};

class BoolLiteralNode : public DecoratedASTNode {
    std::string value;
public:
    BoolLiteralNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;
};

class StringLiteralNode : public DecoratedASTNode {
    std::string value;
public:
    StringLiteralNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;
};

class IntLiteralNode : public DecoratedASTNode {
    std::string value;
public:
    IntLiteralNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;
};

class FloatLiteralNode : public DecoratedASTNode {
    std::string value;
public:
    FloatLiteralNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;
};

class SymbolNode : public DecoratedASTNode {
    std::string value;
public:
    SymbolNode(const std::string &value);

    plugins::Any construct(ConstructContext &context) const override;
    void dump(std::string indent) const override;
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
};
}
#endif
