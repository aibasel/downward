#include "negated_axioms_task.h"

#include "../task_proxy.h"

#include "../algorithms/sccs.h"
#include "../plugins/plugin.h"

#include <deque>
#include <iostream>
#include <memory>
#include <set>

using namespace std;
using utils::ExitCode;

/*
  This task transformation adds explicit axioms for how the default value
  of derived variables can be achieved. In general this is done as follows:
  Given derived variable v with n axioms v <- c_1, ..., v <- c_n, add axioms
  that together represent ¬v <- ¬c_1 ^ ... ^ ¬c_n.

  Notes:
   - THE TRANSFORMATION CAN BE SLOW! The rule ¬v <- ¬c_1 ^ ... ^ ¬c_n must
   be split up into axioms whose conditions are simple conjunctions. Since
   all c_i are also simple conjunctions, this amounts to converting a CNF
   to a DNF.
   - The tranformation is not exact. For derived variables v that have cyclic
   dependencies, the general approach is incorrect. We instead trivially
   overapproximate such cases with the axiom ¬v <- T.
   - The search ignores axioms that set the derived variable to their default
   value. The task transformation is thus only meant for heuristics that need
   to know how to achieve the default value.
 */

namespace tasks {
NegatedAxiomsTask::NegatedAxiomsTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent),
      negated_axioms_start_index(parent->get_num_axioms()) {
    TaskProxy task_proxy(*parent);

    /*
      pos/neg_dependencies store for each derived variable v which
      derived variables appear positively/negatively in an axiom
      making v true.
      axiom_ids_for_var stores for each derived variable v which
      axioms making v true.
      Note that the vectors go over *all* variables (also non-derived ones),
      but only the indices that correspond to a variable ID of a derived
      variable actually have content.
     */
    vector<vector<int>> pos_dependencies(task_proxy.get_variables().size());
    vector<vector<int>> neg_dependencies(task_proxy.get_variables().size());
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
                    neg_dependencies[head_var].push_back(var);
                } else {
                    pos_dependencies[head_var].push_back(var);
                }
            }
        }
    }

    /*
       Get the sccs induced by positive dependencies.
       We do not include negative dependencies because they cannot
       introduce additional cycles (this would imply that the axioms
       are not stratifiable, which is already checked in the translator).
    */
    vector<vector<int>> sccs =
        sccs::compute_maximal_sccs(pos_dependencies);
    vector<vector<int> *> var_to_scc(
        task_proxy.get_variables().size(), nullptr);
    for (int i = 0; i < (int)sccs.size(); ++i) {
        for (int var: sccs[i]) {
            var_to_scc[var] = &sccs[i];
        }
    }

    unordered_set<int> needed_negatively =
        collect_needed_negatively(pos_dependencies, neg_dependencies, sccs);

    for (int var: needed_negatively) {
        vector<int> &axiom_ids = axiom_ids_for_var[var];
        int default_value =
            task_proxy.get_variables()[var].get_default_axiom_value();

        if (var_to_scc[var]->size() > 1) {
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

/*
  Collect for which derived variables it is relevant to know how they
  can become false.
  In general this derived variable v is needed negatively if
    (a) v appears negatively in the goal or an operator condition
    (b) v appears positively in the body of an axiom for a variable v'
    that is needed negatively
    (c) v appears negatively in the body of an axiom for a variable v'
    that is needed positively.
  We will however not consider case (c) if v' is in an scc with size>1.
  This is because for such v' we overapproximate the axioms for ¬v by
  the trivial v' <- T, meaning v is actually not needed for these rules.
 */
unordered_set<int> NegatedAxiomsTask::collect_needed_negatively(
    const vector<vector<int>> &positive_dependencies,
    const vector<vector<int>> &negative_dependencies,
    const vector<vector<int>> &sccs) {
    // Stores which derived variables are needed positively or negatively.
    set<pair<int, bool>> needed;

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

        /*
          var has cyclic dependencies -> negated axioms will have the form
          "¬var <- T" and thus not depend on anything.
        */
        if (sccs[var].size() > 1) {
            continue;
        }

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
}
