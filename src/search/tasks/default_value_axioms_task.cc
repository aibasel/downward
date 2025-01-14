#include "default_value_axioms_task.h"

#include "../task_proxy.h"

#include "../algorithms/sccs.h"
#include "../task_utils/task_properties.h"

#include <deque>
#include <iostream>
#include <memory>
#include <set>

using namespace std;
using utils::ExitCode;

namespace tasks {
DefaultValueAxiomsTask::DefaultValueAxiomsTask(
    const shared_ptr<AbstractTask> &parent,
    AxiomHandlingType axioms)
    : DelegatingTask(parent),
      axioms(axioms),
      default_value_axioms_start_index(parent->get_num_axioms()) {
    TaskProxy task_proxy(*parent);

    /*
      (non)default_dependencies store for each variable v all derived
      variables that appear with their (non)default value in the body of
      an axiom that sets v.
      axiom_ids_for_var stores for each derived variable v which
      axioms set v to their nondefault value.
      Note that the vectors go over *all* variables (also non-derived ones),
      but only the indices that correspond to a variable ID of a derived
      variable actually have content.
     */
    vector<vector<int>> nondefault_dependencies(task_proxy.get_variables().size());
    vector<vector<int>> default_dependencies(task_proxy.get_variables().size());
    vector<vector<int>> axiom_ids_for_var(task_proxy.get_variables().size());
    for (OperatorProxy axiom: task_proxy.get_axioms()) {
        EffectProxy effect = axiom.get_effects()[0];
        int head_var = effect.get_fact().get_variable().get_id();
        axiom_ids_for_var[head_var].push_back(axiom.get_id());
        for (FactProxy cond: effect.get_conditions()) {
            VariableProxy var_proxy = cond.get_variable();
            if (var_proxy.is_derived()) {
                int var = cond.get_variable().get_id();
                if (cond.get_value() == var_proxy.get_default_axiom_value()) {
                    default_dependencies[head_var].push_back(var);
                } else {
                    nondefault_dependencies[head_var].push_back(var);
                }
            }
        }
    }

    /*
       Get the sccs induced by nondefault dependencies.
       We do not include default dependencies because they cannot
       introduce additional cycles (this would imply that the axioms
       are not stratifiable, which is already checked in the translator).
    */
    vector<vector<int>> sccs;
    vector<vector<int> *> var_to_scc;
    // We don't need the sccs if we set axioms "v=default <- {}" everywhere.
    if (axioms == AxiomHandlingType::APPROXIMATE_NEGATIVE_CYCLES) {
        sccs = sccs::compute_maximal_sccs(nondefault_dependencies);
        var_to_scc = vector<vector<int> *>(
            task_proxy.get_variables().size(), nullptr);
        for (int i = 0; i < (int)sccs.size(); ++i) {
            for (int var: sccs[i]) {
                var_to_scc[var] = &sccs[i];
            }
        }
    }

    unordered_set<int> default_value_needed =
        get_vars_with_relevant_default_value(nondefault_dependencies,
                                             default_dependencies,
                                             var_to_scc);

    for (int var: default_value_needed) {
        vector<int> &axiom_ids = axiom_ids_for_var[var];
        int default_value =
            task_proxy.get_variables()[var].get_default_axiom_value();

        if (axioms == AxiomHandlingType::APPROXIMATE_NEGATIVE
            || var_to_scc[var]->size() > 1) {
            /*
               If there is a cyclic dependency between several derived
               variables, the "obvious" way of negating the formula
               defining the derived variable is semantically wrong
               (see issue453).

               In this case we perform a naive overapproximation
               instead, which assumes that derived variables occurring
               in the cycle can be false unconditionally. This is good
               enough for correctness of the code that uses these
               default value axioms, but loses accuracy. Negating the
               axioms in an exact (non-overapproximating) way is possible
               but more expensive (again, see issue453).
            */
            default_value_axioms.emplace_back(
                FactPair(var, default_value), vector<FactPair>());
        } else {
            add_default_value_axioms_for_var(
                FactPair(var, default_value), axiom_ids);
        }
    }
}

/*
  Collect for which derived variables it is relevant to know how they
  can obtain their default value. This is done by tracking for all
  derived variables which of their values are needed.

  Initially, we know that var=val is needed if it appears in a goal or
  operator condition. Then we iteratively do the following:
  a) If var=val is needed and var'=nondefault is in the body of an
     axiom setting var=nondefault, then var'=val is needed.
  b) If var=val is needed and var'=default is in the body of an axiom
     setting var=nondefault, then var'=!val is needed, where
       - !val=nondefault if val=default
       - !val=default if val=nondefault
  (var and var' are always derived variables.)

  If var=default is needed but we already know that the axioms we will
  introduce for var=default are going to have an empty body, then we don't
  apply a)/b) (because the axiom for var=default will not depend on anything).
*/
unordered_set<int> DefaultValueAxiomsTask::get_vars_with_relevant_default_value(
    const vector<vector<int>> &nondefault_dependencies,
    const vector<vector<int>> &default_dependencies,
    const vector<vector<int> *> &var_to_scc) {
    // Store which derived vars are needed default (true) / nondefault(false).
    utils::HashSet<pair<int, bool>> needed;

    TaskProxy task_proxy(*parent);

    // Collect derived variables that occur as their default value.
    for (const FactProxy &goal: task_proxy.get_goals()) {
        VariableProxy var_proxy = goal.get_variable();
        if (var_proxy.is_derived()) {
            bool default_value =
                goal.get_value() == var_proxy.get_default_axiom_value();
            needed.emplace(goal.get_pair().var, default_value);
        }
    }
    for (OperatorProxy op: task_proxy.get_operators()) {
        for (FactProxy condition: op.get_preconditions()) {
            VariableProxy var_proxy = condition.get_variable();
            if (var_proxy.is_derived()) {
                bool default_value =
                    condition.get_value() == var_proxy.get_default_axiom_value();
                needed.emplace(condition.get_pair().var, default_value);
            }
        }
        for (EffectProxy effect: op.get_effects()) {
            for (FactProxy condition: effect.get_conditions()) {
                VariableProxy var_proxy = condition.get_variable();
                if (var_proxy.is_derived()) {
                    bool default_value =
                        condition.get_value() == var_proxy.get_default_axiom_value();
                    needed.emplace(condition.get_pair().var, default_value);
                }
            }
        }
    }

    deque<pair<int, bool>> to_process(needed.begin(), needed.end());
    while (!to_process.empty()) {
        int var = to_process.front().first;
        bool default_value = to_process.front().second;
        to_process.pop_front();

        /*
          If we process a default value and already know that the axiom we
          will introduce has an empty body (either because we trivially
          overapproximate everything or because the variable has cyclic
          dependencies), then the axiom (and thus the current variable/value
          pair) doesn't depend on anything.
        */
        if ((default_value) &&
            (axioms == AxiomHandlingType::APPROXIMATE_NEGATIVE
             || var_to_scc[var]->size() > 1)) {
            continue;
        }

        for (int nondefault_dep : nondefault_dependencies[var]) {
            auto insert_retval = needed.emplace(nondefault_dep, default_value);
            if (insert_retval.second) {
                to_process.emplace_back(nondefault_dep, default_value);
            }
        }
        for (int default_dep : default_dependencies[var]) {
            auto insert_retval = needed.emplace(default_dep, !default_value);
            if (insert_retval.second) {
                to_process.emplace_back(default_dep, !default_value);
            }
        }
    }

    unordered_set<int> default_needed;
    for (pair<int, bool> entry : needed) {
        if (entry.second) {
            default_needed.insert(entry.first);
        }
    }
    return default_needed;
}


void DefaultValueAxiomsTask::add_default_value_axioms_for_var(
    FactPair head, vector<int> &axiom_ids) {
    TaskProxy task_proxy(*parent);

    /*
      If no axioms change the variable to its non-default value,
      then the default is always true.
    */
    if (axiom_ids.empty()) {
        default_value_axioms.emplace_back(head, vector<FactPair>());
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
    unordered_set<int> current_vars;
    set<set<FactPair>> hitting_sets;
    collect_non_dominated_hitting_sets_recursively(
        conditions_as_cnf, 0, current, current_vars, hitting_sets);

    for (const set<FactPair> &c : hitting_sets) {
        default_value_axioms.emplace_back(
            head, vector<FactPair>(c.begin(), c.end()));
    }
}


void DefaultValueAxiomsTask::collect_non_dominated_hitting_sets_recursively(
    const vector<set<FactPair>> &set_of_sets, size_t index,
    set<FactPair> &hitting_set, unordered_set<int> &hitting_set_vars,
    set<set<FactPair>> &results) {
    if (index == set_of_sets.size()) {
        /*
           Check whether the hitting set is dominated.
           If we find a fact f in hitting_set such that no set in the
           set of sets is covered by *only* f, then hitting_set \ {f}
           is still a hitting set that dominates hitting_set.
        */
        set<FactPair> not_uniquely_used(hitting_set);
        for (const set<FactPair> &set : set_of_sets) {
            vector<FactPair> intersection;
            set_intersection(set.begin(), set.end(),
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

    const set<FactPair> &set = set_of_sets[index];
    for (const FactPair &elem : set) {
        /*
          If the current set is covered with the current hitting set
          elements, we continue with the next set.
        */
        if (hitting_set.find(elem) != hitting_set.end()) {
            collect_non_dominated_hitting_sets_recursively(
                set_of_sets, index + 1, hitting_set, hitting_set_vars,
                results);
            return;
        }
    }

    for (const FactPair &elem : set) {
        // We don't allow choosing different facts from the same variable.
        if (hitting_set_vars.find(elem.var) != hitting_set_vars.end()) {
            continue;
        }

        hitting_set.insert(elem);
        hitting_set_vars.insert(elem.var);
        collect_non_dominated_hitting_sets_recursively(
            set_of_sets, index + 1, hitting_set, hitting_set_vars,
            results);
        hitting_set.erase(elem);
        hitting_set_vars.erase(elem.var);
    }
}


int DefaultValueAxiomsTask::get_operator_cost(int index, bool is_axiom) const {
    if (!is_axiom || index < default_value_axioms_start_index) {
        return parent->get_operator_cost(index, is_axiom);
    }

    return 0;
}

string DefaultValueAxiomsTask::get_operator_name(int index, bool is_axiom) const {
    if (!is_axiom || index < default_value_axioms_start_index) {
        return parent->get_operator_name(index, is_axiom);
    }

    return "";
}

int DefaultValueAxiomsTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    if (!is_axiom || index < default_value_axioms_start_index) {
        return parent->get_num_operator_preconditions(index, is_axiom);
    }

    return 1;
}

FactPair DefaultValueAxiomsTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    if (!is_axiom || (op_index < default_value_axioms_start_index)) {
        return parent->get_operator_precondition(op_index, fact_index, is_axiom);
    }

    assert(fact_index == 0);
    FactPair head = default_value_axioms[op_index - default_value_axioms_start_index].head;
    return FactPair(head.var, 1 - head.value);
}

int DefaultValueAxiomsTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    if (!is_axiom || op_index < default_value_axioms_start_index) {
        return parent->get_num_operator_effects(op_index, is_axiom);
    }

    return 1;
}

int DefaultValueAxiomsTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    if (!is_axiom || op_index < default_value_axioms_start_index) {
        return parent->get_num_operator_effect_conditions(
            op_index, eff_index, is_axiom);
    }

    assert(eff_index == 0);
    return default_value_axioms[op_index - default_value_axioms_start_index].condition.size();
}

FactPair DefaultValueAxiomsTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    if (!is_axiom || op_index < default_value_axioms_start_index) {
        return parent->get_operator_effect_condition(
            op_index, eff_index, cond_index, is_axiom);
    }

    assert(eff_index == 0);
    return default_value_axioms[op_index - default_value_axioms_start_index].condition[cond_index];
}

FactPair DefaultValueAxiomsTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    if (!is_axiom || op_index < default_value_axioms_start_index) {
        return parent->get_operator_effect(op_index, eff_index, is_axiom);
    }

    assert(eff_index == 0);
    return default_value_axioms[op_index - default_value_axioms_start_index].head;
}

int DefaultValueAxiomsTask::get_num_axioms() const {
    return parent->get_num_axioms() + default_value_axioms.size();
}

shared_ptr<AbstractTask> get_default_value_axioms_task_if_needed(
    const shared_ptr<AbstractTask> &task,
    AxiomHandlingType axioms) {
    TaskProxy proxy(*task);
    if (task_properties::has_axioms(proxy)) {
        return make_shared<tasks::DefaultValueAxiomsTask>(
            DefaultValueAxiomsTask(task, axioms));
    }
    return task;
}

void add_axioms_option_to_feature(plugins::Feature &feature) {
    feature.add_option<AxiomHandlingType>(
        "axioms",
        "How to compute axioms that describe how the negated "
        "(=default) value of a derived variable can be achieved.",
        "approximate_negative_cycles");
}

tuple<AxiomHandlingType> get_axioms_arguments_from_options(
    const plugins::Options &opts) {
    return make_tuple<AxiomHandlingType>(
        opts.get<AxiomHandlingType>("axioms"));
}

static plugins::TypedEnumPlugin<AxiomHandlingType> _enum_plugin({
        {"approximate_negative",
         "Overapproximate negated axioms for all derived variables by "
         "setting an empty condition, indicating the default value can "
         "always be achieved for free."},
        {"approximate_negative_cycles",
         "Overapproximate negated axioms for all derived variables which "
         "have cyclic dependencies by setting an empty condition, "
         "indicating the default value can always be achieved for free. "
         "For all other derived variables, the negated axioms are computed "
         "exactly. Note that this can potentially lead to a combinatorial "
         "explosion."}
    });
}
