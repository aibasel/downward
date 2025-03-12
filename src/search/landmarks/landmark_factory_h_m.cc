#include "landmark_factory_h_m.h"

#include "exploration.h"
#include "landmark.h"

#include "../abstract_task.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#include <ranges>
#include <set>

using namespace std;
using utils::ExitCode;

namespace landmarks {
// TODO: Can we turn these set operations into static functions?
// alist = alist \cup other
template<typename T>
void union_with(list<T> &alist, const list<T> &other) {
    auto it1 = alist.begin();
    auto it2 = other.begin();

    while (it1 != alist.end() && it2 != other.end()) {
        if (*it1 < *it2) {
            ++it1;
        } else if (*it1 > *it2) {
            alist.insert(it1, *it2);
            ++it2;
        } else {
            ++it1;
            ++it2;
        }
    }
    alist.insert(it1, it2, other.end());
}

// alist = alist \cap other
template<typename T>
void intersect_with(list<T> &alist, const list<T> &other) {
    auto it1 = alist.begin();
    auto it2 = other.begin();

    while (it1 != alist.end() && it2 != other.end()) {
        if (*it1 < *it2) {
            auto tmp = it1;
            ++tmp;
            alist.erase(it1);
            it1 = tmp;
        } else if (*it1 > *it2) {
            ++it2;
        } else {
            ++it1;
            ++it2;
        }
    }
    alist.erase(it1, alist.end());
}

// alist = alist \setminus other
template<typename T>
void set_minus(list<T> &alist, const list<T> &other) {
    auto it1 = alist.begin();
    auto it2 = other.begin();

    while (it1 != alist.end() && it2 != other.end()) {
        if (*it1 < *it2) {
            ++it1;
        } else if (*it1 > *it2) {
            ++it2;
        } else {
            auto tmp = it1;
            ++tmp;
            alist.erase(it1);
            it1 = tmp;
            ++it2;
        }
    }
}

// alist = alist \cup {val}
template<typename T>
void insert_into(list<T> &alist, const T &val) {
    auto it1 = alist.begin();

    while (it1 != alist.end()) {
        if (*it1 > val) {
            alist.insert(it1, val);
            return;
        } else if (*it1 < val) {
            ++it1;
        } else {
            return;
        }
    }
    alist.insert(it1, val);
}

template<typename T>
static bool contains(list<T> &alist, const T &val) {
    return find(alist.begin(), alist.end(), val) != alist.end();
}

static bool are_mutex(const VariablesProxy &variables,
                      const FactPair &atom1, const FactPair &atom2) {
    return variables[atom1.var].get_fact(atom1.value).is_mutex(
        variables[atom2.var].get_fact(atom2.value));
}

void LandmarkFactoryHM::get_m_sets_including_current_var(
    const VariablesProxy &variables, int num_included, int current_var,
    Propositions &current, std::vector<Propositions> &subsets) {
    int domain_size = variables[current_var].get_domain_size();
    for (int value = 0; value < domain_size; ++value) {
        FactPair atom(current_var, value);
        bool use_var = ranges::none_of(
            current.begin(), current.end(), [&](const FactPair &other) {
                return are_mutex(variables, atom, other);
            });
        if (use_var) {
            current.push_back(atom);
            get_m_sets(variables, num_included + 1, current_var + 1, current, subsets);
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
    std::vector<Propositions> &subsets, const Propositions &superset) {
    const FactPair &atom = superset[current_index];
    bool use_proposition = ranges::none_of(
        current.begin(), current.end(), [&](const FactPair &other) {
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
    Propositions &current, vector<Propositions> &subsets, const Propositions &superset) {
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
    get_m_sets_of_set(variables, num_included, current_index + 1, current, subsets, superset);
}

void LandmarkFactoryHM::get_split_m_sets_including_current_proposition_from_first(
    const VariablesProxy &variables, int num_included1, int num_included2,
    int current_index1, int current_index2,
    Propositions &current, std::vector<Propositions> &subsets,
    const Propositions &superset1, const Propositions &superset2) {
    const FactPair &atom = superset1[current_index1];
    bool use_proposition = ranges::none_of(
        current.begin(), current.end(), [&](const FactPair &other) {
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

/*
  Get subsets of size <= m such that at least one element from `superset1` and
  at least one element from `superset2` are included, except if a sets is empty.
  We assume `superset1` and `superset2` are disjoint.
  TODO: Assert that supersets are disjoint. Should the variables occurring in
    the sets be disjoint, rather than their propositions?
*/
vector<Propositions> LandmarkFactoryHM::get_split_m_sets(
    const VariablesProxy &variables,
    const Propositions &superset1, const Propositions &superset2) {
    Propositions c;
    vector<Propositions> subsets;
    // If a set is empty, we do not have to include from it. TODO: Why not?
    if (superset1.empty()) {
        get_m_sets_of_set(variables, 0, 0, c, subsets, superset2);
    } else if (superset2.empty()) {
        get_m_sets_of_set(variables, 0, 0, c, subsets, superset1);
    } else {
        get_split_m_sets(variables, 0, 0, 0, 0, c, subsets, superset1, superset2);
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

static set<FactPair> get_as_set(
    const vector<int> &collection, const vector<HMEntry> &hm_table) {
    set<FactPair> preconditions;
    for (int element : collection) {
        for (const FactPair &proposition : hm_table[element].propositions) {
            preconditions.insert(proposition);
        }
    }
    return preconditions;
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

static pair<vector<int>, vector<int>> split_conditional_noop(
    const vector<int> &conditional_noop) {
    vector<int> effect_condition;
    effect_condition.reserve(conditional_noop.size());
    size_t i;
    for (i = 0; conditional_noop[i] != -1; ++i) {
        effect_condition.push_back(conditional_noop[i]);
    }

    ++i; // Skip delimiter -1.

    vector<int> effect;
    effect.reserve(conditional_noop.size());
    for (; i < conditional_noop.size(); ++i) {
        effect.push_back(conditional_noop[i]);
    }
    return {effect_condition, effect};
}

void LandmarkFactoryHM::print_conditional_noop(
    const VariablesProxy &variables, const vector<int> &conditional_noop,
    vector<pair<set<FactPair>, set<FactPair>>> &conditions) const {
    auto [effect_condition, effect] = split_conditional_noop(conditional_noop);
    set<FactPair> effect_condition_set =
        print_effect_condition(variables, effect_condition);
    set<FactPair> effect_set = print_conditional_effect(variables, effect);
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
    const std::vector<std::pair<std::set<FactPair>, std::set<FactPair>>> &conditions) const {
    log << "Action " << op.index << endl;
    log << "Precondition: ";
    set<FactPair> preconditions = get_as_set(op.precondition, hm_table);
    for (const FactPair &pc : preconditions) {
        print_proposition(variables, pc);
        log << " ";
    }

    log << endl << "Effect: ";
    set<FactPair> effects = get_as_set(op.effect, hm_table);
    for (const FactPair &eff : effects) {
        print_proposition(variables, eff);
        log << " ";
    }
    log << endl << "Conditionals: " << endl;
    int i = 0;
    for (const auto &condition : conditions) {
        log << "Effect Condition #" << i++ << ":\n\t";
        for (const FactPair &cond : condition.first) {
            print_proposition(variables, cond);
            log << " ";
        }
        log << endl << "Conditional Effect #" << i << ":\n\t";
        for (const FactPair &eff : condition.second) {
            print_proposition(variables, eff);
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
        // TODO: Do not abuse FactPair here!!!
        hm_table[set_index].pc_for.emplace_back(op.get_id(), -1);
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
    vector<Propositions> noop_precondition_subsets =
        get_split_m_sets(variables, preconditions, propositions);
    vector<Propositions> noop_postconditions_subsets =
        get_split_m_sets(variables, postconditions, propositions);

    vector<int> conditional_noop;
    conditional_noop.reserve(noop_precondition_subsets.size() +
                             noop_postconditions_subsets.size() + 1);
    num_unsatisfied_preconditions[op_id].second.push_back(
        static_cast<int>(noop_precondition_subsets.size()));

    // Add the conditional noop preconditions.
    for (const auto & subset : noop_precondition_subsets) {
        assert(static_cast<int>(subset.size()) <= m);
        assert(set_indices.contains(subset));
        int set_index = set_indices[subset];
        conditional_noop.push_back(set_index);
        // These propositions are "conditional preconditions" for this operator.
        hm_table[set_index].pc_for.emplace_back(op_id, noop_index);
    }

    // Separate conditional preconditions from conditional effects by number -1.
    conditional_noop.push_back(-1);

    // Add the conditional noop effects.
    for (const auto & subset : noop_postconditions_subsets) {
        assert(static_cast<int>(subset.size()) <= m);
        assert(set_indices.contains(subset));
        int set_index = set_indices[subset];
        conditional_noop.push_back(set_index);
    }

    pm_op.conditional_noops.push_back(move(conditional_noop));
}

void LandmarkFactoryHM::initialize_noops(
    const VariablesProxy &variables, PiMOperator &pm_op, int op_id,
    const Propositions &preconditions, const Propositions &postconditions) {
    pm_op.conditional_noops.reserve(set_indices.size());
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
        pm_op.index = i;

        Propositions preconditions =
            initialize_preconditions(variables, op, pm_op);
        Propositions postconditions =
            initialize_postconditions(variables, op, pm_op);
        initialize_noops(
            variables, pm_op, op.get_id(), preconditions, postconditions);
        print_pm_operator(variables, pm_op);
    }
}

// TODO: Replace usages of `interesting` with `are_mutex` above.
bool LandmarkFactoryHM::interesting(const VariablesProxy &variables,
                                    const FactPair &fact1, const FactPair &fact2) const {
    // mutexes can always be safely pruned
    return !variables[fact1.var].get_fact(fact1.value).is_mutex(
        variables[fact2.var].get_fact(fact2.value));
}

LandmarkFactoryHM::LandmarkFactoryHM(
    int m, bool conjunctive_landmarks, bool use_orders,
    utils::Verbosity verbosity)
    : LandmarkFactory(verbosity),
      m(m),
      conjunctive_landmarks(conjunctive_landmarks),
      use_orders(use_orders) {
}

void LandmarkFactoryHM::initialize(const TaskProxy &task_proxy) {
    if (log.is_at_least_normal()) {
        log << "h^m landmarks m=" << m << endl;
    }
    if (!task_proxy.get_axioms().empty()) {
        cerr << "h^m landmarks don't support axioms" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
    // Get all the m or less size subsets in the domain.
    vector<vector<FactPair>> msets =
        get_m_sets(task_proxy.get_variables());

    // map each set to an integer
    for (size_t i = 0; i < msets.size(); ++i) {
        hm_table.emplace_back();
        set_indices[msets[i]] = i;
        hm_table[i].propositions = msets[i];
    }
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
    if (landmark_graph->get_num_conjunctive_landmarks() > 0) {
        if (log.is_at_least_normal()) {
            log << "Discarding " << landmark_graph->get_num_conjunctive_landmarks()
                << " conjunctive landmarks" << endl;
        }
        landmark_graph->remove_node_if(
            [](const LandmarkNode &node) {return node.get_landmark().is_conjunctive;});
    }
}

void LandmarkFactoryHM::calc_achievers(const TaskProxy &task_proxy) {
    assert(!achievers_calculated);
    if (log.is_at_least_normal()) {
        log << "Calculating achievers." << endl;
    }

    OperatorsProxy operators = task_proxy.get_operators();
    VariablesProxy variables = task_proxy.get_variables();
    // first_achievers are already filled in by compute_h_m_landmarks
    // here only have to do possible_achievers
    for (const auto &node : *landmark_graph) {
        Landmark &landmark = node->get_landmark();
        set<int> candidates;
        // put all possible adders in candidates set
        for (const FactPair &atom : landmark.atoms) {
            const vector<int> &ops = get_operators_including_effect(atom);
            candidates.insert(ops.begin(), ops.end());
        }

        for (int op_id : candidates) {
            Propositions post = get_operator_postcondition(variables.size(), operators[op_id]);
            Propositions pre = get_operator_precondition(operators[op_id]);
            size_t j;
            for (j = 0; j < landmark.atoms.size(); ++j) {
                const FactPair &atom = landmark.atoms[j];
                // action adds this element of landmark as well
                if (find(post.begin(), post.end(), atom) != post.end())
                    continue;
                bool is_mutex = false;
                for (const FactPair &proposition : post) {
                    if (variables[proposition.var].get_fact(proposition.value).is_mutex(
                            variables[atom.var].get_fact(atom.value))) {
                        is_mutex = true;
                        break;
                    }
                }
                if (is_mutex) {
                    break;
                }
                for (const FactPair &proposition : pre) {
                    // we know that lm_val is not added by the operator
                    // so if it incompatible with the pc, this can't be an achiever
                    if (variables[proposition.var].get_fact(proposition.value).is_mutex(
                            variables[atom.var].get_fact(atom.value))) {
                        is_mutex = true;
                        break;
                    }
                }
                if (is_mutex) {
                    break;
                }
            }
            if (j == landmark.atoms.size()) {
                // not inconsistent with any of the other landmark propositions
                landmark.possible_achievers.insert(op_id);
            }
        }
    }
    achievers_calculated = true;
}

void LandmarkFactoryHM::free_unneeded_memory() {
    utils::release_vector_memory(hm_table);
    utils::release_vector_memory(pm_operators);
    utils::release_vector_memory(num_unsatisfied_preconditions);

    set_indices.clear();
    landmark_node_table.clear();
}

// called when a fact is discovered or its landmarks change
// to trigger required actions at next level
// newly_discovered = first time fact becomes reachable
void LandmarkFactoryHM::propagate_pm_atoms(int atom_index, bool newly_discovered,
                                          TriggerSet &trigger) {
    // for each action/noop for which fact is a pc
    for (const FactPair &info : hm_table[atom_index].pc_for) {
        // a pc for the action itself
        if (info.value == -1) {
            if (newly_discovered) {
                --num_unsatisfied_preconditions[info.var].first;
            }
            // add to queue if unsatcount at 0
            if (num_unsatisfied_preconditions[info.var].first == 0) {
                // create empty set or clear prev entries -- signals do all possible noop effects
                trigger[info.var].clear();
            }
        }
        // a pc for a conditional noop
        else {
            if (newly_discovered) {
                --num_unsatisfied_preconditions[info.var].second[info.value];
            }
            // if associated action is applicable, and effect has become applicable
            // (if associated action is not applicable, all noops will be used when it first does)
            if ((num_unsatisfied_preconditions[info.var].first == 0) &&
                (num_unsatisfied_preconditions[info.var].second[info.value] == 0)) {
                // if not already triggering all noops, add this one
                if ((trigger.find(info.var) == trigger.end()) ||
                    (!trigger[info.var].empty())) {
                    trigger[info.var].insert(info.value);
                }
            }
        }
    }
}

void LandmarkFactoryHM::compute_hm_landmarks(const TaskProxy &task_proxy) {
    // get subsets of initial state
    vector<Propositions> init_subsets =
        get_m_sets(task_proxy.get_variables(), task_proxy.get_initial_state());

    TriggerSet current_trigger, next_trigger;

    // for all of the initial state <= m subsets, mark level = 0
    for (size_t i = 0; i < init_subsets.size(); ++i) {
        int index = set_indices[init_subsets[i]];
        hm_table[index].level = 0;

        // set actions to be applied
        propagate_pm_atoms(index, true, current_trigger);
    }

    // mark actions with no precondition to be applied
    for (size_t i = 0; i < pm_operators.size(); ++i) {
        if (num_unsatisfied_preconditions[i].first == 0) {
            // create empty set or clear prev entries
            current_trigger[i].clear();
        }
    }

    vector<int>::iterator it;
    TriggerSet::iterator op_it;

    list<int> local_landmarks;
    list<int> local_necessary;

    size_t prev_size;

    int level = 1;

    // while we have actions to apply
    while (!current_trigger.empty()) {
        for (op_it = current_trigger.begin(); op_it != current_trigger.end(); ++op_it) {
            local_landmarks.clear();
            local_necessary.clear();

            int op_index = op_it->first;
            PiMOperator &action = pm_operators[op_index];

            // gather landmarks for pcs
            // in the set of landmarks for each fact, the fact itself is not stored
            // (only landmarks preceding it)
            for (it = action.precondition.begin(); it != action.precondition.end(); ++it) {
                union_with(local_landmarks, hm_table[*it].landmarks);
                insert_into(local_landmarks, *it);

                if (use_orders) {
                    insert_into(local_necessary, *it);
                }
            }

            for (it = action.effect.begin(); it != action.effect.end(); ++it) {
                if (hm_table[*it].level != -1) {
                    prev_size = hm_table[*it].landmarks.size();
                    intersect_with(hm_table[*it].landmarks, local_landmarks);

                    // if the add effect appears in local landmarks,
                    // fact is being achieved for >1st time
                    // no need to intersect for gn orderings
                    // or add op to first achievers
                    if (!contains(local_landmarks, *it)) {
                        insert_into(hm_table[*it].first_achievers, op_index);
                        if (use_orders) {
                            intersect_with(hm_table[*it].necessary, local_necessary);
                        }
                    }

                    if (hm_table[*it].landmarks.size() != prev_size)
                        propagate_pm_atoms(*it, false, next_trigger);
                } else {
                    hm_table[*it].level = level;
                    hm_table[*it].landmarks = local_landmarks;
                    if (use_orders) {
                        hm_table[*it].necessary = local_necessary;
                    }
                    insert_into(hm_table[*it].first_achievers, op_index);
                    propagate_pm_atoms(*it, true, next_trigger);
                }
            }

            // landmarks changed for action itself, have to recompute
            // landmarks for all noop effects
            if (op_it->second.empty()) {
                for (size_t i = 0; i < action.conditional_noops.size(); ++i) {
                    // actions pcs are satisfied, but cond. effects may still have
                    // unsatisfied pcs
                    if (num_unsatisfied_preconditions[op_index].second[i] == 0) {
                        compute_noop_landmarks(op_index, i,
                                               local_landmarks,
                                               local_necessary,
                                               level, next_trigger);
                    }
                }
            }
            // only recompute landmarks for conditions whose
            // landmarks have changed
            else {
                for (set<int>::iterator noop_it = op_it->second.begin();
                     noop_it != op_it->second.end(); ++noop_it) {
                    assert(num_unsatisfied_preconditions[op_index].second[*noop_it] == 0);

                    compute_noop_landmarks(op_index, *noop_it,
                                           local_landmarks,
                                           local_necessary,
                                           level, next_trigger);
                }
            }
        }
        current_trigger.swap(next_trigger);
        next_trigger.clear();

        if (log.is_at_least_verbose()) {
            log << "Level " << level << " completed." << endl;
        }
        ++level;
    }
    if (log.is_at_least_normal()) {
        log << "h^m landmarks computed." << endl;
    }
}

void LandmarkFactoryHM::compute_noop_landmarks(
    int op_index, int noop_index,
    list<int> const &local_landmarks,
    list<int> const &local_necessary,
    int level,
    TriggerSet &next_trigger) {
    list<int> cn_necessary, cn_landmarks;
    size_t prev_size;
    int pm_proposition;

    PiMOperator &action = pm_operators[op_index];
    vector<int> &pc_eff_pair = action.conditional_noops[noop_index];

    cn_landmarks.clear();

    cn_landmarks = local_landmarks;

    if (use_orders) {
        cn_necessary.clear();
        cn_necessary = local_necessary;
    }

    size_t i;
    for (i = 0; (pm_proposition = pc_eff_pair[i]) != -1; ++i) {
        union_with(cn_landmarks, hm_table[pm_proposition].landmarks);
        insert_into(cn_landmarks, pm_proposition);

        if (use_orders) {
            insert_into(cn_necessary, pm_proposition);
        }
    }

    // go to the beginning of the effects section
    ++i;

    for (; i < pc_eff_pair.size(); ++i) {
        pm_proposition = pc_eff_pair[i];
        if (hm_table[pm_proposition].level != -1) {
            prev_size = hm_table[pm_proposition].landmarks.size();
            intersect_with(hm_table[pm_proposition].landmarks, cn_landmarks);

            // if the add effect appears in cn_landmarks,
            // fact is being achieved for >1st time
            // no need to intersect for gn orderings
            // or add op to first achievers
            if (!contains(cn_landmarks, pm_proposition)) {
                insert_into(hm_table[pm_proposition].first_achievers, op_index);
                if (use_orders) {
                    intersect_with(hm_table[pm_proposition].necessary, cn_necessary);
                }
            }

            if (hm_table[pm_proposition].landmarks.size() != prev_size)
                propagate_pm_atoms(pm_proposition, false, next_trigger);
        } else {
            hm_table[pm_proposition].level = level;
            hm_table[pm_proposition].landmarks = cn_landmarks;
            if (use_orders) {
                hm_table[pm_proposition].necessary = cn_necessary;
            }
            insert_into(hm_table[pm_proposition].first_achievers, op_index);
            propagate_pm_atoms(pm_proposition, true, next_trigger);
        }
    }
}

void LandmarkFactoryHM::add_landmark_node(int set_index, bool goal) {
    if (landmark_node_table.find(set_index) == landmark_node_table.end()) {
        const HMEntry &hm_entry = hm_table[set_index];
        vector<FactPair> facts(hm_entry.propositions);
        utils::sort_unique(facts);
        assert(!facts.empty());
        Landmark landmark(facts, false, (facts.size() > 1), goal);
        landmark.first_achievers.insert(
            hm_entry.first_achievers.begin(),
            hm_entry.first_achievers.end());
        landmark_node_table[set_index] = &landmark_graph->add_landmark(move(landmark));
    }
}

void LandmarkFactoryHM::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    initialize(task_proxy);
    compute_hm_landmarks(task_proxy);
    // now construct landmarks graph
    Propositions goals = task_properties::get_fact_pairs(task_proxy.get_goals());
    VariablesProxy variables = task_proxy.get_variables();
    vector<Propositions> goal_subsets = get_m_sets(variables, goals);
    list<int> all_landmarks;
    for (const Propositions &goal_subset : goal_subsets) {
        assert(set_indices.find(goal_subset) != set_indices.end());

        int set_index = set_indices[goal_subset];

        if (hm_table[set_index].level == -1) {
            if (log.is_at_least_verbose()) {
                log << endl << endl << "Subset of goal not reachable !!." << endl << endl << endl;
                log << "Subset is: ";
                print_proposition_set(variables, hm_table[set_index].propositions);
                log << endl;
            }
        }

        // set up goals landmarks for processing
        union_with(all_landmarks, hm_table[set_index].landmarks);

        // the goal itself is also a landmark
        insert_into(all_landmarks, set_index);

        // make a node for the goal, with in_goal = true;
        add_landmark_node(set_index, true);
    }
    // now make remaining landmark nodes
    for (int landmark : all_landmarks) {
        add_landmark_node(landmark, false);
    }
    if (use_orders) {
        // do reduction of graph
        // if f2 is landmark for f1, subtract landmark set of f2 from that of f1
        for (int f1 : all_landmarks) {
            list<int> everything_to_remove;
            for (int f2 : hm_table[f1].landmarks) {
                union_with(everything_to_remove, hm_table[f2].landmarks);
            }
            set_minus(hm_table[f1].landmarks, everything_to_remove);
            // remove necessaries here, otherwise they will be overwritten
            // since we are writing them as greedy nec. orderings.
            if (use_orders)
                set_minus(hm_table[f1].landmarks, hm_table[f1].necessary);
        }

        // add the orderings.

        for (int set_index : all_landmarks) {
            for (int landmark : hm_table[set_index].landmarks) {
                assert(landmark_node_table.find(landmark) != landmark_node_table.end());
                assert(landmark_node_table.find(set_index) != landmark_node_table.end());

                add_ordering_or_replace_if_stronger(
                    *landmark_node_table[landmark],
                    *landmark_node_table[set_index], OrderingType::NATURAL);
            }
            for (int gn : hm_table[set_index].necessary) {
                add_ordering_or_replace_if_stronger(
                    *landmark_node_table[gn], *landmark_node_table[set_index],
                    OrderingType::GREEDY_NECESSARY);
            }
        }
    }
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
            "The landmark generation method introduced by "
            "Keyder, Richter & Helmert (ECAI 2010).");

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
