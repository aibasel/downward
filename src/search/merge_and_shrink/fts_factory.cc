#include "fts_factory.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "label_equivalence_relation.h"
#include "labels.h"
#include "merge_and_shrink_representation.h"
#include "transition_system.h"
#include "types.h"

#include "../task_proxy.h"

#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <vector>

using namespace std;

namespace merge_and_shrink {
class FTSFactory {
    const TaskProxy &task_proxy;

    struct TransitionSystemData {
        // The following two attributes are only used for statistics
        int num_variables;
        vector<int> incorporated_variables;

        unique_ptr<LabelEquivalenceRelation> label_equivalence_relation;
        vector<vector<Transition>> transitions_by_label;
        vector<bool> relevant_labels;
        int num_states;
        vector<bool> goal_states;
        int init_state;
        TransitionSystemData(TransitionSystemData &&other)
            : num_variables(other.num_variables),
              incorporated_variables(move(other.incorporated_variables)),
              label_equivalence_relation(move(other.label_equivalence_relation)),
              transitions_by_label(move(other.transitions_by_label)),
              relevant_labels(move(other.relevant_labels)),
              num_states(other.num_states),
              goal_states(move(other.goal_states)),
              init_state(other.init_state) {
        }
        TransitionSystemData() = default;
        TransitionSystemData(TransitionSystemData &other) = delete;
        TransitionSystemData &operator=(TransitionSystemData &other) = delete;
    };
    vector<TransitionSystemData> transition_system_data_by_var;
    // see TODO in build_transitions()
    int task_has_conditional_effects;

    vector<unique_ptr<Label>> create_labels();
    void build_label_equivalence_relation(LabelEquivalenceRelation &label_equivalence_relation);
    void build_state_data(VariableProxy var);
    void initialize_transition_system_data(const Labels &labels);
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
    void build_transitions_for_irrelevant_ops(VariableProxy variable);
    void build_transitions();
    vector<unique_ptr<TransitionSystem>> create_transition_systems();
    vector<unique_ptr<MergeAndShrinkRepresentation>> create_mas_representations();
    vector<unique_ptr<Distances>> create_distances(
        const vector<unique_ptr<TransitionSystem>> &transition_systems);
public:
    explicit FTSFactory(const TaskProxy &task_proxy);
    ~FTSFactory();

    /*
      Note: create() may only be called once. We don't worry about
      misuse because the class is only used internally in this file.
    */
    FactoredTransitionSystem create(
        bool compute_init_distances,
        bool compute_goal_distances,
        Verbosity verbosity);
};


FTSFactory::FTSFactory(const TaskProxy &task_proxy)
    : task_proxy(task_proxy), task_has_conditional_effects(false) {
}

FTSFactory::~FTSFactory() {
}

vector<unique_ptr<Label>> FTSFactory::create_labels() {
    vector<unique_ptr<Label>> result;
    for (OperatorProxy op : task_proxy.get_operators()) {
        result.push_back(utils::make_unique_ptr<Label>(op.get_cost()));
    }
    return result;
}

void FTSFactory::build_label_equivalence_relation(
    LabelEquivalenceRelation &label_equivalence_relation) {
    /*
      Prepare label_equivalence_relation data structure: add one single-element
      group for every operator.
    */
    int num_labels = task_proxy.get_operators().size();
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        // We use the label number as index for transitions of groups.
        label_equivalence_relation.add_label_group({label_no});
    }
}

void FTSFactory::build_state_data(VariableProxy var) {
    int var_id = var.get_id();
    TransitionSystemData &ts_data = transition_system_data_by_var[var_id];
    ts_data.init_state = task_proxy.get_initial_state()[var_id].get_value();

    int range = task_proxy.get_variables()[var_id].get_domain_size();
    ts_data.num_states = range;

    int goal_value = -1;
    GoalsProxy goals = task_proxy.get_goals();
    for (FactProxy goal : goals) {
        if (goal.get_variable().get_id() == var_id) {
            assert(goal_value == -1);
            goal_value = goal.get_value();
            break;
        }
    }

    ts_data.goal_states.resize(range, false);
    for (int value = 0; value < range; ++value) {
        if (value == goal_value || goal_value == -1) {
            ts_data.goal_states[value] = true;
        }
    }
}

void FTSFactory::initialize_transition_system_data(const Labels &labels) {
    VariablesProxy variables = task_proxy.get_variables();
    int num_labels = task_proxy.get_operators().size();
    transition_system_data_by_var.resize(variables.size());
    for (VariableProxy var : variables) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var.get_id()];
        ts_data.num_variables = variables.size();
        ts_data.incorporated_variables.push_back(var.get_id());
        ts_data.label_equivalence_relation = utils::make_unique_ptr<LabelEquivalenceRelation>(labels);
        build_label_equivalence_relation(*ts_data.label_equivalence_relation);
        ts_data.transitions_by_label.resize(labels.get_max_size());
        ts_data.relevant_labels.resize(num_labels, false);
        build_state_data(var);
    }
}

void FTSFactory::add_transition(int var_no, int label_no,
                                int src_value, int dest_value) {
    transition_system_data_by_var[var_no].transitions_by_label[label_no].push_back(
        Transition(src_value, dest_value));
}

bool FTSFactory::is_relevant(int var_no, int label_no) const {
    return transition_system_data_by_var[var_no].relevant_labels[label_no];
}

void FTSFactory::mark_as_relevant(int var_no, int label_no) {
    transition_system_data_by_var[var_no].relevant_labels[label_no] = true;
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
        /*
          Only add a transition if it is possible that the effect
          triggers. We can rule out that the effect triggers if it has
          a condition on var and this condition is not satisfied.
        */
        if (cond_effect_pre_value == -1 || cond_effect_pre_value == value)
            add_transition(var_no, label_no, value, post_value);
    }

    // Handle transitions that occur when the effect does not trigger.
    if (!effect_conditions.empty()) {
        for (int value = pre_value_min; value < pre_value_max; ++value) {
            /*
              Add self-loop if the effect might not trigger.
              If the effect has a condition on another variable, then
              it can fail to trigger no matter which value var has.
              If it only has a condition on var, then the effect
              fails to trigger if this condition is false.
            */
            if (has_other_effect_cond || value != cond_effect_pre_value)
                add_transition(var_no, label_no, value, value);
        }
        task_has_conditional_effects = true;
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
      - Mark op as relevant in the transition systems corresponding
        to variables on which it has a precondition or effect.
      - Add transitions induced by op in these transition systems.
    */
    unordered_map<int, int> pre_val = compute_preconditions(op);
    vector<bool> has_effect_on_var(task_proxy.get_variables().size(), false);

    for (EffectProxy effect : op.get_effects())
        handle_operator_effect(op, effect, pre_val, has_effect_on_var);

    /*
      We must handle preconditions *after* effects because handling
      the effects sets has_effect_on_var.
    */
    for (FactProxy precondition : op.get_preconditions())
        handle_operator_precondition(op, precondition, has_effect_on_var);
}

void FTSFactory::build_transitions_for_irrelevant_ops(VariableProxy variable) {
    int var_no = variable.get_id();
    int num_states = variable.get_domain_size();
    int num_labels = task_proxy.get_operators().size();

    // Make all irrelevant labels explicit.
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        if (!is_relevant(var_no, label_no)) {
            for (int state = 0; state < num_states; ++state)
                add_transition(var_no, label_no, state, state);
        }
    }
}

void FTSFactory::build_transitions() {
    /*
      - Add all transitions.
      - Computes relevant operator information as a side effect.
    */
    for (OperatorProxy op : task_proxy.get_operators())
        build_transitions_for_operator(op);

    for (VariableProxy variable : task_proxy.get_variables())
        build_transitions_for_irrelevant_ops(variable);

    if (task_has_conditional_effects) {
        /*
          TODO: Our method for generating transitions is only guarantueed
          to generate sorted and unique transitions if the task has no
          conditional effects. We could replace the instance variable by
          a call to has_conditional_effects(task_proxy).
          Generally, the questions is whether we rely on sorted transitions
          anyway.
        */
        int num_variables = task_proxy.get_variables().size();
        for (int var_no = 0; var_no < num_variables; ++var_no) {
            vector<vector<Transition>> &transitions_by_label =
                transition_system_data_by_var[var_no].transitions_by_label;
            for (vector<Transition> &transitions : transitions_by_label) {
                sort(transitions.begin(), transitions.end());
                transitions.erase(unique(transitions.begin(),
                                         transitions.end()),
                                  transitions.end());
            }
        }
    }
}

vector<unique_ptr<TransitionSystem>> FTSFactory::create_transition_systems() {
    // Create the actual TransitionSystem objects.
    int num_variables = task_proxy.get_variables().size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<TransitionSystem>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    const bool compute_label_equivalence_relation = true;
    for (int var_no = 0; var_no < num_variables; ++var_no) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var_no];
        result.push_back(utils::make_unique_ptr<TransitionSystem>(
                             ts_data.num_variables,
                             move(ts_data.incorporated_variables),
                             move(ts_data.label_equivalence_relation),
                             move(ts_data.transitions_by_label),
                             ts_data.num_states,
                             move(ts_data.goal_states),
                             ts_data.init_state,
                             compute_label_equivalence_relation
                             ));
    }
    return result;
}

vector<unique_ptr<MergeAndShrinkRepresentation>> FTSFactory::create_mas_representations() {
    // Create the actual MergeAndShrinkRepresentation objects.
    int num_variables = task_proxy.get_variables().size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<MergeAndShrinkRepresentation>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        int range = task_proxy.get_variables()[var_no].get_domain_size();
        result.push_back(
            utils::make_unique_ptr<MergeAndShrinkRepresentationLeaf>(var_no, range));
    }
    return result;
}

vector<unique_ptr<Distances>> FTSFactory::create_distances(
    const vector<unique_ptr<TransitionSystem>> &transition_systems) {
    // Create the actual Distances objects.
    int num_variables = task_proxy.get_variables().size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<Distances>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        result.push_back(
            utils::make_unique_ptr<Distances>(*transition_systems[var_no]));
    }
    return result;
}

FactoredTransitionSystem FTSFactory::create(
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity) {
    if (verbosity >= Verbosity::NORMAL) {
        cout << "Building atomic transition systems... " << endl;
    }

    unique_ptr<Labels> labels = utils::make_unique_ptr<Labels>(create_labels());

    initialize_transition_system_data(*labels);
    build_transitions();
    vector<unique_ptr<TransitionSystem>> transition_systems =
        create_transition_systems();
    vector<unique_ptr<MergeAndShrinkRepresentation>> mas_representations =
        create_mas_representations();
    vector<unique_ptr<Distances>> distances =
        create_distances(transition_systems);

    return FactoredTransitionSystem(
        move(labels),
        move(transition_systems),
        move(mas_representations),
        move(distances),
        compute_init_distances,
        compute_goal_distances,
        verbosity);
}

FactoredTransitionSystem create_factored_transition_system(
    const TaskProxy &task_proxy,
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity) {
    return FTSFactory(task_proxy).create(
        compute_init_distances,
        compute_goal_distances,
        verbosity);
}
}
