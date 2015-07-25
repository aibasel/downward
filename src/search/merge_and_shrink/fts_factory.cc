#include "fts_factory.h"

#include "factored_transition_system.h"
#include "labels.h"
#include "transition_system.h"

#include "../task_proxy.h"
#include "../utilities.h"

#include <cassert>
#include <unordered_map>
#include <vector>

using namespace std;


static void build_atomic_transition_systems(
    const TaskProxy &task_proxy,
    vector<TransitionSystem *> &result,
    shared_ptr<Labels> labels);


FactoredTransitionSystem create_factored_transition_system(
    const TaskProxy &task_proxy, shared_ptr<Labels> labels) {
    vector<TransitionSystem *> transition_systems;
    int num_vars = task_proxy.get_variables().size();
    assert(num_vars >= 1);
    transition_systems.reserve(num_vars * 2 - 1);
    build_atomic_transition_systems(task_proxy, transition_systems, labels);
    return FactoredTransitionSystem(move(transition_systems));
}

void build_atomic_transition_systems(
    const TaskProxy &task_proxy,
    vector<TransitionSystem *> &result,
    shared_ptr<Labels> labels) {
    assert(result.empty());
    cout << "Building atomic transition systems... " << endl;
    VariablesProxy variables = task_proxy.get_variables();
    OperatorsProxy operators = task_proxy.get_operators();

    // Step 1: Create the transition system objects without transitions.
    for (VariableProxy var : variables)
        result.push_back(new TransitionSystem(task_proxy, labels, var.get_id()));

    // Step 2: Add transitions.
    vector<vector<bool> > relevant_labels(result.size(), vector<bool>(operators.size(), false));
    for (OperatorProxy op : operators) {
        int label_no = op.get_id();
        labels->add_label(op.get_cost());
        PreconditionsProxy preconditions = op.get_preconditions();
        EffectsProxy effects = op.get_effects();
        unordered_map<int, int> pre_val;
        vector<bool> has_effect_on_var(variables.size(), false);
        for (FactProxy precondition : preconditions)
            pre_val[precondition.get_variable().get_id()] = precondition.get_value();

        for (EffectProxy effect : effects) {
            FactProxy fact = effect.get_fact();
            VariableProxy var = fact.get_variable();
            int var_id = var.get_id();
            has_effect_on_var[var_id] = true;
            int post_value = fact.get_value();
            TransitionSystem *ts = result[var_id];

            // Determine possible values that var can have when this
            // operator is applicable.
            int pre_value = -1;
            auto pre_val_it = pre_val.find(var_id);
            if (pre_val_it != pre_val.end())
                pre_value = pre_val_it->second;
            int pre_value_min, pre_value_max;
            if (pre_value == -1) {
                pre_value_min = 0;
                pre_value_max = var.get_domain_size();
            } else {
                pre_value_min = pre_value;
                pre_value_max = pre_value + 1;
            }

            /*
              cond_effect_pre_value == x means that the effect has an
              effect condition "var == x".
              cond_effect_pre_value == -1 means no effect condition on var.
              has_other_effect_cond is true iff there exists an effect
              condition on a variable other than var.
            */
            EffectConditionsProxy effect_conditions = effect.get_conditions();
            int cond_effect_pre_value = -1;
            bool has_other_effect_cond = false;
            for (FactProxy condition : effect_conditions) {
                if (condition.get_variable() == var) {
                    cond_effect_pre_value = condition.get_value();
                } else {
                    has_other_effect_cond = true;
                }
            }

            // Handle transitions that occur when the effect triggers.
            for (int value = pre_value_min; value < pre_value_max; ++value) {
                /* Only add a transition if it is possible that the effect
                   triggers. We can rule out that the effect triggers if it has
                   a condition on var and this condition is not satisfied. */
                if (cond_effect_pre_value == -1 || cond_effect_pre_value == value) {
                    Transition trans(value, post_value);
                    ts->transitions_of_groups[label_no].push_back(trans);
                }
            }

            // Handle transitions that occur when the effect does not trigger.
            if (!effect_conditions.empty()) {
                for (int value = pre_value_min; value < pre_value_max; ++value) {
                    /* Add self-loop if the effect might not trigger.
                       If the effect has a condition on another variable, then
                       it can fail to trigger no matter which value var has.
                       If it only has a condition on var, then the effect
                       fails to trigger if this condition is false. */
                    if (has_other_effect_cond || value != cond_effect_pre_value) {
                        Transition loop(value, value);
                        ts->transitions_of_groups[label_no].push_back(loop);
                    }
                }
            }

            relevant_labels[var_id][label_no] = true;
        }

        for (FactProxy precondition : preconditions) {
            int var_id = precondition.get_variable().get_id();
            if (!has_effect_on_var[var_id]) {
                int value = precondition.get_value();
                TransitionSystem *ts = result[var_id];
                Transition trans(value, value);
                ts->transitions_of_groups[label_no].push_back(trans);
                relevant_labels[var_id][label_no] = true;
            }
        }
    }

    for (size_t i = 0; i < result.size(); ++i) {
        // Need to set the correct number of shared_ptr<Labels> after* generating them
        TransitionSystem *ts = result[i];
        ts->num_labels = labels->get_size();
        /* Make all irrelevant labels explicit and set the cost of every
           singleton label group. */
        for (int label_no = 0; label_no < ts->num_labels; ++label_no) {
            if (!relevant_labels[i][label_no]) {
                for (int state = 0; state < ts->num_states; ++state) {
                    Transition loop(state, state);
                    ts->transitions_of_groups[label_no].push_back(loop);
                }
            }
            ts->get_group_it(label_no)->set_cost(labels->get_label_cost(label_no));
        }
        ts->compute_locally_equivalent_labels();
        ts->compute_distances_and_prune();
        assert(ts->is_valid());
    }
}

