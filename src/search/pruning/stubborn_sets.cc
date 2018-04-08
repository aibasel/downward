#include "stubborn_sets.h"

#include "../option_parser.h"

#include "../task_utils/task_properties.h"
#include "../utils/collections.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace stubborn_sets {
// Number of expansions after which pruning ratio is checked.
static const int NUM_PRUNING_CALLS_BEFORE_CHECK = 1000;

// Relies on both fact sets being sorted by variable.
bool contain_conflicting_fact(const vector<FactPair> &facts1,
                              const vector<FactPair> &facts2) {
    auto facts1_it = facts1.begin();
    auto facts2_it = facts2.begin();
    while (facts1_it != facts1.end() && facts2_it != facts2.end()) {
        if (facts1_it->var < facts2_it->var) {
            ++facts1_it;
        } else if (facts1_it->var > facts2_it->var) {
            ++facts2_it;
        } else {
            if (facts1_it->value != facts2_it->value)
                return true;
            ++facts1_it;
            ++facts2_it;
        }
    }
    return false;
}

StubbornSets::StubbornSets(const options::Options &opts)
    : min_pruning_ratio(opts.get<double>("min_pruning_ratio")),
      num_pruning_calls(0),
      do_pruning(true) {
}

void StubbornSets::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    TaskProxy task_proxy(*task);
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);

    num_operators = task_proxy.get_operators().size();
    num_unpruned_successors_generated = 0;
    num_pruned_successors_generated = 0;
    sorted_goals = utils::sorted<FactPair>(
        task_properties::get_fact_pairs(task_proxy.get_goals()));

    compute_sorted_operators(task_proxy);
    compute_achievers(task_proxy);
}

// Relies on op_preconds and op_effects being sorted by variable.
bool StubbornSets::can_disable(int op1_no, int op2_no) const {
    return contain_conflicting_fact(sorted_op_effects[op1_no],
                                    sorted_op_preconditions[op2_no]);
}

// Relies on op_effect being sorted by variable.
bool StubbornSets::can_conflict(int op1_no, int op2_no) const {
    return contain_conflicting_fact(sorted_op_effects[op1_no],
                                    sorted_op_effects[op2_no]);
}

void StubbornSets::compute_sorted_operators(const TaskProxy &task_proxy) {
    OperatorsProxy operators = task_proxy.get_operators();

    sorted_op_preconditions = utils::map_vector<vector<FactPair>>(
        operators, [](const OperatorProxy &op) {
            return utils::sorted<FactPair>(
                task_properties::get_fact_pairs(op.get_preconditions()));
        });

    sorted_op_effects = utils::map_vector<vector<FactPair>>(
        operators, [](const OperatorProxy &op) {
            return utils::sorted<FactPair>(
                utils::map_vector<FactPair>(
                    op.get_effects(),
                    [](const EffectProxy &eff) {return eff.get_fact().get_pair(); }));
        });
}

void StubbornSets::compute_achievers(const TaskProxy &task_proxy) {
    achievers = utils::map_vector<vector<vector<int>>>(
        task_proxy.get_variables(), [](const VariableProxy &var) {
            return vector<vector<int>>(var.get_domain_size());
        });

    for (const OperatorProxy op : task_proxy.get_operators()) {
        for (const EffectProxy effect : op.get_effects()) {
            FactPair fact = effect.get_fact().get_pair();
            achievers[fact.var][fact.value].push_back(op.get_id());
        }
    }
}

bool StubbornSets::mark_as_stubborn(int op_no) {
    if (!stubborn[op_no]) {
        stubborn[op_no] = true;
        stubborn_queue.push_back(op_no);
        return true;
    }
    return false;
}

void StubbornSets::prune_operators(
    const State &state, vector<OperatorID> &op_ids) {
    if (!do_pruning) {
        return;
    }
    if (num_pruning_calls == NUM_PRUNING_CALLS_BEFORE_CHECK) {
        double pruning_ratio = 1 - (
            static_cast<double>(num_pruned_successors_generated) /
            static_cast<double>(num_unpruned_successors_generated));
        cout << "Pruning ratio after " << NUM_PRUNING_CALLS_BEFORE_CHECK
             << " calls: " << pruning_ratio << endl;
        if (pruning_ratio < min_pruning_ratio) {
            cout << "-- pruning ratio is lower than minimum pruning ratio ("
                 << min_pruning_ratio << ") -> switching off pruning" << endl;
            do_pruning = false;
        }
    }

    num_unpruned_successors_generated += op_ids.size();
    num_pruning_calls++;

    // Clear stubborn set from previous call.
    stubborn.assign(num_operators, false);
    assert(stubborn_queue.empty());

    initialize_stubborn_set(state);
    /* Iteratively insert operators to stubborn according to the
       definition of strong stubborn sets until a fixpoint is reached. */
    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();
        handle_stubborn_operator(state, op_no);
    }

    // Now check which applicable operators are in the stubborn set.
    vector<OperatorID> remaining_op_ids;
    remaining_op_ids.reserve(op_ids.size());
    for (OperatorID op_id : op_ids) {
        if (stubborn[op_id.get_index()]) {
            remaining_op_ids.emplace_back(op_id);
        }
    }
    op_ids.swap(remaining_op_ids);

    num_pruned_successors_generated += op_ids.size();
}

void StubbornSets::print_statistics() const {
    cout << "total successors before partial-order reduction: "
         << num_unpruned_successors_generated << endl
         << "total successors after partial-order reduction: "
         << num_pruned_successors_generated << endl;
}

void add_pruning_options(options::OptionParser &parser) {
    parser.add_option<double>(
        "min_pruning_ratio",
        "minimal pruning ratio such that pruning is not switched off",
        "0.0",
        Bounds("0.0", "1.0"));
}
}
