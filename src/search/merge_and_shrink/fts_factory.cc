#include "fts_factory.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "labels.h"
#include "merge_and_shrink_representation.h"
#include "transition_system.h"
#include "types.h"

#include "../task_proxy.h"

#include "../utils/collections.h"
#include "../utils/logging.h"

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

        vector<int> label_to_local_label;
        vector<LocalLabelInfo> local_label_infos;
        vector<bool> relevant_labels;
        int num_states;
        vector<bool> goal_states;
        int init_state;
        TransitionSystemData(TransitionSystemData &&other)
            : num_variables(other.num_variables),
              incorporated_variables(move(other.incorporated_variables)),
              label_to_local_label(move(other.label_to_local_label)),
              local_label_infos(move(other.local_label_infos)),
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

    unique_ptr<Labels> create_labels();
    void build_state_data(VariableProxy var);
    void initialize_transition_system_data(const Labels &labels);
    bool is_relevant(int var_id, int label) const;
    void mark_as_relevant(int var_id, int label);
    unordered_map<int, int> compute_preconditions(OperatorProxy op);
    void handle_operator_effect(
        OperatorProxy op,
        EffectProxy effect,
        const unordered_map<int, int> &pre_val,
        vector<bool> &has_effect_on_var,
        vector<vector<Transition>> &transitions_by_var);
    void handle_operator_precondition(
        OperatorProxy op,
        FactProxy precondition,
        const vector<bool> &has_effect_on_var,
        vector<vector<Transition>> &transitions_by_var);
    void build_transitions_for_operator(OperatorProxy op);
    void build_transitions_for_irrelevant_ops(
        const VariableProxy &variable, const Labels &labels);
    void build_transitions(const Labels &labels);
    vector<unique_ptr<TransitionSystem>> create_transition_systems(
        const Labels &labels);
    vector<unique_ptr<MergeAndShrinkRepresentation>> create_mas_representations() const;
    vector<unique_ptr<Distances>> create_distances(
        const vector<unique_ptr<TransitionSystem>> &transition_systems) const;
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
        utils::LogProxy &log);
};


FTSFactory::FTSFactory(const TaskProxy &task_proxy)
    : task_proxy(task_proxy), task_has_conditional_effects(false) {
}

FTSFactory::~FTSFactory() {
}

unique_ptr<Labels> FTSFactory::create_labels() {
    vector<int> label_costs;
    OperatorsProxy ops = task_proxy.get_operators();
    int num_ops = ops.size();
    int max_num_labels = 0;
    if (num_ops > 0) {
        max_num_labels = 2 * num_ops - 1;
        label_costs.reserve(max_num_labels);
        for (OperatorProxy op : ops) {
            label_costs.push_back(op.get_cost());
        }
    }
    return make_unique<Labels>(move(label_costs), max_num_labels);
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
    transition_system_data_by_var.resize(variables.size());
    for (VariableProxy var : variables) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var.get_id()];
        ts_data.num_variables = variables.size();
        ts_data.incorporated_variables.push_back(var.get_id());
        ts_data.label_to_local_label.resize(labels.get_max_num_labels(), -1);
        ts_data.relevant_labels.resize(labels.get_num_total_labels(), false);
        build_state_data(var);
    }
}

bool FTSFactory::is_relevant(int var_id, int label) const {
    return transition_system_data_by_var[var_id].relevant_labels[label];
}

void FTSFactory::mark_as_relevant(int var_id, int label) {
    transition_system_data_by_var[var_id].relevant_labels[label] = true;
}

unordered_map<int, int> FTSFactory::compute_preconditions(OperatorProxy op) {
    unordered_map<int, int> pre_val;
    for (FactProxy precondition : op.get_preconditions())
        pre_val[precondition.get_variable().get_id()] =
            precondition.get_value();
    return pre_val;
}

void FTSFactory::handle_operator_effect(
    OperatorProxy op,
    EffectProxy effect,
    const unordered_map<int, int> &pre_val,
    vector<bool> &has_effect_on_var,
    vector<vector<Transition>> &transitions_by_var) {
    int label = op.get_id();
    FactProxy fact = effect.get_fact();
    VariableProxy var = fact.get_variable();
    int var_id = var.get_id();
    has_effect_on_var[var_id] = true;
    int post_value = fact.get_value();

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
        /*
          Only add a transition if it is possible that the effect
          triggers. We can rule out that the effect triggers if it has
          a condition on var and this condition is not satisfied.
        */
        if (cond_effect_pre_value == -1 || cond_effect_pre_value == value)
            transitions_by_var[var_id].emplace_back(value, post_value);
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
                transitions_by_var[var_id].emplace_back(value, value);
        }
        task_has_conditional_effects = true;
    }
    mark_as_relevant(var_id, label);
}

void FTSFactory::handle_operator_precondition(
    OperatorProxy op,
    FactProxy precondition,
    const vector<bool> &has_effect_on_var,
    vector<vector<Transition>> &transitions_by_var) {
    int label = op.get_id();
    int var_id = precondition.get_variable().get_id();
    if (!has_effect_on_var[var_id]) {
        int value = precondition.get_value();
        transitions_by_var[var_id].emplace_back(value, value);
        mark_as_relevant(var_id, label);
    }
}

void FTSFactory::build_transitions_for_operator(OperatorProxy op) {
    /*
      - Mark op as relevant in the transition systems corresponding
        to variables on which it has a precondition or effect.
      - Add transitions induced by op in these transition systems.
    */
    unordered_map<int, int> pre_val = compute_preconditions(op);
    int num_variables = task_proxy.get_variables().size();
    vector<bool> has_effect_on_var(task_proxy.get_variables().size(), false);
    vector<vector<Transition>> transitions_by_var(num_variables);

    for (EffectProxy effect : op.get_effects())
        handle_operator_effect(op, effect, pre_val, has_effect_on_var, transitions_by_var);

    /*
      We must handle preconditions *after* effects because handling
      the effects sets has_effect_on_var.
    */
    for (FactProxy precondition : op.get_preconditions())
        handle_operator_precondition(op, precondition, has_effect_on_var, transitions_by_var);

    int label = op.get_id();
    int label_cost = op.get_cost();
    for (int var_id = 0; var_id < num_variables; ++var_id) {
        if (!is_relevant(var_id, label)) {
            /*
              We do not want to add transitions of irrelevant labels here,
              since they are handled together in a separate step.
            */
            continue;
        }
        vector<Transition> &transitions = transitions_by_var[var_id];
        /*
          TODO: Our method for generating transitions is only guarantueed
          to generate sorted and unique transitions if the task has no
          conditional effects.
        */
        if (task_has_conditional_effects) {
            utils::sort_unique(transitions);
        } else {
            assert(utils::is_sorted_unique(transitions));
        }

        vector<int> &label_to_local_label =
            transition_system_data_by_var[var_id].label_to_local_label;
        vector<LocalLabelInfo> &local_label_infos = transition_system_data_by_var[var_id].local_label_infos;
        bool found_locally_equivalent_label_group = false;
        for (size_t local_label = 0; local_label < local_label_infos.size(); ++local_label) {
            LocalLabelInfo &local_label_info = local_label_infos[local_label];
            const vector<Transition> &local_label_transitions = local_label_info.get_transitions();
            if (transitions == local_label_transitions) {
                assert(label_to_local_label[label] == -1);
                label_to_local_label[label] = local_label;
                local_label_info.add_label(label, label_cost);
                found_locally_equivalent_label_group = true;
                break;
            }
        }

        if (!found_locally_equivalent_label_group) {
            int new_local_label = local_label_infos.size();
            LabelGroup label_group = {label};
            local_label_infos.emplace_back(move(label_group), move(transitions), label_cost);
            assert(label_to_local_label[label] == -1);
            label_to_local_label[label] = new_local_label;
        }
    }
}

void FTSFactory::build_transitions_for_irrelevant_ops(
    const VariableProxy &variable, const Labels &labels) {
    int var_id = variable.get_id();
    int num_states = variable.get_domain_size();

    // Collect all irrelevant labels for this variable.
    LabelGroup irrelevant_labels;
    int cost = INF;
    for (int label : labels) {
        if (!is_relevant(var_id, label)) {
            irrelevant_labels.push_back(label);
            cost = min(cost, labels.get_label_cost(label));
        }
    }

    TransitionSystemData &ts_data = transition_system_data_by_var[var_id];
    if (!irrelevant_labels.empty()) {
        vector<Transition> transitions;
        transitions.reserve(num_states);
        for (int state = 0; state < num_states; ++state)
            transitions.emplace_back(state, state);
        int new_local_label = ts_data.local_label_infos.size();
        for (int label : irrelevant_labels) {
            assert(ts_data.label_to_local_label[label] == -1);
            ts_data.label_to_local_label[label] = new_local_label;
        }
        ts_data.local_label_infos.emplace_back(
            move(irrelevant_labels), move(transitions), cost);
    }
}

void FTSFactory::build_transitions(const Labels &labels) {
    /*
      - Compute all transitions of all operators for all variables, grouping
        transitions of locally equivalent labels for a given variable.
      - Computes relevant operator information as a side effect.
    */
    for (OperatorProxy op : task_proxy.get_operators())
        build_transitions_for_operator(op);

    /*
      Compute transitions of irrelevant operators for each variable only
      once and put the labels into a single label group.
    */
    for (VariableProxy variable : task_proxy.get_variables())
        build_transitions_for_irrelevant_ops(variable, labels);
}

vector<unique_ptr<TransitionSystem>> FTSFactory::create_transition_systems(const Labels &labels) {
    // Create the actual TransitionSystem objects.
    int num_variables = task_proxy.get_variables().size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<TransitionSystem>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_id = 0; var_id < num_variables; ++var_id) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var_id];
        result.push_back(make_unique<TransitionSystem>(
                             ts_data.num_variables,
                             move(ts_data.incorporated_variables),
                             labels,
                             move(ts_data.label_to_local_label),
                             move(ts_data.local_label_infos),
                             ts_data.num_states,
                             move(ts_data.goal_states),
                             ts_data.init_state
                             ));
    }
    return result;
}

vector<unique_ptr<MergeAndShrinkRepresentation>> FTSFactory::create_mas_representations() const {
    // Create the actual MergeAndShrinkRepresentation objects.
    int num_variables = task_proxy.get_variables().size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<MergeAndShrinkRepresentation>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_id = 0; var_id < num_variables; ++var_id) {
        int range = task_proxy.get_variables()[var_id].get_domain_size();
        result.push_back(
            make_unique<MergeAndShrinkRepresentationLeaf>(var_id, range));
    }
    return result;
}

vector<unique_ptr<Distances>> FTSFactory::create_distances(
    const vector<unique_ptr<TransitionSystem>> &transition_systems) const {
    // Create the actual Distances objects.
    int num_variables = task_proxy.get_variables().size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<Distances>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_id = 0; var_id < num_variables; ++var_id) {
        result.push_back(
            make_unique<Distances>(*transition_systems[var_id]));
    }
    return result;
}

FactoredTransitionSystem FTSFactory::create(
    const bool compute_init_distances,
    const bool compute_goal_distances,
    utils::LogProxy &log) {
    if (log.is_at_least_normal()) {
        log << "Building atomic transition systems... " << endl;
    }

    unique_ptr<Labels> labels = create_labels();

    initialize_transition_system_data(*labels);
    build_transitions(*labels);
    vector<unique_ptr<TransitionSystem>> transition_systems =
        create_transition_systems(*labels);
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
        log);
}

FactoredTransitionSystem create_factored_transition_system(
    const TaskProxy &task_proxy,
    const bool compute_init_distances,
    const bool compute_goal_distances,
    utils::LogProxy &log) {
    return FTSFactory(task_proxy).create(
        compute_init_distances,
        compute_goal_distances,
        log);
}
}
