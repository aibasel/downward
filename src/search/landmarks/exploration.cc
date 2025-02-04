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
        const int var_id = var.get_id();
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
    /*
      Reserve vector size unary operators. This is needed because we
      cross-reference to the memory address of elements of the vector while
      building it; meaning a resize would invalidate all references.
    */
    OperatorsProxy operators = task_proxy.get_operators();
    AxiomsProxy axioms = task_proxy.get_axioms();
    unary_operators.reserve(
        compute_number_of_unary_operators(operators, axioms));

    // Build unary operators for operators and axioms.
    for (OperatorProxy op : operators)
        build_unary_operators(op);
    for (OperatorProxy axiom : axioms)
        build_unary_operators(axiom);
}

vector<Proposition *> Exploration::get_sorted_precondition_propositions(
    const vector<FactPair> &preconditions, const EffectProxy &effect) {
    vector<FactPair> extended_preconditions(preconditions);
    const EffectConditionsProxy &effect_conditions = effect.get_conditions();
    for (FactProxy effect_condition : effect_conditions) {
        extended_preconditions.push_back(effect_condition.get_pair());
    }

    sort(extended_preconditions.begin(), extended_preconditions.end());

    vector<Proposition *> precondition_propositions;
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
    for (EffectProxy effect : op.get_effects()) {
        vector<Proposition *> precondition_propositions =
            get_sorted_precondition_propositions(preconditions, effect);
        FactProxy effect_fact = effect.get_fact();
        Proposition *effect_proposition = &propositions[
            effect_fact.get_variable().get_id()][effect_fact.get_value()];
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

void Exploration::set_state_facts_reached(const State &state) {
    for (FactProxy fact : state) {
        Proposition *init_prop =
            &propositions[fact.get_variable().get_id()][fact.get_value()];
        enqueue_if_necessary(init_prop);
    }
}

/*
  Unary operators derived from operators that are excluded or achieve an
  excluded proposition *unconditionally* must be marked as excluded.

  Note that we in general cannot exclude all unary operators derived from
  operators that achieve an excluded propositon *conditionally*:
  Given an operator with uncoditional effect e1 and conditional effect e2 with
  condition c yields unary operators uo1: {} -> e1 and uo2: c -> e2. Excluding
  both would not allow us to achieve e1 when excluding proposition e2. We
  instead only mark uo2 as excluded (see in *initialize_operator_data* when
  looping over all unary operators). Note however that this can lead to an
  overapproximation, e.g. if the effect e1 also has condition c.
*/
unordered_set<int> Exploration::get_excluded_operators(
    const vector<int> &excluded_op_ids) const {
    unordered_set<int> op_ids_to_mark(excluded_op_ids.begin(),
                                      excluded_op_ids.end());
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            if (effect.get_conditions().empty()
                && propositions[effect.get_fact().get_variable().get_id()]
                [effect.get_fact().get_value()].excluded) {
                op_ids_to_mark.insert(op.get_id());
                break;
            }
        }
    }
    return op_ids_to_mark;
}

void Exploration::initialize_operator_data(
    const vector<int> &excluded_op_ids) {
    const unordered_set<int> op_ids_to_mark =
        get_excluded_operators(excluded_op_ids);

    for (UnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.num_preconditions;

        /*
          Aside from UnaryOperators derived from operators with an id in
          op_ids_to_mark we also exclude UnaryOperators that have an excluded
          proposition as effect (see comment for *get_excluded_operators*).
        */
        if (op.effect->excluded || op_ids_to_mark.contains(op.op_or_axiom_id)) {
            // Operator will not be applied during relaxed exploration.
            op.excluded = true;
            continue;
        }
        op.excluded = false; // Reset from previous exploration.

        // Queue effects of precondition-free operators.
        if (op.unsatisfied_preconditions == 0) {
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
    const vector<int> &excluded_op_ids) {
    prop_queue.clear();

    reset_reachability_information();

    // Set *excluded* to true for initializing operator data.
    for (const FactPair &fact : excluded_props) {
        propositions[fact.var][fact.value].excluded = true;
    }

    set_state_facts_reached(state);
    initialize_operator_data(excluded_op_ids);

    // Reset *excluded* to false for the next exploration.
    for (const FactPair &fact : excluded_props) {
        propositions[fact.var][fact.value].excluded = false;
    }
}

void Exploration::relaxed_exploration() {
    while (!prop_queue.empty()) {
        Proposition *prop = prop_queue.front();
        prop_queue.pop_front();

        const vector<UnaryOperator *> &triggered_operators =
            prop->precondition_of;
        for (UnaryOperator *unary_op : triggered_operators) {
            if (unary_op->excluded)
                continue;
            --unary_op->unsatisfied_preconditions;
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
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
    const vector<FactPair> &excluded_props,
    const vector<int> &excluded_op_ids) {
    setup_exploration_queue(task_proxy.get_initial_state(),
                            excluded_props, excluded_op_ids);
    relaxed_exploration();
    return bundle_reachability_information();
}
}
