#include "successor_generator_internals.h"

#include "../global_state.h"
#include "../task_proxy.h"

#include <cassert>

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
}
