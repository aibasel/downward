#ifndef SUCCESSOR_GENERATOR_INTERNALS_H
#define SUCCESSOR_GENERATOR_INTERNALS_H

#include "../operator_id.h"

#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

class GlobalState;
class State;

namespace successor_generator {
class GeneratorBase {
public:
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const = 0;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const = 0;
};

class GeneratorImmediate : public GeneratorBase {
    std::list<OperatorID> immediate_operators;
    std::unique_ptr<GeneratorBase> next_generator;
public:
    GeneratorImmediate(std::list<OperatorID> &&immediate_operators,
                       std::unique_ptr<GeneratorBase> next_generator);
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const override;
};

class GeneratorFork : public GeneratorBase {
    std::unique_ptr<GeneratorBase> generator1;
    std::unique_ptr<GeneratorBase> generator2;
public:
    GeneratorFork(
        std::unique_ptr<GeneratorBase> generator1,
        std::unique_ptr<GeneratorBase> generator2);
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const override;
};

class GeneratorSwitchVector : public GeneratorBase {
    int switch_var_id;
    std::vector<std::unique_ptr<GeneratorBase>> generator_for_value;
public:
    GeneratorSwitchVector(
        int switch_var_id,
        std::vector<std::unique_ptr<GeneratorBase>> &&generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const override;
};

class GeneratorSwitchHash : public GeneratorBase {
    int switch_var_id;
    std::unordered_map<int, std::unique_ptr<GeneratorBase>> generator_for_value;
public:
    GeneratorSwitchHash(
        int switch_var_id,
        std::unordered_map<int, std::unique_ptr<GeneratorBase>> &&generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const override;
};

class GeneratorSwitchSingle : public GeneratorBase {
    int switch_var_id;
    int value;
    std::unique_ptr<GeneratorBase> generator_for_value;
public:
    GeneratorSwitchSingle(
        int switch_var_id, int value,
        std::unique_ptr<GeneratorBase> generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const override;
};

class GeneratorLeafList : public GeneratorBase {
    std::list<OperatorID> applicable_operators;
public:
    GeneratorLeafList(std::list<OperatorID> &&applicable_operators);
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const override;
};

class GeneratorLeafSingle : public GeneratorBase {
    OperatorID applicable_operator;
public:
    GeneratorLeafSingle(OperatorID applicable_operator);
    virtual void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const override;
};
}

#endif
