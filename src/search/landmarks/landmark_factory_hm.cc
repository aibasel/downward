#include "landmark_factory_hm.h"

#include <numeric>

#include "exploration.h"
#include "landmark.h"

#include "../abstract_task.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/system.h"

#include <ranges>
#include <set>
#include <unordered_set>

using namespace std;
using utils::ExitCode;

namespace landmarks {
static void set_intersection(vector<int> &set1, const vector<int> &set2) {
    assert(ranges::is_sorted(set1));
    assert(ranges::is_sorted(set2));
    vector<int> result;
    ranges::set_intersection(set1, set2, back_inserter(result));
    swap(set1, result);
}

static void set_difference(vector<int> &set1, const vector<int> &set2) {
    assert(ranges::is_sorted(set1));
    assert(ranges::is_sorted(set2));
    vector<int> result;
    ranges::set_difference(set1, set2, inserter(result, result.begin()));
    swap(set1, result);
}

static bool are_mutex(const VariablesProxy &variables,
                      const FactPair &atom1, const FactPair &atom2) {
    return variables[atom1.var].get_fact(atom1.value).is_mutex(
        variables[atom2.var].get_fact(atom2.value));
}

void LandmarkFactoryHM::get_m_sets_including_current_var(
    const VariablesProxy &variables, int num_included, int current_var,
    Propositions &current, vector<Propositions> &subsets) {
    int domain_size = variables[current_var].get_domain_size();
    for (int value = 0; value < domain_size; ++value) {
        FactPair atom(current_var, value);
        bool use_var = ranges::none_of(current, [&](const FactPair &other) {
                                           return are_mutex(variables, atom, other);
                                       });
        if (use_var) {
            current.push_back(atom);
            get_m_sets(variables, num_included + 1, current_var + 1,
                       current, subsets);
            current.pop_back();
        }
    }
}

// Find partial variable assignments of size m or less.
void LandmarkFactoryHM::get_m_sets(
    const VariablesProxy &variables, int num_included, int current_var,
    Propositions &current, vector<Propositions> &subsets) {
    if (num_included == m) {
        subsets.push_back(current);
        return;
    }
    if (current_var == static_cast<int>(variables.size())) {
        if (num_included != 0) {
            subsets.push_back(current);
        }
        return;
    }
    get_m_sets_including_current_var(
        variables, num_included, current_var, current, subsets);
    // Do not include a value of `current_var` in the set.
    get_m_sets(variables, num_included, current_var + 1, current, subsets);
}

void LandmarkFactoryHM::get_m_sets_of_set_including_current_proposition(
    const VariablesProxy &variables, int num_included,
    int current_index, Propositions &current,
    vector<Propositions> &subsets, const Propositions &superset) {
    const FactPair &atom = superset[current_index];
    bool use_proposition = ranges::none_of(current, [&](const FactPair &other) {
                                               return are_mutex(variables, atom, other);
                                           });
    if (use_proposition) {
        current.push_back(atom);
        get_m_sets_of_set(variables, num_included + 1, current_index + 1,
                          current, subsets, superset);
        current.pop_back();
    }
}

// Find all subsets of `superset` with size m or less.
void LandmarkFactoryHM::get_m_sets_of_set(
    const VariablesProxy &variables, int num_included, int current_index,
    Propositions &current, vector<Propositions> &subsets,
    const Propositions &superset) {
    if (num_included == m) {
        subsets.push_back(current);
        return;
    }
    if (current_index == static_cast<int>(superset.size())) {
        if (num_included != 0) {
            subsets.push_back(current);
        }
        return;
    }
    get_m_sets_of_set_including_current_proposition(
        variables, num_included, current_index, current, subsets, superset);
    // Do not include proposition at `current_index` in set.
    get_m_sets_of_set(variables, num_included, current_index + 1,
                      current, subsets, superset);
}

void LandmarkFactoryHM::get_split_m_sets_including_current_proposition_from_first(
    const VariablesProxy &variables, int num_included1, int num_included2,
    int current_index1, int current_index2, Propositions &current,
    vector<Propositions> &subsets, const Propositions &superset1,
    const Propositions &superset2) {
    const FactPair &atom = superset1[current_index1];
    bool use_proposition = ranges::none_of(current, [&](const FactPair &other) {
                                               return are_mutex(variables, atom, other);
                                           });
    if (use_proposition) {
        current.push_back(atom);
        get_split_m_sets(variables, num_included1 + 1, num_included2,
                         current_index1 + 1, current_index2,
                         current, subsets, superset1, superset2);
        current.pop_back();
    }
}

/* Get subsets of `superset1` \cup `superset2` with size m or less, such
   that all subsets have >= 1 elements from each superset. */
void LandmarkFactoryHM::get_split_m_sets(
    const VariablesProxy &variables, int num_included1, int num_included2,
    int current_index1, int current_index2,
    Propositions &current, vector<Propositions> &subsets,
    const Propositions &superset1, const Propositions &superset2) {
    int superset1_size = static_cast<int>(superset1.size());
    int superset2_size = static_cast<int>(superset2.size());
    assert(superset1_size > 0);
    assert(superset2_size > 0);

    if (num_included1 + num_included2 == m ||
        (current_index1 == superset1_size &&
         current_index2 == superset2_size)) {
        if (num_included1 > 0 && num_included2 > 0) {
            subsets.push_back(current);
        }
        return;
    }

    if (current_index1 != superset1_size &&
        (current_index2 == superset2_size ||
         superset1[current_index1] < superset2[current_index2])) {
        get_split_m_sets_including_current_proposition_from_first(
            variables, num_included1, num_included2, current_index1,
            current_index2, current, subsets, superset1, superset2);
        // Do not include proposition at `current_index1` in set.
        get_split_m_sets(
            variables, num_included1, num_included2, current_index1 + 1,
            current_index2, current, subsets, superset1, superset2);
    } else {
        /*
          Switching order of 1 and 2 here to avoid code duplication in the form
          of a function `get_split_m_sets_including_current_proposition_from_second`
          analogous to `get_split_m_sets_including_current_proposition_from_first`.
        */
        get_split_m_sets_including_current_proposition_from_first(
            variables, num_included2, num_included1, current_index2,
            current_index1, current, subsets, superset2, superset1);
        // Do not include proposition at `current_index2` in set.
        get_split_m_sets(
            variables, num_included1, num_included2, current_index1,
            current_index2 + 1, current, subsets, superset1, superset2);
    }
}

// Get partial assignments of size <= m in the problem.
vector<Propositions> LandmarkFactoryHM::get_m_sets(
    const VariablesProxy &variables) {
    Propositions c;
    vector<Propositions> subsets;
    get_m_sets(variables, 0, 0, c, subsets);
    return subsets;
}

// Get subsets of `superset` with size <= m.
vector<Propositions> LandmarkFactoryHM::get_m_sets(
    const VariablesProxy &variables, const Propositions &superset) {
    Propositions c;
    vector<Propositions> subsets;
    get_m_sets_of_set(variables, 0, 0, c, subsets, superset);
    return subsets;
}

#ifndef NDEBUG
static bool proposition_variables_disjoint(
    const Propositions &set1, const Propositions &set2) {
    for (auto [var1, val1] : set1) {
        for (auto [var2, val2] : set2) {
            if (var1 == var2) {
                return false;
            }
        }
    }
    return true;
}
#endif

/*
  Get subsets of size <= m such that at least one element from `superset1` and
  at least one element from `superset2` are included, except if a sets is empty.
  We assume the variables in `superset1` and `superset2` are disjoint.
*/
vector<Propositions> LandmarkFactoryHM::get_split_m_sets(
    const VariablesProxy &variables,
    const Propositions &superset1, const Propositions &superset2) {
    assert(proposition_variables_disjoint(superset1, superset2));
    Propositions c;
    vector<Propositions> subsets;
    // If a set is empty, we do not have to include from it.
    if (superset1.empty()) {
        get_m_sets_of_set(variables, 0, 0, c, subsets, superset2);
    } else if (superset2.empty()) {
        get_m_sets_of_set(variables, 0, 0, c, subsets, superset1);
    } else {
        get_split_m_sets(
            variables, 0, 0, 0, 0, c, subsets, superset1, superset2);
    }
    return subsets;
}

// Get subsets of the propositions true in `state` with size <= m.
vector<Propositions> LandmarkFactoryHM::get_m_sets(
    const VariablesProxy &variables, const State &state) {
    Propositions state_propositions;
    state_propositions.reserve(state.size());
    for (FactProxy fact : state) {
        state_propositions.push_back(fact.get_pair());
    }
    return get_m_sets(variables, state_propositions);
}

void LandmarkFactoryHM::print_proposition(
    const VariablesProxy &variables, const FactPair &proposition) const {
    if (log.is_at_least_verbose()) {
        VariableProxy var = variables[proposition.var];
        FactProxy atom = var.get_fact(proposition.value);
        log << atom.get_name() << " ("
            << var.get_name() << "(" << atom.get_variable().get_id() << ")"
            << "->" << atom.get_value() << ")";
    }
}

static Propositions get_operator_precondition(const OperatorProxy &op) {
    Propositions preconditions =
        task_properties::get_fact_pairs(op.get_preconditions());
    sort(preconditions.begin(), preconditions.end());
    return preconditions;
}

/* Get atoms that are always true after the application of `op`
   (effects plus prevail conditions). */
static Propositions get_operator_postcondition(
    int num_vars, const OperatorProxy &op) {
    Propositions postconditions;
    EffectsProxy effects = op.get_effects();
    vector<bool> has_effect_on_var(num_vars, false);

    for (EffectProxy effect : effects) {
        FactPair atom = effect.get_fact().get_pair();
        postconditions.push_back(atom);
        has_effect_on_var[atom.var] = true;
    }

    for (FactProxy precondition : op.get_preconditions()) {
        if (!has_effect_on_var[precondition.get_variable().get_id()]) {
            postconditions.push_back(precondition.get_pair());
        }
    }

    sort(postconditions.begin(), postconditions.end());
    return postconditions;
}

static set<FactPair> get_propositions(
    const vector<int> &collection, const vector<HMEntry> &hm_table) {
    set<FactPair> propositions;
    for (int element : collection) {
        for (const FactPair &proposition : hm_table[element].propositions) {
            propositions.insert(proposition);
        }
    }
    return propositions;
}

void LandmarkFactoryHM::print_pm_operator(
    const VariablesProxy &variables, const PiMOperator &op) const {
    if (log.is_at_least_verbose()) {
        vector<pair<set<FactPair>, set<FactPair>>> conditions;
        for (const auto &conditional_noop : op.conditional_noops) {
            print_conditional_noop(variables, conditional_noop, conditions);
        }
        print_action(variables, op, conditions);
    }
}

void LandmarkFactoryHM::print_conditional_noop(
    const VariablesProxy &variables, const ConditionalNoop &conditional_noop,
    vector<pair<set<FactPair>, set<FactPair>>> &conditions) const {
    set<FactPair> effect_condition_set =
        print_effect_condition(variables, conditional_noop.effect_condition);
    set<FactPair> effect_set =
        print_conditional_effect(variables, conditional_noop.effect);
    conditions.emplace_back(effect_condition_set, effect_set);
    log << endl << endl << endl;
}

void LandmarkFactoryHM::print_proposition_set(
    const VariablesProxy &variables, const Propositions &propositions) const {
    if (log.is_at_least_verbose()) {
        log << "( ";
        for (const FactPair &fact : propositions) {
            print_proposition(variables, fact);
            log << " ";
        }
        log << ")";
    }
}

set<FactPair> LandmarkFactoryHM::print_effect_condition(
    const VariablesProxy &variables, const vector<int> &effect_conditions) const {
    set<FactPair> effect_condition_set;
    log << "effect conditions:\n";
    for (int effect_condition : effect_conditions) {
        print_proposition_set(
            variables, hm_table[effect_condition].propositions);
        log << endl;
        for (auto proposition : hm_table[effect_condition].propositions) {
            effect_condition_set.insert(proposition);
        }
    }
    return effect_condition_set;
}

set<FactPair> LandmarkFactoryHM::print_conditional_effect(
    const VariablesProxy &variables, const vector<int> &effect) const {
    set<FactPair> effect_set;
    log << "effect:\n";
    for (int eff : effect) {
        print_proposition_set(variables, hm_table[eff].propositions);
        log << endl;
        for (auto proposition : hm_table[eff].propositions) {
            effect_set.insert(proposition);
        }
    }
    return effect_set;
}

void LandmarkFactoryHM::print_action(
    const VariablesProxy &variables, const PiMOperator &op,
    const vector<pair<set<FactPair>, set<FactPair>>> &conditions) const {
    log << "Action " << op.id << endl;
    log << "Precondition: ";
    set<FactPair> preconditions = get_propositions(op.precondition, hm_table);
    for (const FactPair &precondition : preconditions) {
        print_proposition(variables, precondition);
        log << " ";
    }

    log << endl << "Effect: ";
    set<FactPair> effects = get_propositions(op.effect, hm_table);
    for (const FactPair &effect : effects) {
        print_proposition(variables, effect);
        log << " ";
    }
    log << endl << "Conditionals: " << endl;
    int i = 0;
    for (const auto &[effect_conditions, effects] : conditions) {
        log << "Effect Condition #" << i++ << ":\n\t";
        for (const FactPair &condition : effect_conditions) {
            print_proposition(variables, condition);
            log << " ";
        }
        log << endl << "Conditional Effect #" << i << ":\n\t";
        for (const FactPair &effect : effects) {
            print_proposition(variables, effect);
            log << " ";
        }
        log << endl << endl;
    }
}

static bool proposition_set_variables_disjoint(
    const Propositions &propositions1, const Propositions &propositions2) {
    auto it1 = propositions1.begin();
    auto it2 = propositions2.begin();
    while (it1 != propositions1.end() && it2 != propositions2.end()) {
        if (it1->var == it2->var) {
            return false;
        } else if (it1->var < it2->var) {
            ++it1;
        } else {
            ++it2;
        }
    }
    return true;
}

static bool proposition_sets_are_mutex(
    const VariablesProxy &variables, const Propositions &propositions1,
    const Propositions &propositions2) {
    for (const FactPair &atom1 : propositions1) {
        for (const FactPair &atom2 : propositions2) {
            if (are_mutex(variables, atom1, atom2)) {
                return false;
            }
        }
    }
    return true;
}

Propositions LandmarkFactoryHM::initialize_preconditions(
    const VariablesProxy &variables, const OperatorProxy &op,
    PiMOperator &pm_op) {
    /* All subsets of the original precondition are preconditions of the
       P_m operator. */
    Propositions precondition = get_operator_precondition(op);
    vector<Propositions> subsets = get_m_sets(variables, precondition);
    pm_op.precondition.reserve(subsets.size());

    num_unsatisfied_preconditions[op.get_id()].first =
        static_cast<int>(subsets.size());

    for (const Propositions &subset : subsets) {
        assert(set_indices.contains(subset));
        int set_index = set_indices[subset];
        pm_op.precondition.push_back(set_index);
        hm_table[set_index].triggered_operators.emplace_back(op.get_id(), -1);
    }
    return precondition;
}

Propositions LandmarkFactoryHM::initialize_postconditions(
    const VariablesProxy &variables, const OperatorProxy &op,
    PiMOperator &pm_op) {
    Propositions postcondition = get_operator_postcondition(
        static_cast<int>(variables.size()), op);
    vector<Propositions> subsets = get_m_sets(variables, postcondition);
    pm_op.effect.reserve(subsets.size());

    for (const Propositions &subset : subsets) {
        assert(set_indices.contains(subset));
        int set_index = set_indices[subset];
        pm_op.effect.push_back(set_index);
    }
    return postcondition;
}

vector<int> LandmarkFactoryHM::compute_noop_precondition(
    const vector<Propositions> &preconditions, int op_id, int noop_index) {
    vector<int> noop_condition;
    noop_condition.reserve(preconditions.size());
    for (const auto &subset : preconditions) {
        assert(static_cast<int>(subset.size()) <= m);
        assert(set_indices.contains(subset));
        int set_index = set_indices[subset];
        noop_condition.push_back(set_index);
        // These propositions are "conditional preconditions" for this operator.
        hm_table[set_index].triggered_operators.emplace_back(op_id, noop_index);
    }
    return noop_condition;
}

vector<int> LandmarkFactoryHM::compute_noop_effect(
    const vector<Propositions> &postconditions) {
    vector<int> noop_effect;
    noop_effect.reserve(postconditions.size());
    for (const auto &subset: postconditions) {
        assert(static_cast<int>(subset.size()) <= m);
        assert(set_indices.contains(subset));
        int set_index = set_indices[subset];
        noop_effect.push_back(set_index);
    }
    return noop_effect;
}

void LandmarkFactoryHM::add_conditional_noop(
    PiMOperator &pm_op, int op_id,
    const VariablesProxy &variables, const Propositions &propositions,
    const Propositions &preconditions, const Propositions &postconditions) {
    int noop_index = static_cast<int>(pm_op.conditional_noops.size());

    /*
      Get the subsets that have >= 1 element in the precondition (unless
      the precondition is empty) or the postcondition and >= 1 element
      in the `propositions` set.
    */
    vector<Propositions> noop_preconditions_subsets =
        get_split_m_sets(variables, preconditions, propositions);
    vector<Propositions> noop_postconditions_subsets =
        get_split_m_sets(variables, postconditions, propositions);

    num_unsatisfied_preconditions[op_id].second.push_back(
        static_cast<int>(noop_preconditions_subsets.size()));
    vector<int> noop_condition = compute_noop_precondition(
        noop_preconditions_subsets, op_id, noop_index);
    vector<int> noop_effect = compute_noop_effect(noop_postconditions_subsets);
    pm_op.conditional_noops.emplace_back(
        move(noop_condition), move(noop_effect));
}

void LandmarkFactoryHM::initialize_noops(
    const VariablesProxy &variables, PiMOperator &pm_op, int op_id,
    const Propositions &preconditions, const Propositions &postconditions) {
    /*
      For all subsets used in the problem with size *<* m, check whether
      they conflict with the postcondition of the operator. (No need to
      check the precondition because variables appearing in the precondition
      also appear in the postcondition.)
    */
    for (const Propositions &propositions : views::keys(set_indices)) {
        if (static_cast<int>(propositions.size()) >= m) {
            break;
        }
        if (proposition_set_variables_disjoint(postconditions, propositions)
            && proposition_sets_are_mutex(variables, postconditions,
                                          propositions)) {
            // For each such set, add a "conditional effect" to the operator.
            add_conditional_noop(pm_op, op_id, variables,
                                 propositions, preconditions, postconditions);
        }
    }
    pm_op.conditional_noops.shrink_to_fit();
}

void LandmarkFactoryHM::build_pm_operators(const TaskProxy &task_proxy) {
    OperatorsProxy operators = task_proxy.get_operators();
    int num_operators = static_cast<int>(operators.size());
    pm_operators.resize(num_operators);
    num_unsatisfied_preconditions.resize(num_operators);

    VariablesProxy variables = task_proxy.get_variables();

    /* Transfer operators from original problem.
       Represent noops as conditional effects. */
    for (int i = 0; i < num_operators; ++i) {
        const OperatorProxy &op = operators[i];
        PiMOperator &pm_op = pm_operators[op.get_id()];
        pm_op.id = i;

        Propositions preconditions =
            initialize_preconditions(variables, op, pm_op);
        Propositions postconditions =
            initialize_postconditions(variables, op, pm_op);
        initialize_noops(
            variables, pm_op, op.get_id(), preconditions, postconditions);
        print_pm_operator(variables, pm_op);
    }
}

LandmarkFactoryHM::LandmarkFactoryHM(
    int m, bool conjunctive_landmarks, bool use_orders,
    utils::Verbosity verbosity)
    : LandmarkFactory(verbosity),
      m(m),
      conjunctive_landmarks(conjunctive_landmarks),
      use_orders(use_orders) {
}

void LandmarkFactoryHM::initialize_hm_table(const VariablesProxy &variables) {
    // Get all sets of size m or less in the problem.
    vector<vector<FactPair>> msets = get_m_sets(variables);

    // Map each set to an integer.
    for (int i = 0; i < static_cast<int>(msets.size()); ++i) {
        set_indices[msets[i]] = i;
        hm_table.emplace_back(move(msets[i]));
    }
}

void LandmarkFactoryHM::initialize(const TaskProxy &task_proxy) {
    if (log.is_at_least_normal()) {
        log << "h^m landmarks m=" << m << endl;
    }
    if (!task_proxy.get_axioms().empty()) {
        cerr << "h^m landmarks don't support axioms" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
    initialize_hm_table(task_proxy.get_variables());
    if (log.is_at_least_normal()) {
        log << "Using " << hm_table.size() << " P^m propositions." << endl;
    }
    build_pm_operators(task_proxy);
}

void LandmarkFactoryHM::postprocess(const TaskProxy &task_proxy) {
    if (!conjunctive_landmarks)
        discard_conjunctive_landmarks();
    landmark_graph->set_landmark_ids();
    calc_achievers(task_proxy);
}

void LandmarkFactoryHM::discard_conjunctive_landmarks() {
    if (landmark_graph->get_num_conjunctive_landmarks() == 0) {
        return;
    }
    if (log.is_at_least_normal()) {
        log << "Discarding " << landmark_graph->get_num_conjunctive_landmarks()
            << " conjunctive landmarks" << endl;
    }
    landmark_graph->remove_node_if(
        [](const LandmarkNode &node) {
            return node.get_landmark().type == CONJUNCTIVE;
        });
}

static bool operator_can_achieve_landmark(
    const OperatorProxy &op, const Landmark &landmark,
    const VariablesProxy &variables) {
    Propositions precondition = get_operator_precondition(op);
    Propositions postcondition =
        get_operator_postcondition(static_cast<int>(variables.size()), op);

    for (const FactPair &atom : landmark.atoms) {
        if (ranges::find(postcondition, atom) != postcondition.end()) {
            // This `atom` is a postcondition of `op`, move on to the next one.
            continue;
        }
        auto mutex = [&](const FactPair &other) {
                return are_mutex(variables, atom, other);
            };
        if (ranges::any_of(postcondition, mutex)) {
            return false;
        }
        if (ranges::any_of(precondition, mutex)) {
            return false;
        }
    }
    return true;
}

void LandmarkFactoryHM::approximate_possible_achievers(
    Landmark &landmark, const OperatorsProxy &operators,
    const VariablesProxy &variables) const {
    unordered_set<int> candidates;
    for (const FactPair &atom : landmark.atoms) {
        const vector<int> &ops = get_operators_including_effect(atom);
        candidates.insert(ops.begin(), ops.end());
    }

    for (int op_id : candidates) {
        if (operator_can_achieve_landmark(
                operators[op_id], landmark, variables)) {
            landmark.possible_achievers.insert(op_id);
        }
    }
}

void LandmarkFactoryHM::calc_achievers(const TaskProxy &task_proxy) {
    assert(!achievers_calculated);
    if (log.is_at_least_normal()) {
        log << "Calculating achievers." << endl;
    }

    OperatorsProxy operators = task_proxy.get_operators();
    VariablesProxy variables = task_proxy.get_variables();
    /* The `first_achievers` are already filled in by `compute_hm_landmarks`,
       so here we only have to do `possible_achievers` */
    for (const auto &node : *landmark_graph) {
        Landmark &landmark = node->get_landmark();
        approximate_possible_achievers(landmark, operators, variables);
    }
    achievers_calculated = true;
}

void LandmarkFactoryHM::free_unneeded_memory() {
    utils::release_vector_memory(hm_table);
    utils::release_vector_memory(pm_operators);
    utils::release_vector_memory(num_unsatisfied_preconditions);

    set_indices.clear();
    landmark_nodes.clear();
}

void LandmarkFactoryHM::trigger_operator(
    int op_id, bool newly_discovered, TriggerSet &trigger) {
    if (newly_discovered) {
        --num_unsatisfied_preconditions[op_id].first;
    }
    if (num_unsatisfied_preconditions[op_id].first == 0) {
        /*
          Clear trigger for `op_id` (or create entry if it does not yet
          exist) to indicate that the precondition of the corresponding
          operator is satisfied and all conditional noops are triggered.
        */
        trigger[op_id].clear();
    }
}

void LandmarkFactoryHM::trigger_conditional_noop(
    int op_id, int noop_id, bool newly_discovered, TriggerSet &trigger) {
    if (newly_discovered) {
        --num_unsatisfied_preconditions[op_id].second[noop_id];
    }
    /* If the operator is applicable and the effect condition is
       satisfied, then the effect is triggered. */
    if (num_unsatisfied_preconditions[op_id].first == 0 &&
        num_unsatisfied_preconditions[op_id].second[noop_id] == 0) {
        /*
          The trigger for `op_id` being empty indicates that all noops are
          triggered anyway. Testing `contains` first is necessary to not
          generate the (empty) entry for `op_id` when using the [] operator.
        */
        if (!trigger.contains(op_id) || !trigger[op_id].empty()) {
            trigger[op_id].insert(noop_id);
        }
    }
}

// Triggers which operators are reevaluated at the next level.
void LandmarkFactoryHM::propagate_pm_propositions(
    HMEntry &hm_entry, bool newly_discovered, TriggerSet &trigger) {
    // For each operator/noop for which the proposition is a precondition.
    for (auto [op_id, noop_id] : hm_entry.triggered_operators) {
        if (noop_id == -1) {
            // The proposition is a precondition for the operator itself.
            trigger_operator(op_id, newly_discovered, trigger);
        } else {
            trigger_conditional_noop(op_id, noop_id, newly_discovered, trigger);
        }
    }
}

LandmarkFactoryHM::TriggerSet LandmarkFactoryHM::mark_state_propositions_reached(
    const State &state, const VariablesProxy &variables) {
    vector<Propositions> state_propositions = get_m_sets(variables, state);
    TriggerSet triggers;

    for (const auto &proposition : state_propositions) {
        HMEntry &hm_entry = hm_table[set_indices[proposition]];
        hm_entry.reached = true;
        propagate_pm_propositions(hm_entry, true, triggers);
    }

    /* This is necessary to trigger operators without preconditions which are
       not dealt with in the `propagate_pm_propositions` above. */
    for (int i = 0; i < static_cast<int>(pm_operators.size()); ++i) {
        if (num_unsatisfied_preconditions[i].first == 0) {
            /*
              Clear trigger for `op_id` (or create entry if it does not yet
              exist) to indicate that the precondition of the corresponding
              operator is satisfied and all conditional noops are triggered.
            */
            triggers[i].clear();
        }
    }
    return triggers;
}

void LandmarkFactoryHM::collect_condition_landmarks(
    const vector<int> &condition, vector<int> &landmarks) const {
    for (int proposition : condition) {
        const vector<int> &other_landmarks = hm_table[proposition].landmarks;
        landmarks.insert(landmarks.end(), other_landmarks.begin(),
                         other_landmarks.end());
    }
    // Each proposition is a landmark for itself but not stored for itself.
    landmarks.insert(landmarks.end(), condition.begin(), condition.end());
    utils::sort_unique(landmarks);
}

void LandmarkFactoryHM::initialize_proposition_landmark(
    int op_id, HMEntry &hm_entry, const vector<int> &landmarks,
    const vector<int> &precondition_landmarks, TriggerSet &triggers) {
    hm_entry.reached = true;
    hm_entry.landmarks = landmarks;
    if (use_orders) {
        hm_entry.precondition_landmarks = precondition_landmarks;
    }
    hm_entry.first_achievers.insert(op_id);
    propagate_pm_propositions(hm_entry, true, triggers);
}

void LandmarkFactoryHM::update_proposition_landmark(
    int op_id, int proposition, const vector<int> &landmarks,
    const vector<int> &precondition_landmarks, TriggerSet &triggers) {
    HMEntry &hm_entry = hm_table[proposition];
    size_t prev_size = hm_entry.landmarks.size();
    set_intersection(hm_entry.landmarks, landmarks);

    /*
      If the effect appears in `landmarks`, the proposition is not
      achieved for the first time. No need to intersect for
      greedy-necessary orderings or add `op` to the first achievers.
    */
    if (ranges::find(landmarks, proposition) == landmarks.end()) {
        hm_entry.first_achievers.insert(op_id);
        if (use_orders) {
            set_intersection(hm_entry.precondition_landmarks,
                             precondition_landmarks);
        }
    }

    if (hm_entry.landmarks.size() != prev_size) {
        propagate_pm_propositions(hm_entry, false, triggers);
    }
}

void LandmarkFactoryHM::update_effect_landmarks(
    int op_id, const vector<int> &effect, const vector<int> &landmarks,
    const vector<int> &precondition_landmarks, TriggerSet &triggers) {
    for (int proposition : effect) {
        HMEntry &hm_entry = hm_table[proposition];
        if (hm_entry.reached) {
            update_proposition_landmark(op_id, proposition, landmarks,
                                        precondition_landmarks, triggers);
        } else {
            initialize_proposition_landmark(
                op_id, hm_entry, landmarks, precondition_landmarks,
                triggers);
        }
    }
}

void LandmarkFactoryHM::update_noop_landmarks(
    const unordered_set<int> &current_triggers, const PiMOperator &op,
    const vector<int> &landmarks,
    const vector<int> &prerequisites, TriggerSet &next_triggers) {
    if (current_triggers.empty()) {
        /*
          The landmarks for the operator have changed, so we have to recompute
          the landmarks for all conditional noops if all their effect conditions
          are satisfied.
        */
        int num_noops = static_cast<int>(op.conditional_noops.size());
        for (int i = 0; i < num_noops; ++i) {
            if (num_unsatisfied_preconditions[op.id].second[i] == 0) {
                compute_noop_landmarks(
                    op.id, i, landmarks, prerequisites, next_triggers);
            }
        }
    } else {
        // Only recompute landmarks for conditions whose landmarks have changed.
        for (int noop_it : current_triggers) {
            assert(num_unsatisfied_preconditions[op.id].second[noop_it] == 0);
            compute_noop_landmarks(
                op.id, noop_it, landmarks, prerequisites, next_triggers);
        }
    }
}

void LandmarkFactoryHM::compute_hm_landmarks(const TaskProxy &task_proxy) {
    TriggerSet current_trigger = mark_state_propositions_reached(
        task_proxy.get_initial_state(), task_proxy.get_variables());
    for (int level = 1; !current_trigger.empty(); ++level) {
        TriggerSet next_trigger;
        for (const auto &[op_id, triggers] : current_trigger) {
            PiMOperator &op = pm_operators[op_id];
            vector<int> landmarks, precondition_landmarks;
            const vector<int> &precondition = op.precondition;
            collect_condition_landmarks(precondition, landmarks);
            if (use_orders) {
                precondition_landmarks.insert(
                    precondition_landmarks.end(), precondition.begin(),
                    precondition.end());
                utils::sort_unique(precondition_landmarks);
            }
            update_effect_landmarks(op_id, op.effect, landmarks,
                                    precondition_landmarks, next_trigger);
            update_noop_landmarks(
                triggers, op, landmarks, precondition_landmarks, next_trigger);
        }
        current_trigger.swap(next_trigger);

        if (log.is_at_least_verbose()) {
            log << "Level " << level << " completed." << endl;
        }
    }
    if (log.is_at_least_normal()) {
        log << "h^m landmarks computed." << endl;
    }
}

void LandmarkFactoryHM::compute_noop_landmarks(
    int op_id, int noop_index, const vector<int> &landmarks,
    const vector<int> &necessary, TriggerSet &next_trigger) {
    const auto &[effect_condition, effect] =
        pm_operators[op_id].conditional_noops[noop_index];

    vector<int> conditional_noop_landmarks(landmarks);
    collect_condition_landmarks(effect_condition, conditional_noop_landmarks);
    vector<int> conditional_noop_necessary;
    if (use_orders) {
        conditional_noop_necessary = necessary;
        conditional_noop_necessary.insert(
            conditional_noop_necessary.end(), effect_condition.begin(),
            effect_condition.end());
        utils::sort_unique(conditional_noop_necessary);
    }

    update_effect_landmarks(
        op_id, effect, conditional_noop_landmarks,
        conditional_noop_necessary, next_trigger);
}

void LandmarkFactoryHM::add_landmark_node(int set_index, bool goal) {
    if (!landmark_nodes.contains(set_index)) {
        const HMEntry &hm_entry = hm_table[set_index];
        vector<FactPair> facts(hm_entry.propositions);
        utils::sort_unique(facts);
        assert(!facts.empty());
        LandmarkType type = facts.size() == 1 ? ATOMIC : CONJUNCTIVE;
        Landmark landmark(move(facts), type, goal);
        landmark.first_achievers.insert(hm_entry.first_achievers.begin(),
                                        hm_entry.first_achievers.end());
        landmark_nodes[set_index] =
            &landmark_graph->add_landmark(move(landmark));
    }
}

unordered_set<int> LandmarkFactoryHM::collect_and_add_landmarks_to_landmark_graph(
    const VariablesProxy &variables, const Propositions &goals) {
    unordered_set<int> landmarks;
    for (const Propositions &goal_subset : get_m_sets(variables, goals)) {
        assert(set_indices.contains(goal_subset));
        int proposition_id = set_indices[goal_subset];

        if (!hm_table[proposition_id].reached) {
            if (log.is_at_least_verbose()) {
                log << "\n\nSubset of goal not reachable !!.\n\n\n";
                log << "Subset is: ";
                print_proposition_set(
                    variables, hm_table[proposition_id].propositions);
                log << endl;
            }
        }

        const vector<int> &proposition_landmarks =
            hm_table[proposition_id].landmarks;
        landmarks.insert(
            proposition_landmarks.begin(), proposition_landmarks.end());
        // The goal itself is also a landmark.
        landmarks.insert(proposition_id);
        add_landmark_node(proposition_id, true);
    }
    for (int landmark : landmarks) {
        add_landmark_node(landmark, false);
    }
    return landmarks;
}

void LandmarkFactoryHM::reduce_landmarks(const unordered_set<int> &landmarks) {
    assert(use_orders);
    /*
      TODO: This function depends on the order in which landmarks are processed.
       I don't think there's a particular reason to sort the landmarks apart
       from it was like this before the refactoring in issue992 and we wanted
       the changes to induce no semantic changes. It's probably best to replace
       this with a deterministic function that does not depend on the order.
    */
    vector<int> sorted_landmarks(landmarks.begin(), landmarks.end());
    sort(sorted_landmarks.begin(), sorted_landmarks.end());
    for (int landmark : sorted_landmarks) {
        HMEntry &hm_entry = hm_table[landmark];
        /* We cannot remove directly from `hm_entry.landmarks` because doing
           so invalidates the loop variable. */
        vector<int> landmarks_to_remove(hm_entry.precondition_landmarks);
        for (int predecessor : hm_entry.landmarks) {
            const vector<int> &predecessor_landmarks =
                hm_table[predecessor].landmarks;
            landmarks_to_remove.insert(
                landmarks_to_remove.end(), predecessor_landmarks.begin(),
                predecessor_landmarks.end());
        }
        utils::sort_unique(landmarks_to_remove);
        set_difference(hm_entry.landmarks, landmarks_to_remove);
    }
}

void LandmarkFactoryHM::add_landmark_orderings(
    const unordered_set<int> &landmarks) {
    assert(use_orders);
    for (int to : landmarks) {
        assert(landmark_nodes.contains(to));
        for (int from : hm_table[to].precondition_landmarks) {
            assert(landmark_nodes.contains(from));
            add_or_replace_ordering_if_stronger(
                *landmark_nodes[from], *landmark_nodes[to],
                OrderingType::GREEDY_NECESSARY);
        }
        for (int from : hm_table[to].landmarks) {
            assert(landmark_nodes.contains(from));
            add_or_replace_ordering_if_stronger(
                *landmark_nodes[from], *landmark_nodes[to],
                OrderingType::NATURAL);
        }
    }
}

void LandmarkFactoryHM::construct_landmark_graph(
    const TaskProxy &task_proxy) {
    Propositions goals =
        task_properties::get_fact_pairs(task_proxy.get_goals());
    VariablesProxy variables = task_proxy.get_variables();
    unordered_set<int> landmarks =
        collect_and_add_landmarks_to_landmark_graph(variables, goals);
    if (use_orders) {
        reduce_landmarks(landmarks);
        add_landmark_orderings(landmarks);
    }
}

void LandmarkFactoryHM::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    initialize(task_proxy);
    compute_hm_landmarks(task_proxy);
    construct_landmark_graph(task_proxy);
    free_unneeded_memory();
    postprocess(task_proxy);
}

bool LandmarkFactoryHM::supports_conditional_effects() const {
    return false;
}

class LandmarkFactoryHMFeature
    : public plugins::TypedFeature<LandmarkFactory, LandmarkFactoryHM> {
public:
    LandmarkFactoryHMFeature() : TypedFeature("lm_hm") {
        // document_group("");
        document_title("h^m Landmarks");
        document_synopsis(
            "The landmark generation method introduced in the "
            "following paper" +
            utils::format_conference_reference(
                {"Emil Keyder", "Silvia Richter", "Malte Helmert"},
                "Sound and Complete Landmarks for And/Or Graphs",
                "https://ai.dmi.unibas.ch/papers/keyder-et-al-ecai2010.pdf",
                "Proceedings of the 19th European Conference on Artificial "
                "Intelligence (ECAI 2010)",
                "335-340",
                "IOS Press",
                "2010"));

        add_option<int>(
            "m", "subset size (if unsure, use the default of 2)", "2");
        add_option<bool>(
            "conjunctive_landmarks",
            "keep conjunctive landmarks",
            "true");
        add_use_orders_option_to_feature(*this);
        add_landmark_factory_options_to_feature(*this);

        document_language_support(
            "conditional_effects",
            "ignored, i.e. not supported");
    }

    virtual shared_ptr<LandmarkFactoryHM> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryHM>(
            opts.get<int>("m"),
            opts.get<bool>("conjunctive_landmarks"),
            get_use_orders_arguments_from_options(opts),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryHMFeature> _plugin;
}
