#include "root_task.h"

#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/collections.h"

#include <cassert>
#include <memory>

using namespace std;

namespace tasks {
RootTask::RootTask(
    const shared_ptr<AbstractTask> &parent,
    vector<ExplicitVariable> &&variables,
    vector<vector<set<FactPair>>> &&mutexes,
    vector<ExplicitOperator> &&operators,
    vector<ExplicitOperator> &&axioms,
    vector<int> &&initial_state_values,
    vector<FactPair> &&goals)
    : ExplicitTask(parent, move(variables), move(mutexes), move(operators), move(axioms),
                   move(initial_state_values), move(goals)) {
}

const GlobalOperator *RootTask::get_global_operator(int index, bool is_axiom) const {
    // When we switch to operator ids, this should just return index
    if (is_axiom) {
        assert(utils::in_bounds(index, g_axioms));
        return &g_axioms[index];
    } else {
        assert(utils::in_bounds(index, g_operators));
        return &g_operators[index];
    }
}

void RootTask::convert_state_values_from_parent(vector<int> &) const {
    ABORT("Invalid state conversion");
}

ExplicitOperator explicit_operator_from_global_operator(const GlobalOperator &op, bool is_axiom) {
    vector<FactPair> preconditions;
    preconditions.reserve(op.get_preconditions().size());
    for (const GlobalCondition &condition : op.get_preconditions()) {
        preconditions.emplace_back(condition.var, condition.val);
    }

    vector<ExplicitEffect> effects;
    effects.reserve(op.get_effects().size());
    for (const GlobalEffect &eff : op.get_effects()) {
        vector<FactPair> eff_conditions;
        eff_conditions.reserve(eff.conditions.size());
        for (const GlobalCondition &condition : eff.conditions) {
            eff_conditions.emplace_back(condition.var, condition.val);
        }
        effects.emplace_back(eff.var, eff.val, move(eff_conditions));
    }

    return ExplicitOperator(
        move(preconditions),
        move(effects),
        op.get_cost(),
        op.get_name(),
        is_axiom);
}

shared_ptr<RootTask> create_root_task(
    const vector<string> &variable_name,
    const vector<int> &variable_domain,
    const vector<vector<string>> &fact_names,
    const vector<int> &axiom_layers,
    const vector<int> &default_axiom_values,
    const vector<vector<set<FactPair>>> &inconsistent_facts,
    const vector<int> &initial_state_data,
    const vector<pair<int, int>> &goal,
    const vector<GlobalOperator> &global_operators,
    const vector<GlobalOperator> &global_axioms) {
    const shared_ptr<AbstractTask> parent = nullptr;

    vector<ExplicitVariable> variables;
    variables.reserve(variable_domain.size());
    for (int i = 0; i < static_cast<int>(variable_domain.size()); ++i) {
        string name = variable_name[i];
        vector<string> var_fact_names = fact_names[i];
        variables.emplace_back(
            variable_domain[i],
            move(name),
            move(var_fact_names),
            axiom_layers[i],
            default_axiom_values[i]);
    }

    vector<vector<set<FactPair>>> mutexes = inconsistent_facts;


    vector<ExplicitOperator> operators;
    operators.reserve(operators.size());
    for (const GlobalOperator &op : global_operators) {
        operators.push_back(explicit_operator_from_global_operator(op, false));
    }

    vector<ExplicitOperator> axioms;
    axioms.reserve(axioms.size());
    for (const GlobalOperator &axiom : global_axioms) {
        axioms.push_back(explicit_operator_from_global_operator(axiom, true));
    }

    vector<int> initial_state_values = initial_state_data;

    vector<FactPair> goals;
    goals.reserve(goal.size());
    for (pair<int, int> goal : goal) {
        goals.emplace_back(goal.first, goal.second);
    }

    return make_shared<RootTask>(
        parent,
        move(variables),
        move(mutexes),
        move(operators),
        move(axioms),
        move(initial_state_values),
        move(goals));
}

static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return g_root_task();
}

static PluginShared<AbstractTask> _plugin("no_transform", _parse);
}
