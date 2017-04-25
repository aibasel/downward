#include "successor_generator.h"

#include "global_state.h"
#include "task_tools.h"

#include "utils/collections.h"
#include "utils/timer.h"

#include <algorithm>
#include <cassert>

using namespace std;

/* NOTE on possible optimizations:

   * Sharing "GeneratorEmpty" instances might help quite a bit with
     reducing memory usage and possibly even speed, because there are
     bound to be many instance of those. However, it complicates
     deleting the successor generator, and memory doesn't seem to be
     an issue. Speed appears to be fine now, too. So this is probably
     an unnecessary complication.

   * Using slist instead of list led to a further 10% speedup on the
     largest Logistics instance, logistics-98/prob28.pddl. It would of
     course also reduce memory usage. However, it would make the code
     g++-specific, so it's probably not worth it.

*/

bool smaller_variable_id(const FactProxy &f1, const FactProxy &f2) {
    return f1.get_variable().get_id() < f2.get_variable().get_id();
}

class GeneratorBase {
public:
    virtual ~GeneratorBase() = default;
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorProxy> &applicable_ops) const = 0;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const = 0;

    void dump_size_estimate() const {

        int size_estimate = get_size_estimate();
        int size_estimate_object_overhead = get_size_estimate_object_overhead();
        int size_estimate_operators = get_size_estimate_operators();
        int size_estimate_switch_var = get_size_estimate_switch_var();
        int size_estimate_generator_for_value = get_size_estimate_generator_for_value();
        int size_estimate_default_generator = get_size_estimate_default_generator();
        int size_estimate_next_generator = get_size_estimate_next_generator();

        cout << "SG size estimates: "
             << "total: " << size_estimate << endl;
        cout << "SG size estimates: "
             << "object overhead: " << size_estimate_object_overhead
             << " (" << size_estimate_object_overhead / (double) size_estimate << ")" << endl;
        cout << "SG size estimates: "
             << "operators: " << size_estimate_operators
             << " (" << size_estimate_operators / (double) size_estimate << ")" << endl;
        cout << "SG size estimates: "
             << "switch var: " << size_estimate_switch_var
             << " (" << size_estimate_switch_var / (double) size_estimate << ")" << endl;
        cout << "SG size estimates: "
             << "generator for value: " << size_estimate_generator_for_value
             << " (" << size_estimate_generator_for_value / (double) size_estimate << ")" << endl;
        cout << "SG size estimates: "
             << "default generator: " << size_estimate_default_generator
             << " (" << size_estimate_default_generator / (double) size_estimate << ")" << endl;
        cout << "SG size estimates: "
             << "next generator: " << size_estimate_next_generator
             << " (" << size_estimate_next_generator / (double) size_estimate << ")" << endl;

        int immediates = count_immediate();
        int switch_immediate_empty = count_switch_immediate_empty();
        int switch_immediate_single = count_switch_immediate_single();
        int switch_immediate_more = count_switch_immediate_more();
        int forks = count_forks();
        int switches = count_switches();
        int leaves = count_leaves();
        int leaf_applicable_empty = count_leaf_applicable_empty();
        int leaf_applicable_single = count_leaf_applicable_single();
        int leaf_applicable_more = count_leaf_applicable_more();
        int empty = count_empty();
        int total_nodes = immediates + forks + leaves + switches + empty;

        cout << "SG object counts: "
             << "immediates: " << immediates
             << " (" << immediates / (double) total_nodes << ")" << endl;
        cout << "SG object counts: "
             << "forks: " << forks
             << " (" << forks / (double) total_nodes << ")" << endl;
        cout << "SG object counts: "
             << "switches: " << switches
             << " (" << switches / (double) total_nodes << ")" << endl;
        cout << "SG object counts: "
             << "leaves: " << leaves
             << " (" << leaves / (double) total_nodes << ")" << endl;
        cout << "SG object counts: "
             << "empty: " << empty
             << " (" << empty / (double) total_nodes << ")" << endl;

        cout << "SG switch statistics: "
             << "immediate ops empty: " << switch_immediate_empty
             << " (" << switch_immediate_empty / (double) immediates << ")" << endl;
        cout << "SG switch statistics: "
             << "single immediate op: " << switch_immediate_single
             << " (" << switch_immediate_single / (double) immediates << ")" << endl;
        cout << "SG switch statistics: "
             << "more immediate ops: " << switch_immediate_more
             << " (" << switch_immediate_more / (double) immediates << ")" << endl;

        cout << "SG leaf statistics: "
             << "applicable ops empty: " << leaf_applicable_empty
             << " (" << leaf_applicable_empty / (double) leaves << ")" << endl;
        cout << "SG leaf statistics: "
             << "single applicable op: " << leaf_applicable_single
             << " (" << leaf_applicable_single / (double) leaves << ")" << endl;
        cout << "SG leaf statistics: "
             << "more applicable ops: " << leaf_applicable_more
             << " (" << leaf_applicable_more / (double) leaves << ")" << endl;
    }

    size_t get_size_estimate() const {
        return get_size_estimate_object_overhead() +
            get_size_estimate_operators() +
            get_size_estimate_switch_var() +
            get_size_estimate_generator_for_value() +
            get_size_estimate_default_generator() +
            get_size_estimate_next_generator();
    }

    virtual size_t get_size_estimate_object_overhead() const = 0;
    virtual size_t get_size_estimate_operators() const = 0;
    virtual size_t get_size_estimate_switch_var() const = 0;
    virtual size_t get_size_estimate_generator_for_value() const = 0;
    virtual size_t get_size_estimate_default_generator() const = 0;
    virtual size_t get_size_estimate_next_generator() const = 0;

    virtual size_t count_immediate() const = 0;
    virtual size_t count_switches() const = 0;
    virtual size_t count_leaves() const = 0;
    virtual size_t count_empty() const = 0;
    virtual size_t count_forks() const = 0;

    virtual size_t count_switch_immediate_empty() const = 0;
    virtual size_t count_switch_immediate_single() const = 0;
    virtual size_t count_switch_immediate_more() const = 0;
    virtual size_t count_leaf_applicable_empty() const = 0;
    virtual size_t count_leaf_applicable_single() const = 0;
    virtual size_t count_leaf_applicable_more() const = 0;
};

class GeneratorImmediate : public GeneratorBase {
    list<OperatorProxy> immediate_operators;
    GeneratorBase *next_generator;
public:
    ~GeneratorImmediate();
    GeneratorImmediate(list<OperatorProxy> &&immediate_operators,
                       GeneratorBase *next_generator);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorProxy> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const;

    virtual size_t get_size_estimate_object_overhead() const {
        size_t result = 0;
        result += 4; // estimate for vtbl pointer
        result += 8; // estimate for dynamic memory management overhead
        result += next_generator->get_size_estimate_object_overhead();
        return result;
    }

    virtual size_t get_size_estimate_operators() const {
        size_t result = 8; // base cost for list.
        if (immediate_operators.size() > 1)
            result += 16 * (immediate_operators.size() - 1);
        result += next_generator->get_size_estimate_operators();
        return result;
    }

    virtual size_t get_size_estimate_switch_var() const {
        return next_generator->get_size_estimate_switch_var();
    }

    virtual size_t get_size_estimate_generator_for_value() const {
        return next_generator->get_size_estimate_generator_for_value();
    }

    virtual size_t get_size_estimate_default_generator() const {
        return next_generator->get_size_estimate_default_generator();
    }

    virtual size_t get_size_estimate_next_generator() const {
        size_t result = 0;
        result += 4; // next generator pointer
        result += next_generator->get_size_estimate_next_generator();
        return result;
    }

    virtual size_t count_immediate() const {
        size_t result = 1;
        result += next_generator->count_immediate();
        return result;
    }

    virtual size_t count_forks() const {
        return next_generator->count_forks();
    }

    virtual size_t count_switches() const {
        return next_generator->count_switches();
    }

    virtual size_t count_leaves() const {
        return next_generator->count_leaves();
    }

    virtual size_t count_empty() const {
        return next_generator->count_empty();
    }

    virtual size_t count_switch_immediate_empty() const {
        size_t result = 0;
        if (immediate_operators.empty()) {
            result += 1;
        }
        result += next_generator->count_switch_immediate_empty();
        return result;
    }

    virtual size_t count_switch_immediate_single() const {
        size_t result = 0;
        if (immediate_operators.size() == 1) {
            result += 1;
        }
        result += next_generator->count_switch_immediate_single();
        return result;
    }

    virtual size_t count_switch_immediate_more() const {
        size_t result = 0;
        if (immediate_operators.size() > 1) {
            result += 1;
        }
        result += next_generator->count_switch_immediate_more();
        return result;
    }

    virtual size_t count_leaf_applicable_empty() const  {
        return next_generator->count_leaf_applicable_empty();
    }

    virtual size_t count_leaf_applicable_single() const {
        return next_generator->count_leaf_applicable_single();
    }

    virtual size_t count_leaf_applicable_more() const {
        return next_generator->count_leaf_applicable_more();
    }
};

class GeneratorFork : public GeneratorBase {
    GeneratorBase *generator1;
    GeneratorBase *generator2;
public:
    ~GeneratorFork();
    GeneratorFork(GeneratorBase *generator1, GeneratorBase *generator2);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorProxy> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const;

    virtual size_t get_size_estimate_object_overhead() const {
        size_t result = 0;
        result += 4; // estimate for vtbl pointer
        result += 8; // estimate for dynamic memory management overhead
        result += generator1->get_size_estimate_object_overhead();
        result += generator2->get_size_estimate_object_overhead();
        return result;
    }

    virtual size_t get_size_estimate_operators() const {
        size_t result = 0;
        result += generator1->get_size_estimate_operators();
        result += generator2->get_size_estimate_operators();
        return result;
    }

    virtual size_t get_size_estimate_switch_var() const {
        size_t result = 0;
        result += generator1->get_size_estimate_switch_var();
        result += generator2->get_size_estimate_switch_var();
        return result;
    }

    virtual size_t get_size_estimate_generator_for_value() const {
        size_t result = 0;
        result += generator1->get_size_estimate_generator_for_value();
        result += generator2->get_size_estimate_generator_for_value();
        return result;
    }

    virtual size_t get_size_estimate_default_generator() const {
        size_t result = 0;
        result += 8; // 2x generator pointer
        result += generator1->get_size_estimate_default_generator();
        result += generator2->get_size_estimate_default_generator();
        return result;
    }

    virtual size_t get_size_estimate_next_generator() const {
        size_t result = 0;
        result += generator1->get_size_estimate_next_generator();
        result += generator2->get_size_estimate_next_generator();
        return result;
    }

    virtual size_t count_immediate() const {
        size_t result = 0;
        result += generator1->count_immediate();
        result += generator2->count_immediate();
        return result;
    }

    virtual size_t count_forks() const {
        size_t result = 1;
        result += generator1->count_switches();
        result += generator2->count_switches();
        return result;
    }

    virtual size_t count_switches() const {
        size_t result = 0;
        result += generator1->count_switches();
        result += generator2->count_switches();
        return result;
    }

    virtual size_t count_leaves() const {
        size_t result = 0;
        result += generator1->count_leaves();
        result += generator2->count_leaves();
        return result;
    }

    virtual size_t count_empty() const {
        size_t result = 0;
        result += generator1->count_empty();
        result += generator2->count_empty();
        return result;
    }

    virtual size_t count_switch_immediate_empty() const {
        size_t result = 0;
        result += generator1->count_switch_immediate_empty();
        result += generator2->count_switch_immediate_empty();
        return result;
    }

    virtual size_t count_switch_immediate_single() const {
        size_t result = 0;
        result += generator1->count_switch_immediate_single();
        result += generator2->count_switch_immediate_single();
        return result;
    }

    virtual size_t count_switch_immediate_more() const {
        size_t result = 0;
        result += generator1->count_switch_immediate_more();
        result += generator2->count_switch_immediate_more();
        return result;
    }

    virtual size_t count_leaf_applicable_empty() const  {
        size_t result = 0;
        result += generator1->count_leaf_applicable_empty();
        result += generator2->count_leaf_applicable_empty();
        return result;
    }

    virtual size_t count_leaf_applicable_single() const {
        size_t result = 0;
        result += generator1->count_leaf_applicable_single();
        result += generator2->count_leaf_applicable_single();
        return result;
    }

    virtual size_t count_leaf_applicable_more() const {
        size_t result = 0;
        result += generator1->count_leaf_applicable_more();
        result += generator2->count_leaf_applicable_more();
        return result;
    }
};

class GeneratorSwitch : public GeneratorBase {
    VariableProxy switch_var;
    vector<GeneratorBase *> generator_for_value;
public:
    ~GeneratorSwitch();
    GeneratorSwitch(const VariableProxy &switch_var,
                    const vector<GeneratorBase *> &&generator_for_value);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorProxy> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const;

    virtual size_t get_size_estimate_object_overhead() const {
        size_t result = 0;
        result += 4; // estimate for vtbl pointer
        result += 8; // estimate for dynamic memory management overhead
        for (const auto &child : generator_for_value)
            result += child->get_size_estimate_object_overhead();
        return result;
    }

    virtual size_t get_size_estimate_operators() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->get_size_estimate_operators();
        return result;
    }

    virtual size_t get_size_estimate_switch_var() const {
        size_t result = 0;
        result += 8; // estimate for switch_var; could be made smaller
        for (const auto &child : generator_for_value)
            result += child->get_size_estimate_switch_var();
        return result;
    }

    virtual size_t get_size_estimate_generator_for_value() const {
        size_t result = 0;
        result += 12; // empty vector
        if (!generator_for_value.empty()) {
            result += 8; // memory management overhead
            result += 4 * generator_for_value.size();
        }
        for (const auto &child : generator_for_value)
            result += child->get_size_estimate_generator_for_value();
        return result;
    }

    virtual size_t get_size_estimate_default_generator() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->get_size_estimate_default_generator();
        return result;
    }

    virtual size_t get_size_estimate_next_generator() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->get_size_estimate_next_generator();
        return result;
    }

    virtual size_t count_immediate() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_immediate();
        return result;
    }

    virtual size_t count_forks() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_forks();
        return result;
    }

    virtual size_t count_switches() const {
        size_t result = 1;
        for (const auto &child : generator_for_value)
            result += child->count_switches();
        return result;
    }

    virtual size_t count_leaves() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_leaves();
        return result;
    }

    virtual size_t count_empty() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_empty();
        return result;
    }

    virtual size_t count_switch_immediate_empty() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_switch_immediate_empty();
        return result;
    }

    virtual size_t count_switch_immediate_single() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_switch_immediate_single();
        return result;
    }

    virtual size_t count_switch_immediate_more() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_switch_immediate_more();
        return result;
    }

    virtual size_t count_leaf_applicable_empty() const  {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_leaf_applicable_empty();
        return result;
    }

    virtual size_t count_leaf_applicable_single() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_leaf_applicable_single();
        return result;
    }

    virtual size_t count_leaf_applicable_more() const {
        size_t result = 0;
        for (const auto &child : generator_for_value)
            result += child->count_leaf_applicable_more();
        return result;
    }
};

class GeneratorLeaf : public GeneratorBase {
    list<OperatorProxy> applicable_operators;
public:
    GeneratorLeaf(list<OperatorProxy> &&applicable_operators);
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorProxy> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const;

    virtual size_t get_size_estimate_object_overhead() const {
        size_t result = 0;
        result += 4; // estimate for vtbl pointer
        result += 8; // estimate for dynamic memory management overhead
        return result;
    }

    virtual size_t get_size_estimate_operators() const {
        size_t result = 8; // base cost for list.
        if (applicable_operators.size() > 1)
            result += 16 * (applicable_operators.size() - 1);
        return result;
    }

    virtual size_t get_size_estimate_switch_var() const {
        return 0;
    }

    virtual size_t get_size_estimate_generator_for_value() const {
        return 0;
    }

    virtual size_t get_size_estimate_default_generator() const {
        return 0;
    }

    virtual size_t get_size_estimate_next_generator() const {
        return 0;
    }

    virtual size_t count_immediate() const {
        return 0;
    }

    virtual size_t count_forks() const {
        return 0;
    }

    virtual size_t count_switches() const {
        return 0;
    }

    virtual size_t count_leaves() const {
        return 1;
    }

    virtual size_t count_empty() const {
        return 0;
    }

    virtual size_t count_switch_immediate_empty() const {
        return 0;
    }

    virtual size_t count_switch_immediate_single() const {
        return 0;
    }

    virtual size_t count_switch_immediate_more() const {
        return 0;
    }

    virtual size_t count_leaf_applicable_empty() const {
        if (applicable_operators.empty()) {
            return 1;
        }
        return 0;
    }

    virtual size_t count_leaf_applicable_single() const {
        if (applicable_operators.size() == 1) {
            return 1;
        }
        return 0;
    }
    virtual size_t count_leaf_applicable_more() const {
        if (applicable_operators.size() > 1) {
            return 1;
        }
        return 0;
    }
};

class GeneratorEmpty : public GeneratorBase {
public:
    virtual void generate_applicable_ops(
        const State &state, vector<OperatorProxy> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    virtual void generate_applicable_ops(
        const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const;

    virtual size_t get_size_estimate_object_overhead() const {
        size_t result = 0;
        result += 4; // estimate for vtbl pointer
        result += 8; // estimate for dynamic memory management overhead
        return result;
    }

    virtual size_t get_size_estimate_operators() const {
        return 0;
    }

    virtual size_t get_size_estimate_switch_var() const {
        return 0;
    }

    virtual size_t get_size_estimate_generator_for_value() const {
        return 0;
    }

    virtual size_t get_size_estimate_default_generator() const {
        return 0;
    }

    virtual size_t get_size_estimate_next_generator() const {
        return 0;
    }

    virtual size_t count_immediate() const {
        return 0;
    }

    virtual size_t count_forks() const {
        return 0;
    }

    virtual size_t count_switches() const {
        return 0;
    }

    virtual size_t count_leaves() const {
        return 0;
    }

    virtual size_t count_empty() const {
        return 1;
    }

    virtual size_t count_switch_immediate_empty() const {
        return 0;
    }

    virtual size_t count_switch_immediate_single() const {
        return 0;
    }

    virtual size_t count_switch_immediate_more() const {
        return 0;
    }

    virtual size_t count_leaf_applicable_empty() const {
        return 0;
    }

    virtual size_t count_leaf_applicable_single() const {
        return 0;
    }

    virtual size_t count_leaf_applicable_more() const {
        return 0;
    }
};

GeneratorImmediate::GeneratorImmediate(
    list<OperatorProxy> &&immediate_operators,
    GeneratorBase *next_generator)
    : immediate_operators(move(immediate_operators)),
      next_generator(next_generator) {
}

GeneratorImmediate::~GeneratorImmediate() {
    delete next_generator;
}

void GeneratorImmediate::generate_applicable_ops(
    const State &state, vector<OperatorProxy> &applicable_ops) const {
    applicable_ops.insert(applicable_ops.end(),
                          immediate_operators.begin(),
                          immediate_operators.end());
    next_generator->generate_applicable_ops(state, applicable_ops);
}

void GeneratorImmediate::generate_applicable_ops(
    const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const {
    for (OperatorProxy op : immediate_operators) {
        applicable_ops.push_back(op.get_global_operator());
    }
    next_generator->generate_applicable_ops(state, applicable_ops);
}

GeneratorFork::GeneratorFork(GeneratorBase *generator1, GeneratorBase *generator2)
    : generator1(generator1),
      generator2(generator2) {
}

GeneratorFork::~GeneratorFork() {
    delete generator1;
    delete generator2;
}

void GeneratorFork::generate_applicable_ops(
    const State &state, vector<OperatorProxy> &applicable_ops) const {
    generator1->generate_applicable_ops(state, applicable_ops);
    generator2->generate_applicable_ops(state, applicable_ops);
}

void GeneratorFork::generate_applicable_ops(
    const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const {
    generator1->generate_applicable_ops(state, applicable_ops);
    generator2->generate_applicable_ops(state, applicable_ops);
}

GeneratorSwitch::GeneratorSwitch(
    const VariableProxy &switch_var, const vector<GeneratorBase *> &&generator_for_value)
    : switch_var(switch_var),
      generator_for_value(move(generator_for_value)) {
}

GeneratorSwitch::~GeneratorSwitch() {
    for (GeneratorBase *generator : generator_for_value)
        delete generator;
}

void GeneratorSwitch::generate_applicable_ops(
    const State &state, vector<OperatorProxy> &applicable_ops) const {
    int val = state[switch_var].get_value();
    generator_for_value[val]->generate_applicable_ops(state, applicable_ops);
}

void GeneratorSwitch::generate_applicable_ops(
    const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const {
    int val = state[switch_var.get_id()];
    generator_for_value[val]->generate_applicable_ops(state, applicable_ops);
}

GeneratorLeaf::GeneratorLeaf(list<OperatorProxy> &&applicable_operators)
    : applicable_operators(move(applicable_operators)) {
}

void GeneratorLeaf::generate_applicable_ops(
    const State &, vector<OperatorProxy> &applicable_ops) const {
    applicable_ops.insert(applicable_ops.end(),
                          applicable_operators.begin(),
                          applicable_operators.end());
}

void GeneratorLeaf::generate_applicable_ops(
    const GlobalState &, vector<const GlobalOperator *> &applicable_ops) const {
    for (OperatorProxy op : applicable_operators) {
        applicable_ops.push_back(op.get_global_operator());
    }
}

void GeneratorEmpty::generate_applicable_ops(
    const State &, vector<OperatorProxy> &) const {
}

void GeneratorEmpty::generate_applicable_ops(
    const GlobalState &, vector<const GlobalOperator *> &) const {
}

SuccessorGenerator::SuccessorGenerator(const TaskProxy &task_proxy)
    : task_proxy(task_proxy) {
    utils::Timer construction_timer;
    int peak_memory_before = utils::get_peak_memory_in_kb();

    OperatorsProxy operators = task_proxy.get_operators();
    // We need the iterators to conditions to be stable:
    conditions.reserve(operators.size());
    list<OperatorProxy> all_operators;
    for (OperatorProxy op : operators) {
        Condition cond;
        cond.reserve(op.get_preconditions().size());
        for (FactProxy pre : op.get_preconditions()) {
            cond.push_back(pre);
        }
        // Conditions must be ordered by variable id.
        sort(cond.begin(), cond.end(), smaller_variable_id);
        all_operators.push_back(op);
        conditions.push_back(cond);
        next_condition_by_op.push_back(conditions.back().begin());
    }

    root = unique_ptr<GeneratorBase>(construct_recursive(0, move(all_operators)));
    utils::release_vector_memory(conditions);
    utils::release_vector_memory(next_condition_by_op);

    int peak_memory_after = utils::get_peak_memory_in_kb();
    int memory_diff = 1024 * (peak_memory_after - peak_memory_before);
    cout << endl;
    cout << "SG construction time: " << construction_timer << endl;
    cout << "SG construction peak memory difference: " << memory_diff << endl;
    root->dump_size_estimate();
}

SuccessorGenerator::~SuccessorGenerator() {
}

GeneratorBase *SuccessorGenerator::construct_recursive(
    int switch_var_id, list<OperatorProxy> &&operator_queue) {
    if (operator_queue.empty())
        return new GeneratorEmpty;

    VariablesProxy variables = task_proxy.get_variables();
    int num_variables = variables.size();

    while (true) {
        // Test if no further switch is necessary (or possible).
        if (switch_var_id == num_variables)
            return new GeneratorLeaf(move(operator_queue));

        VariableProxy switch_var = variables[switch_var_id];
        int number_of_children = switch_var.get_domain_size();

        vector<list<OperatorProxy>> operators_for_val(number_of_children);
        list<OperatorProxy> default_operators;
        list<OperatorProxy> applicable_operators;

        bool all_ops_are_immediate = true;
        bool var_is_interesting = false;

        while (!operator_queue.empty()) {
            OperatorProxy op = operator_queue.front();
            operator_queue.pop_front();
            int op_id = op.get_id();
            assert(op_id >= 0 && op_id < (int)next_condition_by_op.size());
            Condition::const_iterator &cond_iter = next_condition_by_op[op_id];
            assert(cond_iter - conditions[op_id].begin() >= 0);
            assert(cond_iter - conditions[op_id].begin()
                   <= (int)conditions[op_id].size());
            if (cond_iter == conditions[op_id].end()) {
                var_is_interesting = true;
                applicable_operators.push_back(op);
            } else {
                all_ops_are_immediate = false;
                FactProxy fact = *cond_iter;
                if (fact.get_variable() == switch_var) {
                    var_is_interesting = true;
                    while (cond_iter != conditions[op_id].end() &&
                           cond_iter->get_variable() == switch_var) {
                        ++cond_iter;
                    }
                    operators_for_val[fact.get_value()].push_back(op);
                } else {
                    default_operators.push_back(op);
                }
            }
        }

        if (all_ops_are_immediate) {
            return new GeneratorLeaf(move(applicable_operators));
        } else if (var_is_interesting) {
            vector<GeneratorBase *> generator_for_val;
            for (list<OperatorProxy> &ops : operators_for_val) {
                generator_for_val.push_back(
                    construct_recursive(switch_var_id + 1, move(ops)));
            }
            GeneratorSwitch *switch_generator = new GeneratorSwitch(
                switch_var,
                move(generator_for_val));

            GeneratorBase *non_immediate_generator = nullptr;
            if (default_operators.empty()) {
                non_immediate_generator = switch_generator;
            } else {
                GeneratorBase *default_generator = construct_recursive(
                    switch_var_id + 1, move(default_operators));
                non_immediate_generator = new GeneratorFork(switch_generator, default_generator);
            }

            if (applicable_operators.empty()) {
                return non_immediate_generator;
            } else {
                return new GeneratorImmediate(move(applicable_operators), non_immediate_generator);
            }
        } else {
            // this switch var can be left out because no operator depends on it
            ++switch_var_id;
            default_operators.swap(operator_queue);
        }
    }
}

void SuccessorGenerator::generate_applicable_ops(
    const State &state, vector<OperatorProxy> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}


void SuccessorGenerator::generate_applicable_ops(
    const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}
