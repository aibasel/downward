#include "negated_axioms_task.h"

#include "../operator_cost.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"

#include <deque>
#include <iostream>
#include <memory>
#include <set>

using namespace std;
using utils::ExitCode;

namespace tasks {
NegatedAxiomsTask::NegatedAxiomsTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent),
      negated_axioms_start_index(parent->get_num_axioms()) {
    TaskProxy task_proxy(*parent);

    vector<vector<int>> pos_dependencies(task_proxy.get_variables().size());
    vector<vector<int>> neg_dependencies(task_proxy.get_variables().size());
    vector<vector<int>> axiom_ids_for_var(task_proxy.get_variables().size());
    for (OperatorProxy axiom: task_proxy.get_axioms()) {
        EffectProxy effect = axiom.get_effects()[0];
        int head_var = effect.get_fact().get_variable().get_id();
        axiom_ids_for_var[head_var].push_back(axiom.get_id());
        for (FactProxy condition: effect.get_conditions()) {
            VariableProxy var_proxy = condition.get_variable();
            if (var_proxy.is_derived()) {
                int var = condition.get_variable().get_id();
                if (condition.get_value() == var_proxy.get_default_axiom_value()) {
                    neg_dependencies[head_var].push_back(var);
                } else {
                    pos_dependencies[head_var].push_back(var);
                }
            }
        }
    }
    unordered_set<int> needed_negatively =
        collect_needed_negatively(pos_dependencies, neg_dependencies);

    /* TODO: I removed the return if needed_negatively is empty.
       It is somewhat wasteful if we have a big graph to compute the sccs,
       but probably should not be an issue. */

    /*
       Get the sccs induced by positive dependencies.
       Note that negative dependencies cannot introduce additional
       cycles because this would imply that the axioms are not
       stratifiable, which is already checked in the translator.
    */
    vector<vector<int>> sccs =
        sccs::compute_maximal_sccs(pos_dependencies);
    vector<int> var_to_scc(task_proxy.get_variables().size(), -1);
    for (int i = 0; i < (int)sccs.size(); ++i) {
        for (int var: sccs[i]) {
            var_to_scc[var] = i;
        }
    }

    for (int var: needed_negatively) {
        vector<int> &axiom_ids = axiom_ids_for_var[var];
        int default_value =
            task_proxy.get_variables()[var].get_default_axiom_value();

        if (sccs[var_to_scc[var]].size() > 1) {
            /*
               If there is a cyclic dependency between several derived
               variables, the "obvious" way of negating the formula
               defining the derived variable is semantically wrong
               (see issue453).

               In this case we perform a naive overapproximation
               instead, which assumes that derived variables occurring
               in the cycle can be false unconditionally. This is good
               enough for correctness of the code that uses these
               negated axioms, but loses accuracy. Negating the rules in
               an exact (non-overapproximating) way is possible but more
               expensive (again, see issue453).
            */
            negated_axioms.emplace_back(
                FactPair(var, default_value), vector<FactPair>());
        } else {
            add_negated_axioms_for_var(
                FactPair(var, default_value), axiom_ids);
        }
    }
}


unordered_set<int> NegatedAxiomsTask::collect_needed_negatively(
    const vector<vector<int>> &positive_dependencies,
    const vector<vector<int>> &negative_dependencies) {
    // Stores which derived variables are needed positively or negatively.
    set<pair<int, bool>> needed;

    // TODO: Should we store the proxy in the class? Pass it through methods?
    TaskProxy task_proxy(*parent);

    // Collect derived variables that occur as their default value.
    for (const FactProxy &goal: task_proxy.get_goals()) {
        VariableProxy var_proxy = goal.get_variable();
        if (var_proxy.is_derived()) {
            bool non_default =
                goal.get_value() != var_proxy.get_default_axiom_value();
            needed.emplace(goal.get_pair().var, non_default);
        }
    }
    for (OperatorProxy op: task_proxy.get_operators()) {
        for (FactProxy condition: op.get_preconditions()) {
            VariableProxy var_proxy = condition.get_variable();
            if (var_proxy.is_derived()) {
                bool non_default =
                    condition.get_value() != var_proxy.get_default_axiom_value();
                needed.emplace(condition.get_pair().var, non_default);
            }
        }

        for (EffectProxy effect: op.get_effects()) {
            for (FactProxy condition: effect.get_conditions()) {
                VariableProxy var_proxy = condition.get_variable();
                if (var_proxy.is_derived()) {
                    bool non_default =
                        condition.get_value() != var_proxy.get_default_axiom_value();
                    needed.emplace(condition.get_pair().var, non_default);
                }
            }
        }
    }

    deque<pair<int, bool>> to_process(needed.begin(), needed.end());
    while (!to_process.empty()) {
        int var = to_process.front().first;
        bool non_default = to_process.front().second;
        to_process.pop_front();
        for (int pos_dep : positive_dependencies[var]) {
            auto insert_retval = needed.emplace(pos_dep, non_default);
            if (insert_retval.second) {
                to_process.emplace_back(pos_dep, non_default);
            }
        }
        for (int neg_dep : negative_dependencies[var]) {
            auto insert_retval = needed.emplace(neg_dep, !non_default);
            if (insert_retval.second) {
                to_process.emplace_back(neg_dep, !non_default);
            }
        }
    }

    unordered_set<int> needed_negatively;
    for (pair<int, bool> entry : needed) {
        if (!entry.second) {
            needed_negatively.insert(entry.first);
        }
    }
    return needed_negatively;
}


void NegatedAxiomsTask::add_negated_axioms_for_var(
    FactPair head, vector<int> &axiom_ids) {
    TaskProxy task_proxy(*parent);

    /*
      If no axioms change the variable to its non-default value,
      then the default is always true.
    */
    if (axiom_ids.empty()) {
        negated_axioms.emplace_back(head, vector<FactPair>());
        return;
    }

    vector<set<FactPair>> conditions_as_cnf;
    conditions_as_cnf.reserve(axiom_ids.size());
    for (int axiom_id : axiom_ids) {
        OperatorProxy axiom = task_proxy.get_axioms()[axiom_id];
        conditions_as_cnf.emplace_back();
        for (FactProxy fact : axiom.get_effects()[0].get_conditions()) {
            int var_id = fact.get_variable().get_id();
            int num_vals = task_proxy.get_variables()[var_id].get_domain_size();
            for (int value = 0; value < num_vals; ++value) {
                if (value != fact.get_value()) {
                    conditions_as_cnf.back().insert({var_id, value});
                }
            }
        }
    }

    // We can see multiplying out the cnf as collecting all hitting sets.
    set<FactPair> current;
    set<set<FactPair>> hitting_sets;
    collect_non_dominated_hitting_sets_recursively(
        conditions_as_cnf, 0, current, hitting_sets);

    for (const set<FactPair> &c : hitting_sets) {
        negated_axioms.emplace_back(
            head, vector<FactPair>(c.begin(), c.end()));
    }
}


void NegatedAxiomsTask::collect_non_dominated_hitting_sets_recursively(
    const vector<set<FactPair>> &conditions_as_cnf, size_t index,
    set<FactPair> &hitting_set, set<set<FactPair>> &results) {
    if (index == conditions_as_cnf.size()) {
        /*
           Check whether the axiom body denoted in body is dominated.
           This is the case if there is a fact such that no conjunction
           in the cnf is covered by *only* this fact, implying that
           removing the fact from the body would still cover all
           conjunctions.
        */
        set<FactPair> not_uniquely_used(hitting_set);
        for (const set<FactPair> &condition : conditions_as_cnf) {
            vector<FactPair> intersection;
            set_intersection(condition.begin(), condition.end(),
                             hitting_set.begin(), hitting_set.end(),
                             back_inserter(intersection));
            if (intersection.size() == 1) {
                not_uniquely_used.erase(intersection[0]);
            }
        }
        if (not_uniquely_used.empty()) {
            results.insert(hitting_set);
        }
        return;
    }

    const set<FactPair> &conditions = conditions_as_cnf[index];
    for (const FactPair &fact_pair : conditions) {
        // The current set is covered with the current hitting set elements.
        if (hitting_set.find(fact_pair) != hitting_set.end()) {
            collect_non_dominated_hitting_sets_recursively(
                conditions_as_cnf, index + 1, hitting_set, results);
            return;
        }
    }

    for (const FactPair &elem : conditions) {
        // We don't allow choosing different facts from the same variable.
        bool same_var_occurs_already = false;
        for (const FactPair &f : hitting_set) {
            if (f.var == elem.var) {
                same_var_occurs_already = true;
                break;
            }
        }
        if (same_var_occurs_already) {
            continue;
        }

        set<FactPair> chosen_new(hitting_set);
        chosen_new.insert(elem);
        collect_non_dominated_hitting_sets_recursively(
            conditions_as_cnf, index + 1, chosen_new, results);
    }
}


int NegatedAxiomsTask::get_operator_cost(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        return parent->get_operator_cost(index, is_axiom);
    }

    return 0;
}

string NegatedAxiomsTask::get_operator_name(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        return parent->get_operator_name(index, is_axiom);
    }

    return "<axiom>";
}

int NegatedAxiomsTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    if (!is_axiom || index < negated_axioms_start_index) {
        return parent->get_num_operator_preconditions(index, is_axiom);
    }

    return negated_axioms[index - negated_axioms_start_index].condition.size();
}

FactPair NegatedAxiomsTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    if (!is_axiom || (op_index < negated_axioms_start_index)) {
        return parent->get_operator_precondition(op_index, fact_index, is_axiom);
    }
    return negated_axioms[op_index - negated_axioms_start_index].condition[fact_index];
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
    return negated_axioms[op_index - negated_axioms_start_index].head;
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
            "A task transformation that adds rules for when the a "
            "derived variable retains its default value. "
            "This can be useful and even needed for correctness for "
            "heuristics that treat axioms as normal operators.");

        add_cost_type_option_to_feature(*this);
    }

    virtual shared_ptr<NegatedAxiomsTask> create_component(const plugins::Options &, const utils::Context &) const override {
        // TODO: actually pass parent task?
        return make_shared<NegatedAxiomsTask>(g_root_task);
    }
};

static plugins::FeaturePlugin<NegatedAxiomsTaskFeature> _plugin;
}
