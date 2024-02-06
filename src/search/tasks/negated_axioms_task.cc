#include "negated_axioms_task.h"

#include "../operator_cost.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <deque>
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
    unordered_set<int> needed_negatively;
    State init = task_proxy.get_initial_state();
    for (const FactProxy &goal : task_proxy.get_goals()) {
        if (goal.get_variable().is_derived() && goal.get_value() == 1) { // TODO: fix the hardcoded 1
            needed_negatively.insert(goal.get_pair().var);
        }
    }
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (FactProxy condition : op.get_preconditions()) {
            if (condition.get_variable().is_derived() && condition.get_value() == 1) {  // TODO: fix the hardcoded 1
                needed_negatively.insert(condition.get_pair().var);
            }
        }

        for (EffectProxy effect  : op.get_effects()) {
            for (FactProxy condition : effect.get_conditions()) {
                if (condition.get_variable().is_derived() && condition.get_value() == 1) {  // TODO: fix the hardcoded 1
                    needed_negatively.insert(condition.get_pair().var);
                }
            }
        }
    }
    unordered_map<int, vector<int>> positive_dependencies;
    unordered_map<int, vector<OperatorProxy>> axioms_for_var;
    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        int head_var = axiom.get_effects()[0].get_fact().get_variable().get_id();
        if (axioms_for_var.count(head_var) > 0) {
            axioms_for_var[head_var].push_back(axiom);
        } else {
            axioms_for_var.insert({head_var, {axiom} });
        }
        for (FactProxy condition : axiom.get_effects()[0].get_conditions()) {
            if (condition.get_variable().is_derived()) {
                int var = condition.get_variable().get_id();
                if (condition.get_value() == 1) { // TODO: fix the hardcoded 1
                    needed_negatively.insert(var);
                } else {
                    if (positive_dependencies.count(head_var) > 0) {
                        positive_dependencies[head_var].push_back(var);
                    } else {
                        positive_dependencies.insert({head_var, {var} });
                    }
                }
            }
        }
    }

    // All derived variables that occur positively in an axiom for a needed negatively var are also needed negatively.
    deque<int> to_process(needed_negatively.begin(), needed_negatively.end());
    while (!to_process.empty()) {
        int var = to_process.front();
        to_process.pop_front();
        for (int var2 : positive_dependencies[var]) {
            if (needed_negatively.count(var2) == 0) {
                needed_negatively.insert(var2);
                to_process.push_back(var2);
            }
        }
    }

//    unordered_map<int, vector<OperatorProxy>> relevant_variables;
//    for (VariableProxy var : task_proxy.get_variables()) {
//        if (var.is_derived()) {
//            relevant_variables.insert({ var.get_id(), {} });
//        }
//    }

    // If there are no derived variables that occur as their default value we don't need to do anything.
    if (needed_negatively.empty()) {
        return;
    }

    // Build dependency graph and collect relevant axioms for derived values occurring as their default value.
    // TODO: this is a superset of positive_dependencies
    // TODO: this includes also (unreachable) vertices for non-derived variables
    vector<vector<int>> dependency_graph;
    dependency_graph.resize(task_proxy.get_variables().size(), {});

    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        FactPair head = axiom.get_effects()[0].get_fact().get_pair();
        for (FactProxy condition : axiom.get_effects()[0].get_conditions()) {
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
            if (needed_negatively.count(var) > 0) {
                var_to_scc.insert({var, i});
            }
        }
    }

    for (int var : needed_negatively) {
        vector<OperatorProxy> &axioms = axioms_for_var[var];

        if (sccs[var_to_scc[var]].size() > 1) {
            /* If the cluster contains multiple variables, they have a cyclic
               positive dependency. In this case, the "obvious" way of
               negating the formula defining the derived variable is
               semantically wrong. For details, see issue453.

               Therefore, in this case we perform a naive overapproximation
               instead, which assumes that derived variables occurring in
               such clusters can be false unconditionally. This is good
               enough for correctness of the code that uses these negated
               axioms, but loses accuracy. Negating the rules in an exact
               (non-overapproximating) way is possible but more expensive.
               Again, see issue453 for details.
            */
            std::string name = task_proxy.get_variables()[var].get_name() + "_negated";
            // TODO: fix the hardcoded 1
            negated_axioms.emplace_back(FactPair(var, 1), vector<FactPair>(), name);
        } else {
            // TODO: fix the hardcoded 1
            add_negated_axioms(FactPair(var, 1), axioms, task_proxy);
        }
    }

//    for (NegatedAxiom axiom : negated_axioms) {
//        cout << "(" << axiom.head.var << "," << axiom.head.value << " <- { ";
//        for (FactPair fact : axiom.condition) {
//            cout << "(" << fact.var << "," << fact.value << "), ";
//        }
//        cout << "} (" << axiom.name << ")" << endl;
//    }
}

void NegatedAxiomsTask::add_negated_axioms(FactPair head, std::vector<OperatorProxy> &axioms, TaskProxy &task_proxy) {
    for (auto ax : axioms) {
        auto eff = ax.get_effects();
    }
    std::string name = task_proxy.get_variables()[head.var].get_name() + "_negated";
    // If no axioms change the variable to its non-default value then the default is always true.
    if (axioms.size() == 0) {
        negated_axioms.emplace_back(head, vector<FactPair>(), name);
        return;
    }

    /*
       The conditions of the negated axioms are calculated by multiplying out the CNF resulting from negating
       the disjunction over all axioms. This is done incrementally, multiplying out each new axiom each step.

       For example, if we have axiom conditions {a,b,c}, {0,1}, {x} we proceed as follows:
        - Initialize negated_axiom_conditions to { {} }
        - After processing {a,b,c}, negated_axiom_conditions is { {¬a}, {¬b}, {¬c} }.
        - After processing {0,1}, negated_axiom_conditions is { {¬a,¬0}, {¬b,¬0}, {¬c,¬0}, {¬a,¬1}, {¬b,¬1}, {¬c,¬1} }.
        - After processing {x}, negated_axiom_conditions is
          { {¬a,¬0,¬x}, {¬b,¬0,¬x}, {¬c,¬0,¬x}, {¬a,¬1,¬x}, {¬b,¬1,¬x}, {¬c,¬1,¬x} }.
    */
    vector<vector<FactPair>> negated_axiom_conditions = { vector<FactPair>() };
    for(OperatorProxy axiom : axioms) {
        /*
           The derived fact we want to negate is triggered with an empty condition,
           so it is always true and its negation is always false.
        */
        if (axiom.get_effects()[0].get_conditions().size() == 0) {
            return;
        }

        /*
           We need to negate the condition of the axiom, which is a conjunction of facts.
           This is a disjunction over all negated facts, while a negated fact in turn
           is a disjunction over all other facts belonging to the same variable.

           For example assume dom(v0) = {1,2}, dom(v1) = {a,b,c} and the condition of the axiom is
           v0=1 \land v1=c. Then the negation is v0=0 \lor v1=a \lor v1 = b.
        */
        std::vector<FactPair> facts;
        for (FactProxy axiom_fact : axiom.get_effects()[0].get_conditions()) {
            FactPair axiom_fact_pair = axiom_fact.get_pair();
            for (int i = 0; i < task_proxy.get_variables()[axiom_fact_pair.var].get_domain_size(); ++i) {
                if (i != axiom_fact_pair.value) {
                    facts.emplace_back(axiom_fact_pair.var, i);
                }
            }
        }

        if (facts.size() == 1) {
            for (vector<FactPair> &condition : negated_axiom_conditions) {
                condition.push_back(facts[0]);
            }
        } else {
            vector<vector<FactPair>> new_conditions;
            for (FactPair &fact : facts) {
                size_t start = new_conditions.size();
                new_conditions.insert(new_conditions.end(),
                                      negated_axiom_conditions.begin(), negated_axiom_conditions.end());
                for (size_t i = start; i < new_conditions.size(); ++i) {
                    new_conditions[i].push_back(fact);
                }
            }
            negated_axiom_conditions = new_conditions;
        }
    }

    // TODO: simplify step, we currently have axioms with duplicate preconditions, eg (2,1), (14,1), (14,1)
    for (size_t i = 0; i < negated_axiom_conditions.size(); ++i) {
        negated_axioms.emplace_back(head, negated_axiom_conditions[i], name + to_string(i));
    }
}

int NegatedAxiomsTask::get_operator_cost(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        return parent->get_operator_cost(index, is_axiom);
    }

    return 0;
}

std::string NegatedAxiomsTask::get_operator_name(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        return parent->get_operator_name(index, is_axiom);
    }

    return negated_axioms[index-negated_axioms_start_index].name;
}

int NegatedAxiomsTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        return parent->get_num_operator_preconditions(index, is_axiom);
    }

    return negated_axioms[index-negated_axioms_start_index].condition.size();
}

FactPair NegatedAxiomsTask::get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const {
    if (!is_axiom || (op_index < negated_axioms_start_index)) {
        return parent->get_operator_precondition(op_index, fact_index, is_axiom);
    }
    return negated_axioms[op_index-negated_axioms_start_index].condition[fact_index];
}

int NegatedAxiomsTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    if (!is_axiom || op_index < negated_axioms_start_index) {
        return parent->get_num_operator_effects(op_index, is_axiom);
    }

    return 1;
}

int NegatedAxiomsTask::get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const {
    if (!is_axiom || op_index < negated_axioms_start_index) {
        return parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
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
        return parent->get_operator_effect(op_index, eff_index, is_axiom);
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
    NegatedAxiomsTaskFeature() : TypedFeature("negated_axioms") {
        document_title("negated axioms task");
        document_synopsis(
            "A task transformation that adds rules for when the a derived variable retains its default value. "
            "This can be useful and even needed for correctness for heuristics that treat axioms as normal operators.");

        add_cost_type_option_to_feature(*this);
    }

    virtual shared_ptr<NegatedAxiomsTask> create_component(const plugins::Options &, const utils::Context &) const override {
        // TODO: actually pass parent task?
        return make_shared<NegatedAxiomsTask>(g_root_task);
    }
};

static plugins::FeaturePlugin<NegatedAxiomsTaskFeature> _plugin;
}
