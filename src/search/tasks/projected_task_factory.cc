#include "projected_task_factory.h"

#include "projected_task.h"
#include "../task_proxy.h"
#include "../task_utils/task_properties.h"

#include <algorithm>

using namespace std;

namespace extra_tasks {
shared_ptr<AbstractTask> build_projected_task(
    const shared_ptr<AbstractTask> &parent,
    vector<int> &&variables) {
    TaskProxy parent_proxy(*parent);
    if (task_properties::has_axioms(parent_proxy)) {
        ABORT("ProjectedTask doesn't support axioms.");
    }
    if (task_properties::has_conditional_effects(parent_proxy)) {
        ABORT("ProjectedTask doesn't support conditional effects.");
    }

    vector<int> operator_indices;
    vector<vector<FactPair>> operator_preconditions;
    vector<vector<FactPair>> operator_effects;
    vector<FactPair> goals;

    // Map variables of the parent to indices in the projection.
    vector<int> var_to_index(parent_proxy.get_variables().size(), -1);
    for (size_t i = 0; i < variables.size(); ++i) {
        int var = variables[i];
        var_to_index[var] = i;
    }

    /*
      Compute the preconditions and effects (both set of facts) of all
      operators relevant to the projection, i.e., operators which have an
      effect on some variable of the projection.
    */
    int num_operators = parent->get_num_operators();
    for (int op_index = 0; op_index < num_operators; ++op_index) {
        vector<FactPair> effects;

        int num_effects = parent->get_num_operator_effects(op_index, false);
        for (int eff_index = 0; eff_index < num_effects; ++eff_index) {
            FactPair effect =
                parent->get_operator_effect(op_index, eff_index, false);
            int var = effect.var;
            if (var_to_index[var] != -1) {
                effects.emplace_back(var_to_index[var], effect.value);
            }
        }

        if (!effects.empty()) {
            operator_indices.push_back(op_index);
            operator_effects.push_back(move(effects));

            vector<FactPair> preconditions;
            int num_preconditions = parent->get_num_operator_preconditions(op_index, false);
            for (int fact_index = 0; fact_index < num_preconditions; ++fact_index) {
                FactPair precondition =
                    parent->get_operator_precondition(op_index, fact_index, false);
                int var = precondition.var;
                if (var_to_index[var] != -1) {
                    preconditions.emplace_back(var_to_index[var], precondition.value);
                }
            }
            operator_preconditions.push_back(move(preconditions));
        }
    }

    int num_goals = parent->get_num_goals();
    for (int goal_index = 0; goal_index < num_goals; ++goal_index) {
        FactPair goal = parent->get_goal_fact(goal_index);
        int var = goal.var;
        if (find(variables.begin(), variables.end(), var) != variables.end()) {
            goals.emplace_back(var_to_index[var], goal.value);
        }
    }

    return make_shared<ProjectedTask>(
        parent,
        move(variables),
        move(operator_indices),
        move(operator_preconditions),
        move(operator_effects),
        move(goals));
}
}
