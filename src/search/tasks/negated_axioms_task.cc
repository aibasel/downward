#include "negated_axioms_task.h"

#include "../operator_cost.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <iostream>
#include <memory>

using namespace std;
using utils::ExitCode;

namespace tasks {

NegatedAxiomsTask::NegatedAxiomsTask(
    const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent),
      negated_axioms_start_index(parent->get_num_axioms()) {

    TaskProxy task_proxy(*parent);

    // Collect derived variables that occur as their default value.
    unordered_map<int, vector<OperatorProxy>> relevant_variables;
    State init = task_proxy.get_initial_state();
    for (const FactProxy &goal : task_proxy.get_goals()) {
        if (goal.get_variable().is_derived() &&
                goal == init[goal.get_pair().var]) {
            relevant_variables.insert({goal.get_pair().var, {}});
        }
    }
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (FactProxy condition : op.get_preconditions()) {
            if (condition.get_variable().is_derived() &&
                condition == init[condition.get_pair().var]) {
                relevant_variables.insert({condition.get_pair().var, {}});
            }
        }

        for (EffectProxy effect  : op.get_effects()) {
            for (FactProxy condition : effect.get_conditions()) {
                if (condition.get_variable().is_derived() &&
                        condition == init[condition.get_pair().var]) {
                    relevant_variables.insert({condition.get_pair().var, {}});
                }
            }
        }
    }
    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        for (FactProxy condition : axiom.get_preconditions()) {
            if (condition.get_variable().is_derived() &&
                condition == init[condition.get_variable().get_id()]) {
                relevant_variables.insert({condition.get_variable().get_id(),{}});
            }
        }
    }

    // If there are no derived variables that occur as their default value we don't need to do anything.
    if (relevant_variables.empty()) {
        return;
    }

    // Build dependency graph and collect relevant axioms for derived values occurring as their default value.
    vector<vector<int>> dependency_graph; // TODO: this includes also non-derived variables, change?
    dependency_graph.resize(task_proxy.get_variables().size(), {});

    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        FactPair head = axiom.get_effects()[0].get_fact().get_pair();
        if (relevant_variables.count(head.var) > 0) {
            relevant_variables[head.var].push_back(axiom);
        }
        for (FactProxy condition : axiom.get_preconditions()) {
            if (condition.get_variable().is_derived()) {
                dependency_graph[condition.get_pair().var].push_back(head.var);
            }
        }
    }

    // Get strongly connected components and map all relevant vars to their scc
    vector<vector<int>> sccs = sccs::compute_maximal_sccs(dependency_graph);
    unordered_map<int,int> var_to_scc;
    for (size_t i = 0; i < sccs.size(); ++i) {
        for (int var : sccs[i]) {
            if (relevant_variables.count(var) > 0) {
                var_to_scc.insert({var, i});
            }
        }
    }

    for (auto &it : relevant_variables) {
        int var = it.first;
        vector<OperatorProxy> &axioms = it.second;

        if (sccs[var_to_scc[var]].size() > 1) {
            std::string name = task_proxy.get_variables()[var].get_name() + "_negated";
            // TODO: why does emplace not work here?
            negated_axioms.emplace(FactPair(var, init[var].get_value()), {}, name);
        } else {
            add_negated_axioms(var, axioms);
        }
    }
}

int NegatedAxiomsTask::get_operator_cost(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        parent->get_operator_cost(index, is_axiom);
    }

    return 0;
}

std::string NegatedAxiomsTask::get_operator_name(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        parent->get_operator_name(index, is_axiom);
    }

    return negated_axioms[index-negated_axioms_start_index].name;
}

int NegatedAxiomsTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        parent->get_num_operator_preconditions(index, is_axiom);
    }

    return negated_axioms[index-negated_axioms_start_index].condition.size();
}

FactPair NegatedAxiomsTask::get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const {
    if (!is_axiom || op_index < negated_axioms_start_index) {
        parent->get_operator_precondition(op_index, fact_index, is_axiom);
    }

    return negated_axioms[op_index-negated_axioms_start_index].condition[fact_index];
}

int NegatedAxiomsTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    if (!is_axiom || op_index < negated_axioms_start_index) {
        parent->get_num_operator_effects(op_index, is_axiom);
    }

    return 1;
}

int NegatedAxiomsTask::get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const {
    if (!is_axiom || op_index < negated_axioms_start_index) {
        parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
    }

    return 0;
}

FactPair NegatedAxiomsTask::get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const {
    assert(!is_axiom || op_index < negated_axioms_start_index);
    return parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom);
}

FactPair NegatedAxiomsTask::get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const {
    if (!is_axiom || op_index < negated_axioms_start_index) {
        parent->get_operator_effect(op_index, eff_index, is_axiom);
    }

    assert(eff_index == 0);
    return negated_axioms[op_index-negated_axioms_start_index].head;
}

int NegatedAxiomsTask::convert_operator_index_to_parent(int index) const {
    return index;
}

int NegatedAxiomsTask::get_num_axioms() const {
    return parent->get_num_axioms() + negated_axioms.size();
}


// TODO: should we even offer this as a Feature? Can we just leave this out if we only want it used internally?
class NegatedAxiomsTaskFeature : public plugins::TypedFeature<AbstractTask, NegatedAxiomsTask> {
public:
    NegatedAxiomsTaskFeature() : TypedFeature("adapt_costs") {
        document_title("negated axioms task");
        document_synopsis(
            "A task transformation that adds rules for when the a derived variable retains its default value. "
            "This can be useful and even needed for correctness for heuristics that treat axioms as normal operators.");

        add_cost_type_option_to_feature(*this);
    }

    virtual shared_ptr<NegatedAxiomsTask> create_component(const plugins::Options &options, const utils::Context &) const override {
        // TODO: actually pass parent task?
        return make_shared<NegatedAxiomsTask>(g_root_task);
    }
};

static plugins::FeaturePlugin<NegatedAxiomsTaskFeature> _plugin;
}
