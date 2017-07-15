#ifndef SUCCESSOR_GENERATOR_INTERNALS_H
#define SUCCESSOR_GENERATOR_INTERNALS_H

#include "../operator_id.h"

#include <cmath>
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

template<typename T>
int estimate_vector_size(int num_elements) {
    int size = 0;
    size += 2 * sizeof(void *);       // overhead for dynamic memory management
    size += sizeof(std::vector<T>);   // size of empty vector
    size += num_elements * sizeof(T); // size of actual entries
    return size;
}

template<typename Key, typename Value>
int estimate_unordered_map_size(int num_entries) {
    // See issue705 for a discussion of this estimation.
    int num_buckets;
    if (num_entries < 2) {
        num_buckets = 2;
    } else if (num_entries < 5) {
        num_buckets = 5;
    } else if (num_entries < 11) {
        num_buckets = 11;
    } else if (num_entries < 23) {
        num_buckets = 23;
    } else if (num_entries < 47) {
        num_buckets = 47;
    } else if (num_entries < 97) {
        num_buckets = 97;
    } else {
        int n = std::log2((num_entries + 1) / 3);
        num_buckets = 3 * std::pow(2, n + 1) - 1;
    }

    int size = 0;
    size += 2 * sizeof(void *);                            // overhead for dynamic memory management
    size += sizeof(std::unordered_map<Key, Value>);        // empty map
    size += num_entries * sizeof(std::pair<Key, Value>);   // actual entries
    size += num_entries * sizeof(std::pair<Key, Value> *); // pointer to values
    size += num_entries * sizeof(void *);                  // pointer to next node
    size += num_buckets * sizeof(void *);                  // pointer to next bucket
    return size;
}

}

#endif
