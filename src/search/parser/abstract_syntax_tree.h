#ifndef PARSER_ABSTRACT_SYNTAX_TREE_H
#define PARSER_ABSTRACT_SYNTAX_TREE_H

#include "decorated_abstract_syntax_tree.h"
#include "token_stream.h"

#include <cassert>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace plugins {
struct ArgumentInfo;
class Type;
}

namespace parser {
class DecorateContext;

class ASTNode {
public:
    virtual ~ASTNode() = default;

    DecoratedASTNodePtr decorate() const;
    virtual DecoratedASTNodePtr decorate(DecorateContext &context) const = 0;
    virtual void dump(std::string indent = "+") const = 0;
    virtual const plugins::Type &get_type(DecorateContext &context) const = 0;
};
using ASTNodePtr = std::unique_ptr<ASTNode>;

class LetNode : public ASTNode {
    std::string variable_name;
    ASTNodePtr variable_definition;
    ASTNodePtr nested_value;
public:
    LetNode(const std::string &variable_name, ASTNodePtr variable_definition,
            ASTNodePtr nested_value);
    DecoratedASTNodePtr decorate(DecorateContext &context) const override;
    void dump(std::string indent) const override;
    const plugins::Type &get_type(DecorateContext &context) const override;
};

class FunctionCallNode : public ASTNode {
    std::string name;
    std::vector<ASTNodePtr> positional_arguments;
    std::unordered_map<std::string, ASTNodePtr> keyword_arguments;
    std::string unparsed_config;

    using CollectedArguments = std::unordered_map<std::string, FunctionArgument>;
    bool collect_argument(
        const ASTNode &arg,
        const plugins::ArgumentInfo &arg_info,
        DecorateContext &context,
        CollectedArguments &arguments) const;
    void collect_keyword_arguments(
        const std::vector<plugins::ArgumentInfo> &argument_infos,
        DecorateContext &context,
        CollectedArguments &arguments) const;
    void collect_positional_arguments(
        const std::vector<plugins::ArgumentInfo> &argument_infos,
        DecorateContext &context,
        CollectedArguments &arguments) const;
    void collect_default_values(
        const std::vector<plugins::ArgumentInfo> &argument_infos,
        DecorateContext &context,
        CollectedArguments &arguments) const;
public:
    FunctionCallNode(const std::string &name,
                     std::vector<ASTNodePtr> &&positional_arguments,
                     std::unordered_map<std::string, ASTNodePtr> &&keyword_arguments,
                     const std::string &unparsed_config);
    DecoratedASTNodePtr decorate(DecorateContext &context) const override;
    void dump(std::string indent) const override;
    const plugins::Type &get_type(DecorateContext &context) const override;
};

class ListNode : public ASTNode {
    std::vector<ASTNodePtr> elements;
public:
    explicit ListNode(std::vector<ASTNodePtr> &&elements);
    DecoratedASTNodePtr decorate(DecorateContext &context) const override;
    void dump(std::string indent) const override;
    const plugins::Type *get_common_element_type(DecorateContext &context) const;
    const plugins::Type &get_type(DecorateContext &context) const override;
};

class LiteralNode : public ASTNode {
    Token value;
public:
    explicit LiteralNode(const Token &value);
    DecoratedASTNodePtr decorate(DecorateContext &context) const override;
    void dump(std::string indent) const override;
    const plugins::Type &get_type(DecorateContext &context) const override;
};
}
#endif
