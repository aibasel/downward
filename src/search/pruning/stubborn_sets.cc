#include "stubborn_sets.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

using namespace std;

namespace stubborn_sets {
StubbornSets::StubbornSets(const plugins::Options &opts)
    : PruningMethod(opts),
      num_operators(-1) {
}

void StubbornSets::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    TaskProxy task_proxy(*task);
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);

    num_operators = task_proxy.get_operators().size();
    sorted_goals = utils::sorted<FactPair>(
        task_properties::get_fact_pairs(task_proxy.get_goals()));

    compute_sorted_operators(task_proxy);
    compute_achievers(task_proxy);
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
                    [](const EffectProxy &eff) {return eff.get_fact().get_pair();}));
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

void StubbornSets::prune(const State &state, vector<OperatorID> &op_ids) {
    // Clear stubborn set from previous call.
    stubborn.assign(num_operators, false);

    compute_stubborn_set(state);

    // Now check which applicable operators are in the stubborn set.
    vector<OperatorID> remaining_op_ids;
    remaining_op_ids.reserve(op_ids.size());
    for (OperatorID op_id : op_ids) {
        if (stubborn[op_id.get_index()]) {
            remaining_op_ids.emplace_back(op_id);
        }
    }
    op_ids.swap(remaining_op_ids);
}
}
