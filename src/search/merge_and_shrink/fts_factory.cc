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


class FTSFactory {
    const TaskProxy &task_proxy;
    shared_ptr<Labels> labels;

    struct TransitionSystemData {
        vector<vector<Transition> > transitions_by_label;
        vector<bool> relevant_labels;
    };
    vector<TransitionSystemData> transition_system_by_var;

    vector<TransitionSystem *> transition_systems;

    void build_labels();
    void build_empty_transition_systems();
    void add_transition(int var_no, int label_no,
                        int src_value, int dest_value);
    bool is_relevant(int var_no, int label_no) const;
    void mark_as_relevant(int var_no, int label_no);
    unordered_map<int, int> compute_preconditions(OperatorProxy op);
    void handle_operator_effect(
        OperatorProxy op, EffectProxy effect,
        const unordered_map<int, int> &pre_val,
        vector<bool> &has_effect_on_var);
    void handle_operator_precondition(
        OperatorProxy op, FactProxy precondition,
        const vector<bool> &has_effect_on_var);
    void build_transitions_for_operator(OperatorProxy op);
    void build_transitions();
    void finalize_transition_systems();
public:
    FTSFactory(const TaskProxy &task_proxy, shared_ptr<Labels> labels);
    ~FTSFactory();

    /*
      Note: create() may only be called once. We don't worry about
      misuse because the class is only used internally in this file.
    */
    FactoredTransitionSystem create();
};


FTSFactory::FTSFactory(const TaskProxy &task_proxy, shared_ptr<Labels> labels)
    : task_proxy(task_proxy), labels(labels) {
}

FTSFactory::~FTSFactory() {
}

void FTSFactory::build_labels() {
    // Add all operators to the labels object.
    for (OperatorProxy op : task_proxy.get_operators())
        labels->add_label(op.get_cost());
}

void FTSFactory::build_empty_transition_systems() {
    // Create the transition system objects (without transitions).
    VariablesProxy variables = task_proxy.get_variables();
    assert(variables.size() >= 1);
    int num_operators = task_proxy.get_operators().size();
    // We reserve space for the transition systems added later by merging.
    transition_systems.reserve(variables.size() * 2 - 1);
    transition_system_by_var.resize(variables.size());
    for (VariableProxy var : variables) {
        transition_systems.push_back(
            new TransitionSystem(task_proxy, labels, var.get_id()));
        TransitionSystemData &ts_data = transition_system_by_var[var.get_id()];
        ts_data.transitions_by_label.resize(num_operators);
        ts_data.relevant_labels.resize(num_operators, false);
    }

}

void FTSFactory::add_transition(int var_no, int label_no,
                                int src_value, int dest_value) {
    transition_system_by_var[var_no].transitions_by_label[label_no].push_back(
        Transition(src_value, dest_value));
}

bool FTSFactory::is_relevant(int var_no, int label_no) const {
    return transition_system_by_var[var_no].relevant_labels[label_no];
}

void FTSFactory::mark_as_relevant(int var_no, int label_no) {
    transition_system_by_var[var_no].relevant_labels[label_no] = true;
}

unordered_map<int, int> FTSFactory::compute_preconditions(OperatorProxy op) {
    unordered_map<int, int> pre_val;
    for (FactProxy precondition : op.get_preconditions())
        pre_val[precondition.get_variable().get_id()] =
            precondition.get_value();
    return pre_val;
}

void FTSFactory::handle_operator_effect(
    OperatorProxy op, EffectProxy effect,
    const unordered_map<int, int> &pre_val, vector<bool> &has_effect_on_var) {
    int label_no = op.get_id();
    FactProxy fact = effect.get_fact();
    VariableProxy var = fact.get_variable();
    int var_no = var.get_id();
    has_effect_on_var[var_no] = true;
    int post_value = fact.get_value();

    // Determine possible values that var can have when this
    // operator is applicable.
    int pre_value = -1;
    auto pre_val_it = pre_val.find(var_no);
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
        if (cond_effect_pre_value == -1 || cond_effect_pre_value == value)
            add_transition(var_no, label_no, value, post_value);
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
                add_transition(var_no, label_no, value, value);
            }
        }
    }
    mark_as_relevant(var_no, label_no);
}

void FTSFactory::handle_operator_precondition(
    OperatorProxy op, FactProxy precondition,
    const vector<bool> &has_effect_on_var) {
    int label_no = op.get_id();
    int var_no = precondition.get_variable().get_id();
    if (!has_effect_on_var[var_no]) {
        int value = precondition.get_value();
        add_transition(var_no, label_no, value, value);
        mark_as_relevant(var_no, label_no);
    }
}

void FTSFactory::build_transitions_for_operator(OperatorProxy op) {
    /*
      - Add transitions induced by op.
      - Mark op as relevant in the appropriate transition systems.
    */
    unordered_map<int, int> pre_val = compute_preconditions(op);
    vector<bool> has_effect_on_var(task_proxy.get_variables().size(), false);

    for (EffectProxy effect : op.get_effects())
        handle_operator_effect(op, effect, pre_val, has_effect_on_var);

    // We must handle preconditions *after* effects because handling
    // the effects sets has_effect_on_var.
    for (FactProxy precondition : op.get_preconditions())
        handle_operator_precondition(op, precondition, has_effect_on_var);
}

void FTSFactory::build_transitions() {
    /*
      - Add transitions to the empty transition system objects.
      - Compute relevant labels.
    */
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators)
        build_transitions_for_operator(op);
}

void FTSFactory::finalize_transition_systems() {
    /*
      - Add transitions for irrelevant operators.
      - Normalize transition systems.
    */
    VariablesProxy variables = task_proxy.get_variables();
    for (VariableProxy variable : task_proxy.get_variables()) {
        size_t var_no = variable.get_id();
        int num_states = variable.get_domain_size();
        int num_labels = labels->get_size();
        /* Make all irrelevant labels explicit. */
        for (int label_no = 0; label_no < num_labels; ++label_no) {
            if (!is_relevant(var_no, label_no)) {
                for (int state = 0; state < num_states; ++state)
                    add_transition(var_no, label_no, state, state);
            }
        }

        // Copy all transitions into the TransitionSystem objects.
        /*
          Note: this is ugly and copies instead of moving because
          TransitionSystem is a bit peculiar about changing the identity
          of transitions_of_groups. We will fix this later when we get
          a constructor that can move the transitions.
        */
        TransitionSystemData &ts_data = transition_system_by_var[var_no];
        TransitionSystem *ts = transition_systems[var_no];
        for (int label_no = 0; label_no < num_labels; ++label_no) {
            ts->transitions_of_groups[label_no].insert(
                ts->transitions_of_groups[label_no].end(),
                ts_data.transitions_by_label[label_no].begin(),
                ts_data.transitions_by_label[label_no].end());
        }

        transition_systems[var_no]->hacky_finish_construction();
    }
}

FactoredTransitionSystem FTSFactory::create() {
    cout << "Building atomic transition systems... " << endl;
    assert(transition_systems.empty());

    build_labels();
    build_empty_transition_systems();
    build_transitions();
    finalize_transition_systems();

    return FactoredTransitionSystem(move(transition_systems));
}

FactoredTransitionSystem create_factored_transition_system(
    const TaskProxy &task_proxy, shared_ptr<Labels> labels) {
    return FTSFactory(task_proxy, labels).create();
}
