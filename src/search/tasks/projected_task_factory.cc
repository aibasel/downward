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

    unordered_map<int, int> var_to_index;
    vector<int> operator_indices;
    vector<vector<FactPair>> operator_preconditions;
    vector<vector<FactPair>> operator_effects;
    vector<FactPair> goals;

    // remember where in the pattern each variable is stored
    for (size_t i = 0; i < variables.size(); ++i) {
        int var = variables[i];
        var_to_index[var] = i;
    }

    // make a list of operator indices for operators which are
    // relevant to at least one variable in the pattern
    for (int opi = 0; opi < parent->get_num_operators(); ++opi) {
        vector<FactPair> effects;

        for (int effi = 0;
             effi < parent->get_num_operator_effects(opi, false); ++effi) {
            FactPair effect =
                parent->get_operator_effect(opi, effi, false);
            int var = effect.var;

            if (var_to_index.count(var) != 0) {
                effects.emplace_back(var_to_index[effect.var], effect.value);
            }
        }

        if (effects.empty()) {
            continue;
        } else {
            operator_indices.push_back(opi);
            operator_effects.push_back(effects);
        }

        vector<FactPair> preconditions;
        for (int condi = 0;
             condi < parent->get_num_operator_preconditions(opi, false); ++condi) {
            FactPair precondition =
                parent->get_operator_precondition(opi, condi, false);
            int var = precondition.var;

            // check if the precondition variable appears in the pattern
            if (var_to_index.count(var) != 0) {
                preconditions.emplace_back(
                    var_to_index[precondition.var], precondition.value);
            }
        }
        operator_preconditions.push_back(preconditions);
    }

    for (int goali = 0;
         goali < parent->get_num_goals(); ++goali) {
        FactPair goal =
            parent->get_goal_fact(goali);
        int var = goal.var;

        auto res = find(variables.begin(), variables.end(), var);
        if (res != variables.end()) {
            goals.emplace_back(var_to_index[goal.var], goal.value);
        }
    }
    assert((unsigned)parent->get_num_goals() >= goals.size());

    return make_shared<ProjectedTask>(
        parent,
        move(variables),
        move(operator_indices),
        move(operator_preconditions),
        move(operator_effects),
        move(goals));
}
}
