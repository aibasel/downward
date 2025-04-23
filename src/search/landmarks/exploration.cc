#include "exploration.h"

#include "util.h"

#include "../task_utils/task_properties.h"
#include "../utils/hash.h"
#include "../utils/logging.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace landmarks {
/*
  Implementation note: Compared to RelaxationHeuristic, we *cannot simplify*
  unary operators, because this may conflict with excluded operators.
  For an example, consider that unary operator o1 is thrown out during
  simplify() because it is dominated by unary operator o2, but then o2
  is excluded during an exploration ==> the shared effect of o1 and o2 is
  wrongly never reached in the exploration.
*/

// Construction and destruction
Exploration::Exploration(const TaskProxy &task_proxy, utils::LogProxy &log)
    : task_proxy(task_proxy) {
    if (log.is_at_least_normal()) {
        log << "Initializing Exploration..." << endl;
    }
    build_propositions();
    build_unary_operators();
}

void Exploration::build_propositions() {
    for (VariableProxy var : task_proxy.get_variables()) {
        int var_id = var.get_id();
        propositions.emplace_back(var.get_domain_size());
        for (int value = 0; value < var.get_domain_size(); ++value) {
            propositions[var_id][value].fact = FactPair(var_id, value);
        }
    }
}

static int compute_number_of_unary_operators(
    const OperatorsProxy &operators, const AxiomsProxy &axioms) {
    int num_unary_ops = 0;
    for (OperatorProxy op : operators) {
        num_unary_ops += static_cast<int>(op.get_effects().size());
    }
    for (OperatorProxy axiom : axioms) {
        num_unary_ops += static_cast<int>(axiom.get_effects().size());
    }
    return num_unary_ops;
}

void Exploration::build_unary_operators() {
    const OperatorsProxy &operators = task_proxy.get_operators();
    const AxiomsProxy &axioms = task_proxy.get_axioms();
    /*
      We need to reserve memory for this vector because we cross-reference to
      the memory address of its elements while building it, meaning a resize
      would invalidate all references.
    */
    unary_operators.reserve(
        compute_number_of_unary_operators(operators, axioms));

    for (const OperatorProxy &op : operators) {
        build_unary_operators(op);
    }
    for (const OperatorProxy &axiom : axioms) {
        build_unary_operators(axiom);
    }
}

static vector<FactPair> get_sorted_effect_conditions(
    const EffectProxy &effect) {
    vector<FactPair> effect_conditions;
    effect_conditions.reserve(effect.get_conditions().size());
    for (FactProxy effect_condition : effect.get_conditions()) {
        effect_conditions.push_back(effect_condition.get_pair());
    }
    sort(effect_conditions.begin(), effect_conditions.end());
    return effect_conditions;
}

static vector<FactPair> get_sorted_extended_preconditions(
    const vector<FactPair> &preconditions, const EffectProxy &effect) {
    /* Since this function is called with the same `preconditions` repeatedly,
       we expect them to be sorted to avoid sorting them over and over again. */
    assert(is_sorted(preconditions.begin(), preconditions.end()));
    vector<FactPair> effect_conditions = get_sorted_effect_conditions(effect);

    vector<FactPair> extended_preconditions(
        preconditions.size() + effect_conditions.size(), FactPair::no_fact);
    merge(preconditions.begin(), preconditions.end(), effect_conditions.begin(),
          effect_conditions.end(), extended_preconditions.begin());
    assert(is_sorted(
               extended_preconditions.begin(), extended_preconditions.end()));
    return extended_preconditions;
}

vector<Proposition *> Exploration::get_sorted_precondition_propositions(
    const vector<FactPair> &preconditions, const EffectProxy &effect) {
    vector<FactPair> extended_preconditions =
        get_sorted_extended_preconditions(preconditions, effect);
    vector<Proposition *> precondition_propositions;
    precondition_propositions.reserve(extended_preconditions.size());
    for (const FactPair &precondition_fact : extended_preconditions) {
        precondition_propositions.push_back(
            &propositions[precondition_fact.var][precondition_fact.value]);
    }
    return precondition_propositions;
}

void Exploration::build_unary_operators(const OperatorProxy &op) {
    vector<FactPair> preconditions;
    int op_or_axiom_id = get_operator_or_axiom_id(op);

    for (FactProxy pre : op.get_preconditions()) {
        preconditions.push_back(pre.get_pair());
    }
    sort(preconditions.begin(), preconditions.end());
    for (EffectProxy effect : op.get_effects()) {
        vector<Proposition *> precondition_propositions =
            get_sorted_precondition_propositions(preconditions, effect);
        auto [var, value] = effect.get_fact().get_pair();
        Proposition *effect_proposition = &propositions[var][value];
        unary_operators.emplace_back(
            precondition_propositions, effect_proposition, op_or_axiom_id);

        // Cross-reference unary operators.
        for (Proposition *pre : precondition_propositions) {
            pre->precondition_of.push_back(&unary_operators.back());
        }
    }
}

void Exploration::reset_reachability_information() {
    for (auto &propositions_for_variable : propositions) {
        for (auto &prop : propositions_for_variable) {
            prop.reached = false;
        }
    }
}

void Exploration::set_state_atoms_reached(const State &state) {
    for (const FactProxy &atom : state) {
        Proposition *init_prop =
            &propositions[atom.get_variable().get_id()][atom.get_value()];
        if (!init_prop->excluded) {
            enqueue_if_necessary(init_prop);
        }
    }
}

/*
  Unary operators derived from operators that are excluded or achieve an
  excluded proposition *unconditionally* must be marked as excluded.

  Note that we in general cannot exclude all unary operators derived from
  operators that achieve an excluded proposition *conditionally*:
  Given an operator with unconditional effect e1 and conditional effect e2 with
  condition c yields unary operators uo1: {} -> e1 and uo2: c -> e2. Excluding
  both would not allow us to achieve e1 when excluding proposition e2. We
  instead only mark uo2 as excluded (see in `initialize_operator_data` when
  looping over all unary operators). Note however that this can lead to an
  overapproximation, e.g. if the effect e1 also has condition c.
*/
unordered_set<int> Exploration::get_excluded_operators(
    bool use_unary_relaxation) const {
    /* When using unary relaxation, we only exclude unary operators but none
       of the original operators which have an undesired side effect. */
    if (use_unary_relaxation) {
        return unordered_set<int>{};
    }

    unordered_set<int> excluded_op_ids;
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            auto [var, value] = effect.get_fact().get_pair();
            if (effect.get_conditions().empty()
                && propositions[var][value].excluded) {
                excluded_op_ids.insert(op.get_id());
                break;
            }
        }
    }
    return excluded_op_ids;
}

void Exploration::initialize_operator_data(bool use_unary_relaxation) {
    unordered_set<int> excluded_op_ids =
        get_excluded_operators(use_unary_relaxation);

    for (UnaryOperator &op : unary_operators) {
        op.num_unsatisfied_preconditions = op.num_preconditions;

        /*
          Aside from UnaryOperators derived from operators with an id in
          `excluded_op_ids` we also exclude UnaryOperators that have an excluded
          proposition as effect (see comment for `get_excluded_operators`).
        */
        if (op.effect->excluded
            || excluded_op_ids.contains(op.op_or_axiom_id)) {
            // Operator will not be applied during relaxed exploration.
            op.excluded = true;
            continue;
        }
        op.excluded = false; // Reset from previous exploration.

        // Queue effects of precondition-free operators.
        if (op.num_unsatisfied_preconditions == 0) {
            enqueue_if_necessary(op.effect);
        }
    }
}

/*
  This function initializes the priority queue and the information associated
  with propositions and unary operators for the relaxed exploration. Unary
  operators that are not allowed to be applied due to exclusions are marked by
  setting their *excluded* flag.
*/
void Exploration::setup_exploration_queue(
    const State &state, const vector<FactPair> &excluded_props,
    bool use_unary_relaxation) {
    prop_queue.clear();
    reset_reachability_information();

    // Set *excluded* to true for initializing operator data.
    for (const FactPair &atom : excluded_props) {
        propositions[atom.var][atom.value].excluded = true;
    }

    set_state_atoms_reached(state);
    initialize_operator_data(use_unary_relaxation);

    // Reset *excluded* to false for the next exploration.
    for (const FactPair &atom : excluded_props) {
        propositions[atom.var][atom.value].excluded = false;
    }
}

void Exploration::relaxed_exploration() {
    while (!prop_queue.empty()) {
        Proposition *prop = prop_queue.front();
        prop_queue.pop_front();

        const vector<UnaryOperator *> &triggered_operators =
            prop->precondition_of;
        for (UnaryOperator *unary_op : triggered_operators) {
            if (unary_op->excluded) {
                continue;
            }
            --unary_op->num_unsatisfied_preconditions;
            assert(unary_op->num_unsatisfied_preconditions >= 0);
            if (unary_op->num_unsatisfied_preconditions == 0) {
                enqueue_if_necessary(unary_op->effect);
            }
        }
    }
}

void Exploration::enqueue_if_necessary(Proposition *prop) {
    if (!prop->reached) {
        prop->reached = true;
        prop_queue.push_back(prop);
    }
}

vector<vector<bool>> Exploration::bundle_reachability_information() const {
    vector<vector<bool>> reached;
    reached.resize(propositions.size());
    for (size_t var_id = 0; var_id < propositions.size(); ++var_id) {
        reached[var_id].resize(propositions[var_id].size(), false);
        for (size_t value = 0; value < propositions[var_id].size(); ++value) {
            if (propositions[var_id][value].reached) {
                reached[var_id][value] = true;
            }
        }
    }
    return reached;
}

vector<vector<bool>> Exploration::compute_relaxed_reachability(
    const vector<FactPair> &excluded_props, bool use_unary_relaxation) {
    setup_exploration_queue(task_proxy.get_initial_state(), excluded_props,
                            use_unary_relaxation);
    relaxed_exploration();
    return bundle_reachability_information();
}
}
