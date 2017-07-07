#include "successor_generator.h"

#include "global_state.h"
#include "task_tools.h"

#include "utils/collections.h"
#include "utils/memory.h"
#include "utils/timer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
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

using Condition = std::vector<FactProxy>;

bool smaller_variable_id(const FactProxy &f1, const FactProxy &f2) {
    return f1.get_variable().get_id() < f2.get_variable().get_id();
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

GeneratorBase *construct_recursive(
    const TaskProxy &task_proxy,
    vector<Condition> &conditions,
    vector<Condition::const_iterator> &next_condition_by_op,
    int switch_var_id, list<OperatorID> &&operator_queue);

class GeneratorBase {
public:
    virtual ~GeneratorBase() = default;
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const = 0;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const = 0;
};

class GeneratorImmediate : public GeneratorBase {
    list<OperatorID> immediate_operators;
    GeneratorBase *next_generator;
public:
    ~GeneratorImmediate();
    GeneratorImmediate(list<OperatorID> &&immediate_operators,
                       GeneratorBase *next_generator);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const;
};

class GeneratorFork : public GeneratorBase {
    GeneratorBase *generator1;
    GeneratorBase *generator2;
public:
    ~GeneratorFork();
    GeneratorFork(GeneratorBase *generator1, GeneratorBase *generator2);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const;
};

class GeneratorSwitchVector : public GeneratorBase {
    int switch_var_id;
    vector<GeneratorBase *> generator_for_value;
public:
    ~GeneratorSwitchVector();
    GeneratorSwitchVector(
        int switch_var_id,
        const vector<GeneratorBase *> &&generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const;
};

class GeneratorSwitchHash : public GeneratorBase {
    int switch_var_id;
    unordered_map<int, GeneratorBase *> generator_for_value;
public:
    ~GeneratorSwitchHash();
    GeneratorSwitchHash(
        int switch_var_id,
        unordered_map<int, GeneratorBase *> &&generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const;
};

class GeneratorSwitchSingle : public GeneratorBase {
    int switch_var_id;
    int value;
    GeneratorBase *generator_for_value;
public:
    ~GeneratorSwitchSingle();
    GeneratorSwitchSingle(
        int switch_var_id, int value, GeneratorBase *generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const;
};

class GeneratorLeafList : public GeneratorBase {
    list<OperatorID> applicable_operators;
public:
    GeneratorLeafList(list<OperatorID> &&applicable_operators);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const;
};

class GeneratorLeafSingle : public GeneratorBase {
    OperatorID applicable_operator;
public:
    GeneratorLeafSingle(OperatorID applicable_operator);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<OperatorID> &applicable_ops) const;
};

GeneratorImmediate::GeneratorImmediate(
    list<OperatorID> &&immediate_operators,
    GeneratorBase *next_generator)
    : immediate_operators(move(immediate_operators)),
      next_generator(next_generator) {
    /* There is no reason to to use GeneratorImmediate if there is no next generator.
       Use GeneratorLeaf instead in such situtations. */
    assert(next_generator);
}

GeneratorImmediate::~GeneratorImmediate() {
    delete next_generator;
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

GeneratorFork::GeneratorFork(GeneratorBase *generator1, GeneratorBase *generator2)
    : generator1(generator1),
      generator2(generator2) {
    /* There is no reason to use a fork if only one of the generators exists.
       Use the existing generator directly if one of them exists or a nullptr
       otherwise. */
    assert(generator1);
    assert(generator2);
}

GeneratorFork::~GeneratorFork() {
    delete generator1;
    delete generator2;
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
    int switch_var_id, const vector<GeneratorBase *> &&generator_for_value)
    : switch_var_id(switch_var_id),
      generator_for_value(move(generator_for_value)) {
}

GeneratorSwitchVector::~GeneratorSwitchVector() {
    for (GeneratorBase *generator : generator_for_value)
        delete generator;
}

void GeneratorSwitchVector::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id].get_value();
    GeneratorBase *generator_for_val = generator_for_value[val];
    if (generator_for_val) {
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

void GeneratorSwitchVector::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id];
    GeneratorBase *generator_for_val = generator_for_value[val];
    if (generator_for_val) {
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

GeneratorSwitchHash::GeneratorSwitchHash(
    int switch_var_id, unordered_map<int, GeneratorBase *> &&generator_for_value)
    : switch_var_id(switch_var_id),
      generator_for_value(move(generator_for_value)) {
}

GeneratorSwitchHash::~GeneratorSwitchHash() {
    for (auto child : generator_for_value)
        delete child.second;
}

void GeneratorSwitchHash::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id].get_value();
    const auto &child = generator_for_value.find(val);
    if (child != generator_for_value.end()) {
        GeneratorBase *generator_for_val = child->second;
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

void GeneratorSwitchHash::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id];
    const auto &child = generator_for_value.find(val);
    if (child != generator_for_value.end()) {
        GeneratorBase *generator_for_val = child->second;
        generator_for_val->generate_applicable_ops(state, applicable_ops);
    }
}

GeneratorSwitchSingle::GeneratorSwitchSingle(
    int switch_var_id, int value, GeneratorBase *generator_for_value)
    : switch_var_id(switch_var_id),
      value(value),
      generator_for_value(generator_for_value) {
}

GeneratorSwitchSingle::~GeneratorSwitchSingle() {
    delete generator_for_value;
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

SuccessorGenerator::SuccessorGenerator(const TaskProxy &task_proxy) {
    utils::Timer construction_timer;
    int peak_memory_before = utils::get_peak_memory_in_kb();

    OperatorsProxy operators = task_proxy.get_operators();
    // We need the iterators to conditions to be stable:
    vector<Condition> conditions;
    vector<Condition::const_iterator> next_condition_by_op;
    conditions.reserve(operators.size());
    list<OperatorID> all_operators;
    for (OperatorProxy op : operators) {
        Condition cond;
        cond.reserve(op.get_preconditions().size());
        for (FactProxy pre : op.get_preconditions()) {
            cond.push_back(pre);
        }
        // Conditions must be ordered by variable id.
        sort(cond.begin(), cond.end(), smaller_variable_id);
        all_operators.push_back(OperatorID(op.get_id()));
        conditions.push_back(cond);
        next_condition_by_op.push_back(conditions.back().begin());
    }

    root = unique_ptr<GeneratorBase>(construct_recursive(task_proxy, conditions, next_condition_by_op, 0, move(all_operators)));
    if (!root) {
        /* Task is trivially unsolvable. Create dummy leaf,
           so we don't have to check root for nullptr everywhere. */
        list<OperatorID> no_applicable_operators;
        root = utils::make_unique_ptr<GeneratorLeafList>(move(no_applicable_operators));
    }

    int peak_memory_after = utils::get_peak_memory_in_kb();
    int memory_diff = 1024 * (peak_memory_after - peak_memory_before);
    cout << endl;
    cout << "SG construction time: " << construction_timer << endl;
    cout << "SG construction peak memory difference: " << memory_diff << endl;
}

SuccessorGenerator::~SuccessorGenerator() {
}

GeneratorBase *construct_leaf(list<OperatorID> &&operators) {
    if (operators.size() == 1) {
        return new GeneratorLeafSingle(operators.front());
    } else {
        return new GeneratorLeafList(move(operators));
    }
}

GeneratorBase *construct_switch(
    const TaskProxy &task_proxy,
    vector<Condition> &conditions,
    vector<Condition::const_iterator> &next_condition_by_op,
    int switch_var_id,
    vector<list<OperatorID>> &&operators_for_value) {
    int num_values = operators_for_value.size();
    vector<GeneratorBase *> generator_for_value;
    generator_for_value.reserve(num_values);
    int num_non_zero = 0;
    for (list<OperatorID> &ops : operators_for_value) {
        GeneratorBase *value_generator =
            construct_recursive(task_proxy, conditions, next_condition_by_op, switch_var_id + 1, move(ops));
        if (value_generator) {
            ++num_non_zero;
        }
        generator_for_value.push_back(value_generator);
    }

    if (num_non_zero == 1) {
        for (int value = 0; value < num_values; ++value) {
            if (generator_for_value[value]) {
                return new GeneratorSwitchSingle(
                    switch_var_id, value, generator_for_value[value]);
            }
        }
        assert(false);
    }
    int vector_size = estimate_vector_size<GeneratorBase *>(num_values);
    int hash_size = estimate_unordered_map_size<int, GeneratorBase *>(num_non_zero);
    if (hash_size < vector_size) {
        unordered_map<int, GeneratorBase *> map;
        for (int value = 0; value < num_values; ++value) {
            if (generator_for_value[value]) {
                map[value] = generator_for_value[value];
            }
        }
        return new GeneratorSwitchHash(switch_var_id, move(map));
    } else {
        return new GeneratorSwitchVector(switch_var_id, move(generator_for_value));
    }
}

GeneratorBase *construct_branch(
    const TaskProxy &task_proxy,
    vector<Condition> &conditions,
    vector<Condition::const_iterator> &next_condition_by_op,
    int switch_var_id,
    vector<list<OperatorID>> &&operators_for_value,
    list<OperatorID> &&default_operators,
    list<OperatorID> &&applicable_operators) {
    GeneratorBase *switch_generator =
        construct_switch(task_proxy, conditions, next_condition_by_op, switch_var_id, move(operators_for_value));
    assert(switch_generator);

    GeneratorBase *non_immediate_generator = nullptr;
    if (default_operators.empty()) {
        non_immediate_generator = switch_generator;
    } else {
        GeneratorBase *default_generator = construct_recursive(
            task_proxy, conditions, next_condition_by_op, switch_var_id + 1, move(default_operators));
        non_immediate_generator = new GeneratorFork(switch_generator, default_generator);
    }

    if (applicable_operators.empty()) {
        return non_immediate_generator;
    } else {
        return new GeneratorImmediate(move(applicable_operators), non_immediate_generator);
    }
}

GeneratorBase *construct_recursive(
    const TaskProxy &task_proxy,
    vector<Condition> &conditions,
    vector<Condition::const_iterator> &next_condition_by_op,
    int switch_var_id, list<OperatorID> &&operator_queue) {
    if (operator_queue.empty())
        return nullptr;

    VariablesProxy variables = task_proxy.get_variables();
    int num_variables = variables.size();

    while (true) {
        // Test if no further switch is necessary (or possible).
        if (switch_var_id == num_variables) {
            return construct_leaf(move(operator_queue));
        }

        VariableProxy switch_var = variables[switch_var_id];
        int number_of_children = switch_var.get_domain_size();

        vector<list<OperatorID>> operators_for_val(number_of_children);
        list<OperatorID> default_operators;
        list<OperatorID> applicable_operators;

        bool all_ops_are_immediate = true;
        bool var_is_interesting = false;

        while (!operator_queue.empty()) {
            OperatorID op_id = operator_queue.front();
            int op_index = op_id.get_index();
            operator_queue.pop_front();
            assert(utils::in_bounds(op_index, next_condition_by_op));
            Condition::const_iterator &cond_iter = next_condition_by_op[op_index];
            if (cond_iter == conditions[op_index].end()) {
                var_is_interesting = true;
                applicable_operators.push_back(op_id);
            } else {
                assert(utils::in_bounds(
                           cond_iter - conditions[op_index].begin(), conditions[op_index]));
                all_ops_are_immediate = false;
                FactProxy fact = *cond_iter;
                if (fact.get_variable() == switch_var) {
                    var_is_interesting = true;
                    while (cond_iter != conditions[op_index].end() &&
                           cond_iter->get_variable() == switch_var) {
                        ++cond_iter;
                    }
                    operators_for_val[fact.get_value()].push_back(op_id);
                } else {
                    default_operators.push_back(op_id);
                }
            }
        }

        if (all_ops_are_immediate) {
            return construct_leaf(move(applicable_operators));
        } else if (var_is_interesting) {
            return construct_branch(
                task_proxy,
                conditions,
                next_condition_by_op,
                switch_var_id,
                move(operators_for_val),
                move(default_operators),
                move(applicable_operators));
        } else {
            // this switch var can be left out because no operator depends on it
            ++switch_var_id;
            default_operators.swap(operator_queue);
        }
    }
}

void SuccessorGenerator::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}


void SuccessorGenerator::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}
