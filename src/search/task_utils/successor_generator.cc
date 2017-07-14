#include "successor_generator.h"

#include "../global_state.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <list>
#include <memory>
#include <unordered_map>

using namespace std;

/* Notes on possible optimizations:
   Allocators
   * Using specialized allocators (e.g. an arena allocator) could improve
     cache locality and reduce the memory management overhead. We could then
     also use indices instead of pointers to reduce the overhead in 64-bit
     builds.

   Switch nodes
   * For small numbers of entries (2-3) it could be worth it
     to store a list/vector of (value, generator) tuples and do a linear scan
     instead of using a hash map.

   Immediate and leaf nodes
   * Using forward_list instead of list led to a further 10% speedup on the
     largest Logistics instance, logistics-98/prob28.pddl, when we tested this
     (way back when). It would of course also reduce memory usage.
   * We could also experiment with other types (e.g. vector) to see if they
     perform better.
   * Analogously to GeneratorSwitchSingle and GeneratorLeafSingle, we could
     add GeneratorImmediateSingle.
*/

namespace successor_generator {
using Condition = vector<FactPair>;

bool smaller_variable_id(const FactPair &f1, const FactPair &f2) {
    return f1.var < f2.var;
}

template<typename T>
int estimate_vector_size(int num_elements) {
    int size = 0;
    size += 2 * sizeof(void *);       // overhead for dynamic memory management
    size += sizeof(vector<T>);        // size of empty vector
    size += num_elements * sizeof(T); // size of actual entries
    return size;
}

template<typename Key, typename Value>
int estimate_unordered_map_size(int num_entries) {
    // See issue705 for a discussion of this estimation.
    int num_buckets = 2;
    if (num_entries < 5) {
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
        int n = log2((num_entries + 1) / 3);
        num_buckets = 3 * pow(2, n + 1) - 1;
    }

    int size = 0;
    size += 2 * sizeof(void *);                       // overhead for dynamic memory management
    size += sizeof(unordered_map<Key, Value>);        // empty map
    size += num_entries * sizeof(pair<Key, Value>);   // actual entries
    size += num_entries * sizeof(pair<Key, Value> *); // pointer to values
    size += num_entries * sizeof(void *);             // pointer to next node
    size += num_buckets * sizeof(void *);             // pointer to next bucket
    return size;
}

class GeneratorBase {
public:
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const = 0;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const = 0;
};

class GeneratorImmediate : public GeneratorBase {
    list<OperatorID> immediate_operators;
    unique_ptr<GeneratorBase> next_generator;
public:
    GeneratorImmediate(list<OperatorID> &&immediate_operators,
                       unique_ptr<GeneratorBase> next_generator);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const override;
};

class GeneratorFork : public GeneratorBase {
    unique_ptr<GeneratorBase> generator1;
    unique_ptr<GeneratorBase> generator2;
public:
    GeneratorFork(
        unique_ptr<GeneratorBase> generator1,
        unique_ptr<GeneratorBase> generator2);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const override;
};

class GeneratorSwitchVector : public GeneratorBase {
    int switch_var_id;
    vector<unique_ptr<GeneratorBase>> generator_for_value;
public:
    GeneratorSwitchVector(
        int switch_var_id,
        vector<unique_ptr<GeneratorBase>> &&generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const override;
};

class GeneratorSwitchHash : public GeneratorBase {
    int switch_var_id;
    unordered_map<int, unique_ptr<GeneratorBase>> generator_for_value;
public:
    GeneratorSwitchHash(
        int switch_var_id,
        unordered_map<int, unique_ptr<GeneratorBase>> &&generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const override;
};

class GeneratorSwitchSingle : public GeneratorBase {
    int switch_var_id;
    int value;
    unique_ptr<GeneratorBase> generator_for_value;
public:
    GeneratorSwitchSingle(
        int switch_var_id, int value,
        unique_ptr<GeneratorBase> generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const override;
};

class GeneratorLeafList : public GeneratorBase {
    list<OperatorID> applicable_operators;
public:
    GeneratorLeafList(list<OperatorID> &&applicable_operators);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const override;
};

class GeneratorLeafSingle : public GeneratorBase {
    OperatorID applicable_operator;
public:
    GeneratorLeafSingle(OperatorID applicable_operator);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const override;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const override;
};


/*
  TODO: Reintegrate this code, or split it up properly into multiple
  files (e.g. successor_generator.{h,cc},
  successor_generator_details.{h,cc},
  successor_generator_factory.{h,cc}).
*/
#include "successor_generator_factory.cc"


GeneratorImmediate::GeneratorImmediate(
    list<OperatorID> &&immediate_operators,
    unique_ptr<GeneratorBase> next_generator_)
    : immediate_operators(move(immediate_operators)),
      next_generator(move(next_generator_)) {
    /* There is no reason to to use GeneratorImmediate if there is no next generator.
       Use GeneratorLeaf instead in such situtations. */
    assert(next_generator);
}

void GeneratorImmediate::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    /* A loop over push_back is faster than using insert in this situation
       because the lists are typically very small. We measured this in issue688. */
    for (OperatorID id : immediate_operators) {
        applicable_ops.push_back(id);
    }
    next_generator->generate_applicable_ops(state, applicable_ops);
}

void GeneratorImmediate::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    // See above for the reason for using push_back instead of insert.
    for (OperatorID id : immediate_operators) {
        applicable_ops.push_back(id);
    }
    next_generator->generate_applicable_ops(state, applicable_ops);
}

GeneratorFork::GeneratorFork(
    unique_ptr<GeneratorBase> generator1_,
    unique_ptr<GeneratorBase> generator2_)
    : generator1(move(generator1_)),
      generator2(move(generator2_)) {
    /* There is no reason to use a fork if only one of the generators exists.
       Use the existing generator directly if one of them exists or a nullptr
       otherwise. */
    assert(generator1);
    assert(generator2);
}

void GeneratorFork::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    generator1->generate_applicable_ops(state, applicable_ops);
    generator2->generate_applicable_ops(state, applicable_ops);
}

void GeneratorFork::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    generator1->generate_applicable_ops(state, applicable_ops);
    generator2->generate_applicable_ops(state, applicable_ops);
}

GeneratorSwitchVector::GeneratorSwitchVector(
    int switch_var_id, vector<unique_ptr<GeneratorBase>> &&generator_for_value)
    : switch_var_id(switch_var_id),
      generator_for_value(move(generator_for_value)) {
}

void GeneratorSwitchVector::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id].get_value();
    const unique_ptr<GeneratorBase> &generator_for_val = generator_for_value[val];
    if (generator_for_val) {
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

void GeneratorSwitchVector::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id];
    const unique_ptr<GeneratorBase> &generator_for_val = generator_for_value[val];
    if (generator_for_val) {
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

GeneratorSwitchHash::GeneratorSwitchHash(
    int switch_var_id,
    unordered_map<int, unique_ptr<GeneratorBase>> &&generator_for_value)
    : switch_var_id(switch_var_id),
      generator_for_value(move(generator_for_value)) {
}

void GeneratorSwitchHash::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id].get_value();
    const auto &child = generator_for_value.find(val);
    if (child != generator_for_value.end()) {
        const unique_ptr<GeneratorBase> &generator_for_val = child->second;
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

void GeneratorSwitchHash::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id];
    const auto &child = generator_for_value.find(val);
    if (child != generator_for_value.end()) {
        const unique_ptr<GeneratorBase> &generator_for_val = child->second;
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

GeneratorSwitchSingle::GeneratorSwitchSingle(
    int switch_var_id, int value, unique_ptr<GeneratorBase> generator_for_value)
    : switch_var_id(switch_var_id),
      value(value),
      generator_for_value(move(generator_for_value)) {
}

void GeneratorSwitchSingle::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    if (value == state[switch_var_id].get_value()) {
        generator_for_value->generate_applicable_ops(state, applicable_ops);
    }
}

void GeneratorSwitchSingle::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    if (value == state[switch_var_id]) {
        generator_for_value->generate_applicable_ops(state, applicable_ops);
    }
}

GeneratorLeafList::GeneratorLeafList(list<OperatorID> &&applicable_operators)
    : applicable_operators(move(applicable_operators)) {
}

void GeneratorLeafList::generate_applicable_ops(
    const State &, vector<OperatorID> &applicable_ops) const {
    // See above for the reason for using push_back instead of insert.
    for (OperatorID id : applicable_operators) {
        applicable_ops.push_back(id);
    }
}

void GeneratorLeafList::generate_applicable_ops(
    const GlobalState &, vector<OperatorID> &applicable_ops) const {
    // See above for the reason for using push_back instead of insert.
    for (OperatorID id : applicable_operators) {
        applicable_ops.push_back(id);
    }
}

GeneratorLeafSingle::GeneratorLeafSingle(OperatorID applicable_operator)
    : applicable_operator(applicable_operator) {
}

void GeneratorLeafSingle::generate_applicable_ops(
    const State &, vector<OperatorID> &applicable_ops) const {
    applicable_ops.push_back(applicable_operator);
}

void GeneratorLeafSingle::generate_applicable_ops(
    const GlobalState &, vector<OperatorID> &applicable_ops) const {
    applicable_ops.push_back(applicable_operator);
}

SuccessorGenerator::SuccessorGenerator(const TaskProxy &task_proxy)
    : root(SuccessorGeneratorFactory(task_proxy).create()) {
}

SuccessorGenerator::~SuccessorGenerator() = default;

void SuccessorGenerator::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}


void SuccessorGenerator::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}
}
