#include "landmark_factory_rpg_sasp.h"

#include "exploration.h"
#include "landmark.h"
#include "landmark_factory.h"
#include "landmark_graph.h"
#include "util.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#include <cassert>
#include <ranges>
#include <unordered_map>

using namespace std;
using utils::ExitCode;

namespace landmarks {
LandmarkFactoryRpgSasp::LandmarkFactoryRpgSasp(
    bool disjunctive_landmarks, bool use_orders, utils::Verbosity verbosity)
    : LandmarkFactoryRelaxation(verbosity),
      disjunctive_landmarks(disjunctive_landmarks),
      use_orders(use_orders) {
}

void LandmarkFactoryRpgSasp::resize_dtg_data_structures(
    const TaskProxy &task_proxy) {
    VariablesProxy variables = task_proxy.get_variables();
    dtg_successors.resize(variables.size());
    for (VariableProxy var : variables) {
        dtg_successors[var.get_id()].resize(var.get_domain_size());
    }
}

void LandmarkFactoryRpgSasp::compute_dtg_successors(
    const EffectProxy &effect, const unordered_map<int, int> &preconditions,
    const unordered_map<int, int> &effect_conditions) {
    /* If the operator can change the value of `var` from `pre` to
       `post`, we insert `post` into `dtg_successors[var][pre]`. */
    auto [var, post] = effect.get_fact().get_pair();
    if (preconditions.contains(var)) {
        int pre = preconditions.at(var);
        if (effect_conditions.contains(var) &&
            effect_conditions.at(var) != pre) {
            // The precondition conflicts with the effect condition.
            return;
        }
        add_dtg_successor(var, pre, post);
    } else if (effect_conditions.contains(var)) {
        add_dtg_successor(var, effect_conditions.at(var), post);
    } else {
        int domain_size =
            effect.get_fact().get_variable().get_domain_size();
        for (int pre = 0; pre < domain_size; ++pre) {
            add_dtg_successor(var, pre, post);
        }
    }
}

static unordered_map<int, int> build_conditions_map(
    const ConditionsProxy &conditions) {
    unordered_map<int, int> condition_map;
    for (FactProxy condition : conditions) {
        condition_map[condition.get_variable().get_id()] =
            condition.get_value();
    }
    return condition_map;
}

void LandmarkFactoryRpgSasp::build_dtg_successors(const TaskProxy &task_proxy) {
    resize_dtg_data_structures(task_proxy);
    for (OperatorProxy op : task_proxy.get_operators()) {
        unordered_map<int, int> preconditions =
            build_conditions_map(op.get_preconditions());
        for (EffectProxy effect : op.get_effects()) {
            unordered_map<int, int> effect_conditions =
                build_conditions_map(effect.get_conditions());
            compute_dtg_successors(effect, preconditions, effect_conditions);
        }
    }
}

void LandmarkFactoryRpgSasp::add_dtg_successor(int var_id, int pre, int post) {
    if (pre != post) {
        dtg_successors[var_id][pre].insert(post);
    }
}

// Returns the set of variables occurring in the precondition.
static unordered_set<int> add_preconditions(
    const OperatorProxy &op, utils::HashSet<FactPair> &result) {
    unordered_set<int> precondition_variables;
    for (FactProxy precondition : op.get_preconditions()) {
        result.insert(precondition.get_pair());
        precondition_variables.insert(precondition.get_variable().get_id());
    }
    return precondition_variables;
}

/*
  For all binary variables, if there is an effect but no precondition on
  that variable and if the initial value differs from the value in the
  landmark, then the variable still has its initial value right before
  it is reached for the first time.
*/
static void add_binary_variable_conditions(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const EffectsProxy &effects,
    const unordered_set<int> &precondition_variables,
    utils::HashSet<FactPair> &result) {
    State initial_state = task_proxy.get_initial_state();
    for (EffectProxy effect : effects) {
        FactProxy effect_atom = effect.get_fact();
        int var_id = effect_atom.get_variable().get_id();
        if (!precondition_variables.contains(var_id) &&
            effect_atom.get_variable().get_domain_size() == 2) {
            for (const FactPair &atom : landmark.atoms) {
                if (atom.var == var_id &&
                    initial_state[var_id].get_value() != atom.value) {
                    assert(ranges::none_of(result,
                                           [&](const FactPair &result_atom) {
                                               return result_atom.var == var_id;
                                           }));
                    result.insert(initial_state[var_id].get_pair());
                    break;
                }
            }
        }
    }
}

static void add_effect_conditions(
    const Landmark &landmark, const EffectsProxy &effects,
    utils::HashSet<FactPair> &result) {
    // Intersect effect conditions of all effects that can achieve `landmark`.
    utils::HashSet<FactPair> intersection;
    bool init = true;
    for (const EffectProxy &effect : effects) {
        const FactPair &effect_atom = effect.get_fact().get_pair();
        if (!landmark.contains(effect_atom)) {
            continue;
        }
        if (effect.get_conditions().empty()) {
            /* Landmark is reached unconditionally, no effect conditions
               need to be added. */
            return;
        }

        utils::HashSet<FactPair> effect_condition;
        for (FactProxy atom : effect.get_conditions()) {
            effect_condition.insert(atom.get_pair());
        }
        if (init) {
            swap(intersection, effect_condition);
            init = false;
        } else {
            intersection = get_intersection(intersection, effect_condition);
        }

        if (intersection.empty()) {
            assert(!init);
            break;
        }
    }
    result.insert(intersection.begin(), intersection.end());
}

/*
  Collects conditions that must hold in all states in which `op` is
  applicable and potentially reaches `landmark` when applied. These are
  (1) preconditions of `op`,
  (2) inverse values of binary variables if the landmark does not hold
      initially, and
  (3) shared effect conditions of all conditional effects that achieve
      the landmark.
*/
static utils::HashSet<FactPair> approximate_preconditions_to_achieve_landmark(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const OperatorProxy &op) {
    utils::HashSet<FactPair> result;
    unordered_set<int> precondition_variables = add_preconditions(op, result);
    EffectsProxy effects = op.get_effects();
    add_binary_variable_conditions(
        task_proxy, landmark, effects, precondition_variables, result);
    add_effect_conditions(landmark, effects, result);
    return result;
}

/* Remove all pointers to `disjunctive_landmark_node` from internal data
   structures (i.e., the list of open landmarks and forward orders). */
void LandmarkFactoryRpgSasp::remove_occurrences_of_landmark_node(
    const LandmarkNode *node) {
    auto it = ranges::find(open_landmarks, node);
    if (it != open_landmarks.end()) {
        open_landmarks.erase(it);
    }
    forward_orderings.erase(node);
}

static vector<LandmarkNode *> get_natural_parents(const LandmarkNode *node) {
    // Retrieve incoming orderings from `disjunctive_landmark_node`.
    vector<LandmarkNode *> parents;
    parents.reserve(node->parents.size());
    assert(ranges::all_of(
               node->parents, [](const pair<LandmarkNode *, OrderingType> &parent) {
                   return parent.second >= OrderingType::NATURAL;
               }));
    for (auto &parent : views::keys(node->parents)) {
        parents.push_back(parent);
    }
    return parents;
}

void LandmarkFactoryRpgSasp::remove_disjunctive_landmark_and_rewire_orderings(
    LandmarkNode &atomic_landmark_node) {
    /*
      In issue1004, we fixed a bug in this part of the code. It now
      removes the disjunctive landmark along with all its orderings from
      the landmark graph and adds a new atomic landmark node. Before
      this change, incoming orderings were maintained, which is not
      always correct for greedy-necessary orderings. We now replace
      those incoming orderings with natural orderings.
    */
    const Landmark &landmark = atomic_landmark_node.get_landmark();
    assert(landmark.type == ATOMIC);
    LandmarkNode *disjunctive_landmark_node =
        &landmark_graph->get_disjunctive_landmark_node(landmark.atoms[0]);
    remove_occurrences_of_landmark_node(disjunctive_landmark_node);
    vector<LandmarkNode *> parents =
        get_natural_parents(disjunctive_landmark_node);
    assert(use_orders || parents.empty());
    landmark_graph->remove_node(disjunctive_landmark_node);
    /* Add incoming orderings of replaced `disjunctive_landmark_node` as
       natural orderings to `atomic_landmark_node`. */
    for (LandmarkNode *parent : parents) {
        add_or_replace_ordering_if_stronger(
            *parent, atomic_landmark_node, OrderingType::NATURAL);
    }
}

void LandmarkFactoryRpgSasp::add_atomic_landmark_and_ordering(
    const FactPair &atom, LandmarkNode &node, OrderingType type) {
    if (landmark_graph->contains_atomic_landmark(atom)) {
        if (use_orders) {
            LandmarkNode &atomic_landmark =
                landmark_graph->get_atomic_landmark_node(atom);
            add_or_replace_ordering_if_stronger(atomic_landmark, node, type);
        }
        return;
    }

    Landmark landmark({atom}, ATOMIC);
    LandmarkNode &atomic_landmark_node =
        landmark_graph->add_landmark(move(landmark));
    open_landmarks.push_back(&atomic_landmark_node);
    if (use_orders) {
        add_or_replace_ordering_if_stronger(atomic_landmark_node, node, type);
    }
    if (landmark_graph->contains_disjunctive_landmark(atom)) {
        // Simple landmarks are more informative than disjunctive ones.
        remove_disjunctive_landmark_and_rewire_orderings(atomic_landmark_node);
    }
}

// Returns true if an overlapping landmark exists already.
bool LandmarkFactoryRpgSasp::deal_with_overlapping_landmarks(
    const utils::HashSet<FactPair> &atoms, LandmarkNode &node,
    OrderingType type) const {
    if (ranges::any_of(atoms, [&](const FactPair &atom) {
                           return landmark_graph->contains_atomic_landmark(atom);
                       })) {
        /*
          Do not add the landmark because the atomic one is stronger. Do not add
          the ordering(s) to the corresponding atomic landmark(s) as they are
          not guaranteed to hold.
        */
        return true;
    }
    if (landmark_graph->contains_overlapping_disjunctive_landmark(atoms)) {
        if (use_orders &&
            landmark_graph->contains_superset_disjunctive_landmark(atoms)) {
            LandmarkNode &other =
                landmark_graph->get_disjunctive_landmark_node(*atoms.begin());
            add_or_replace_ordering_if_stronger(other, node, type);
        }
        return true;
    }
    return false;
}

void LandmarkFactoryRpgSasp::add_disjunctive_landmark_and_ordering(
    const utils::HashSet<FactPair> &atoms, LandmarkNode &node,
    OrderingType type) {
    assert(atoms.size() > 1);
    bool overlaps = deal_with_overlapping_landmarks(atoms, node, type);

    /* Only add the landmark to the landmark graph if it does not
       overlap with an existing landmark. */
    if (!overlaps) {
        Landmark landmark(
            vector<FactPair>(atoms.begin(), atoms.end()), DISJUNCTIVE);
        LandmarkNode *new_landmark_node =
            &landmark_graph->add_landmark(move(landmark));
        open_landmarks.push_back(new_landmark_node);
        if (use_orders) {
            add_or_replace_ordering_if_stronger(*new_landmark_node, node, type);
        }
    }
}

/* Compute the shared preconditions of all operators that can potentially
   achieve `landmark`, given the reachability in the relaxed planning graph. */
utils::HashSet<FactPair> LandmarkFactoryRpgSasp::compute_shared_preconditions(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const vector<vector<bool>> &reached) const {
    utils::HashSet<FactPair> shared_preconditions;
    bool init = true;
    for (const FactPair &atom : landmark.atoms) {
        const vector<int> &op_ids = get_operators_including_effect(atom);
        for (int op_or_axiom_id : op_ids) {
            OperatorProxy op =
                get_operator_or_axiom(task_proxy, op_or_axiom_id);
            if (possibly_reaches_landmark(op, reached, landmark)) {
                utils::HashSet<FactPair> preconditions =
                    approximate_preconditions_to_achieve_landmark(
                        task_proxy, landmark, op);
                if (init) {
                    swap(shared_preconditions, preconditions);
                    init = false;
                } else {
                    shared_preconditions =
                        get_intersection(shared_preconditions, preconditions);
                }
                if (shared_preconditions.empty()) {
                    return shared_preconditions;
                }
            }
        }
    }
    return shared_preconditions;
}

static string get_predicate_for_atom(const VariablesProxy &variables,
                                     int var_id, int value) {
    const string atom_name = variables[var_id].get_fact(value).get_name();
    if (atom_name == "<none of those>") {
        return "";
    }
    int predicate_pos = 0;
    if (atom_name.substr(0, 5) == "Atom ") {
        predicate_pos = 5;
    } else if (atom_name.substr(0, 12) == "NegatedAtom ") {
        predicate_pos = 12;
    }
    size_t paren_pos = atom_name.find('(', predicate_pos);
    if (predicate_pos == 0 || paren_pos == string::npos) {
        cerr << "Cannot extract predicate from atom: " << atom_name << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    return {
        atom_name.begin() + predicate_pos,
        atom_name.begin() + static_cast<int>(paren_pos)
    };
}

/*
  The RHW landmark generation method only allows disjunctive landmarks where all
  atoms stem from the same PDDL predicate. This functionality is implemented in
  this method.

  The approach we use is to map each atom (var/value pair) to an equivalence
  class (representing all atoms with the same predicate). The special class "-1"
  means "cannot be part of any disjunctive landmark". This is used for atoms
  that do not belong to any predicate.

  Similar methods for restricting disjunctive landmarks could be implemented by
  just changing this function, as long as the restriction could also be
  implemented as an equivalence class. For example, issue384 suggests to simply
  use the finite-domain variable ID as the equivalence class, which would be a
  cleaner method than what we currently use since it doesn't care about where
  the finite-domain representation comes from. (But of course making such a
  change would require a performance evaluation.)
*/
void LandmarkFactoryRpgSasp::build_disjunction_classes(
    const TaskProxy &task_proxy) {
    typedef unordered_map<string, int> PredicateIndex;
    PredicateIndex predicate_to_index;

    VariablesProxy variables = task_proxy.get_variables();
    disjunction_classes.resize(variables.size());
    for (VariableProxy var : variables) {
        int num_values = var.get_domain_size();
        disjunction_classes[var.get_id()].reserve(num_values);
        for (int value = 0; value < num_values; ++value) {
            string predicate =
                get_predicate_for_atom(variables, var.get_id(), value);
            int disjunction_class;
            if (predicate.empty()) {
                disjunction_class = -1;
            } else {
                /* Insert predicate into unordered_map or extract value
                   that is already there. */
                pair<string, int> entry(predicate, predicate_to_index.size());
                disjunction_class =
                    predicate_to_index.insert(entry).first->second;
            }
            disjunction_classes[var.get_id()].push_back(disjunction_class);
        }
    }
}

vector<int> LandmarkFactoryRpgSasp::get_operators_achieving_landmark(
    const Landmark &landmark) const {
    unordered_set<int> op_ids;
    for (const FactPair &atom : landmark.atoms) {
        const vector<int> &tmp_op_ids = get_operators_including_effect(atom);
        op_ids.insert(tmp_op_ids.begin(), tmp_op_ids.end());
    }
    return {op_ids.begin(), op_ids.end()};
}

void LandmarkFactoryRpgSasp::extend_disjunction_class_lookups(
    const utils::HashSet<FactPair> &landmark_preconditions, int op_id,
    unordered_map<int, vector<FactPair>> &preconditions,
    unordered_map<int, unordered_set<int>> &used_operators) const {
    for (auto [var, value] : landmark_preconditions) {
        int disjunction_class = disjunction_classes[var][value];
        if (disjunction_class == -1) {
            /* This atom may not participate in any disjunctive
               landmarks since it has no associated predicate. */
            continue;
        }

        /* Only deal with propositions that are not shared preconditions
           (which have been found already and are atomic landmarks). */
        FactPair precondition(var, value);
        if (!landmark_graph->contains_atomic_landmark(precondition)) {
            preconditions[disjunction_class].push_back(precondition);
            used_operators[disjunction_class].insert(op_id);
        }
    }
}

static vector<utils::HashSet<FactPair>> get_disjunctive_preconditions(
    const unordered_map<int, vector<FactPair>> &preconditions_by_disjunction_class,
    const unordered_map<int, unordered_set<int>> &used_operators_by_disjunction_class,
    int num_ops) {
    vector<utils::HashSet<FactPair>> disjunctive_preconditions;
    for (const auto &[disjunction_class, atoms] : preconditions_by_disjunction_class) {
        int used_operators = static_cast<int>(
            used_operators_by_disjunction_class.at(disjunction_class).size());
        if (used_operators == num_ops) {
            utils::HashSet<FactPair> preconditions(atoms.begin(), atoms.end());
            if (preconditions.size() > 1) {
                disjunctive_preconditions.push_back(preconditions);
            } // Otherwise this landmark is not actually a disjunctive landmark.
        }
    }
    return disjunctive_preconditions;
}

/*
  Compute disjunctive preconditions from all operators than can potentially
  achieve `landmark`, given the reachability in the relaxed planning graph.
  A disjunctive precondition is a set of atoms which contains one precondition
  atom from each of the operators, which we additionally restrict so that
  each atom in the set stems from the same disjunction class.
*/
vector<utils::HashSet<FactPair>> LandmarkFactoryRpgSasp::compute_disjunctive_preconditions(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const vector<vector<bool>> &reached) const {
    vector<int> op_or_axiom_ids =
        get_operators_achieving_landmark(landmark);
    int num_ops = 0;
    unordered_map<int, vector<FactPair>> preconditions_by_disjunction_class;
    unordered_map<int, unordered_set<int>> used_operators_by_disjunction_class;
    for (int op_id : op_or_axiom_ids) {
        const OperatorProxy &op =
            get_operator_or_axiom(task_proxy, op_id);
        if (possibly_reaches_landmark(op, reached, landmark)) {
            ++num_ops;
            utils::HashSet<FactPair> landmark_preconditions =
                approximate_preconditions_to_achieve_landmark(
                    task_proxy, landmark, op);
            extend_disjunction_class_lookups(
                landmark_preconditions, op_id,
                preconditions_by_disjunction_class,
                used_operators_by_disjunction_class);
        }
    }
    return get_disjunctive_preconditions(
        preconditions_by_disjunction_class,
        used_operators_by_disjunction_class, num_ops);
}

void LandmarkFactoryRpgSasp::generate_goal_landmarks(
    const TaskProxy &task_proxy) {
    for (FactProxy goal : task_proxy.get_goals()) {
        Landmark landmark({goal.get_pair()}, ATOMIC, true);
        LandmarkNode &node = landmark_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&node);
    }
}

void LandmarkFactoryRpgSasp::generate_shared_precondition_landmarks(
    const TaskProxy &task_proxy, const Landmark &landmark,
    LandmarkNode *node, const vector<vector<bool>> &reached) {
    utils::HashSet<FactPair> shared_preconditions =
        compute_shared_preconditions(task_proxy, landmark, reached);
    /* All shared preconditions are landmarks, and greedy-necessary
       predecessors of `landmark`. */
    for (const FactPair &atom : shared_preconditions) {
        add_atomic_landmark_and_ordering(
            atom, *node, OrderingType::GREEDY_NECESSARY);
    }
}

void LandmarkFactoryRpgSasp::generate_disjunctive_precondition_landmarks(
    const TaskProxy &task_proxy, const State &initial_state,
    const Landmark &landmark, LandmarkNode *node,
    const vector<vector<bool>> &reached) {
    vector<utils::HashSet<FactPair>> disjunctive_preconditions =
        compute_disjunctive_preconditions(task_proxy, landmark, reached);
    for (const auto &preconditions : disjunctive_preconditions) {
        /* We don't want disjunctive landmarks to get too big. Also,
           they should not hold in the initial state. */
        if (preconditions.size() < 5 && ranges::none_of(
                preconditions, [&](const FactPair &atom) {
                    return initial_state[atom.var].get_value() == atom.value;
                })) {
            add_disjunctive_landmark_and_ordering(
                preconditions, *node, OrderingType::GREEDY_NECESSARY);
        }
    }
}

void LandmarkFactoryRpgSasp::generate_backchaining_landmarks(
    const TaskProxy &task_proxy, Exploration &exploration) {
    State initial_state = task_proxy.get_initial_state();
    while (!open_landmarks.empty()) {
        LandmarkNode *node = open_landmarks.front();
        Landmark &landmark = node->get_landmark();
        open_landmarks.pop_front();
        assert(forward_orderings[node].empty());

        if (landmark.is_true_in_state(initial_state)) {
            continue;
        }
        vector<vector<bool>> reached =
            exploration.compute_relaxed_reachability(landmark.atoms, false);
        generate_shared_precondition_landmarks(
            task_proxy, landmark, node, reached);
        approximate_lookahead_orderings(task_proxy, reached, node);
        generate_disjunctive_precondition_landmarks(
            task_proxy, initial_state, landmark, node, reached);
    }
}

void LandmarkFactoryRpgSasp::generate_relaxed_landmarks(
    const shared_ptr<AbstractTask> &task, Exploration &exploration) {
    TaskProxy task_proxy(*task);
    if (log.is_at_least_normal()) {
        log << "Generating landmarks using the RPG/SAS+ approach" << endl;
    }
    build_dtg_successors(task_proxy);
    build_disjunction_classes(task_proxy);
    generate_goal_landmarks(task_proxy);
    generate_backchaining_landmarks(task_proxy, exploration);
    if (use_orders) {
        add_landmark_forward_orderings();
    }
    if (!disjunctive_landmarks) {
        discard_disjunctive_landmarks();
    }
}

/*
  Tests whether in the domain transition graph represented by `successors`,
  there is a path from `init_value` to `goal_value`, without passing through
  `excluded_value` or any unreachable value according to the relaxed planning
  graph. If this is not possible, that means `excluded_value` is crucial to
  achieve `goal_value`.
*/
static bool value_critical_to_reach_landmark(
    int init_value, int landmark_value, int excluded_value,
    const vector<bool> &reached, const vector<unordered_set<int>> &successors) {
    assert(landmark_value != init_value);
    assert(landmark_value != excluded_value);
    assert(!reached[landmark_value]);
    if (excluded_value == init_value) {
        return true;
    }
    deque<int> open;
    unordered_set<int> closed(reached.size());
    open.push_back(init_value);
    closed.insert(init_value);
    while (!open.empty()) {
        int value = open.front();
        open.pop_front();
        for (int succ : successors[value]) {
            if (succ == landmark_value) {
                return false;
            }
            if (!reached[succ] || succ == excluded_value) {
                /*
                  Values unreached in the delete relaxation cannot be landmarks
                  for `landmark_value` even if they are reachable in the DTG.
                  Also, we want to check whether it is possible to reach
                  `landmark_value` without going through `excluded_value`.
                */
                continue;
            }
            if (!closed.contains(succ)) {
                open.push_back(succ);
                closed.insert(succ);
            }
        }
    }
    return true;
}

static vector<int> get_critical_dtg_predecessors(
    int init_value, int landmark_value, const vector<bool> &reached,
    const vector<unordered_set<int>> &successors) {
    assert(!reached[landmark_value]);
    int domain_size = static_cast<int>(reached.size());
    vector<int> critical;
    critical.reserve(domain_size);
    for (int value = 0; value < domain_size; ++value) {
        if (reached[value] && value_critical_to_reach_landmark(
                init_value, landmark_value, value, reached, successors)) {
            critical.push_back(value);
        }
    }
    return critical;
}

// Extract orderings from the relaxed planning and domain transition graph.
void LandmarkFactoryRpgSasp::approximate_lookahead_orderings(
    const TaskProxy &task_proxy, const vector<vector<bool>> &reached,
    LandmarkNode *node) {
    const VariablesProxy &variables = task_proxy.get_variables();
    const Landmark &landmark = node->get_landmark();
    forward_orderings[node] = compute_atoms_unreachable_without_landmark(
        variables, landmark, reached);
    if (landmark.type != ATOMIC) {
        return;
    }
    assert(landmark.atoms.size() == 1);

    const FactPair landmark_atom = landmark.atoms[0];
    const FactPair init_atom =
        task_proxy.get_initial_state()[landmark_atom.var].get_pair();
    vector<int> critical_predecessors = get_critical_dtg_predecessors(
        init_atom.value, landmark_atom.value,
        reached[landmark_atom.var], dtg_successors[landmark_atom.var]);
    for (int value : critical_predecessors) {
        add_atomic_landmark_and_ordering(FactPair(landmark_atom.var, value),
                                         *node, OrderingType::NATURAL);
    }
}

bool LandmarkFactoryRpgSasp::atom_and_landmark_achievable_together(
    const FactPair &atom, const Landmark &landmark) const {
    assert(landmark.type != CONJUNCTIVE);
    for (const FactPair &landmark_atom : landmark.atoms) {
        if (atom == landmark_atom) {
            return true;
        }

        /* Make sure there is no operator that reaches both `atom` and
           `landmark_atom` at the same time. */
        const vector<int> &atom_achievers =
            get_operators_including_effect(atom);
        const vector<int> &landmark_achievers =
            get_operators_including_effect(landmark_atom);
        for (int atom_achiever_id : atom_achievers) {
            for (int landmark_achiever_id : landmark_achievers) {
                if (atom_achiever_id == landmark_achiever_id) {
                    return true;
                }
            }
        }
    }
    return false;
}

/*
  The landmark of `node` is ordered before any atom that cannot be reached
  before the landmark of `node` according to relaxed planning graph (as captured
  in `reached`). These orderings are saved in `forward_orderings` and added to
  the landmark graph in `add_landmark_forward_orderings` when it is known which
  atoms are actually landmarks.
*/
utils::HashSet<FactPair> LandmarkFactoryRpgSasp::compute_atoms_unreachable_without_landmark(
    const VariablesProxy &variables, const Landmark &landmark,
    const vector<vector<bool>> &reached) const {
    utils::HashSet<FactPair> unreachable_atoms;
    for (VariableProxy var : variables) {
        for (int value = 0; value < var.get_domain_size(); ++value) {
            FactPair atom(var.get_id(), value);
            if (!reached[atom.var][atom.value] &&
                !atom_and_landmark_achievable_together(atom, landmark)) {
                unreachable_atoms.insert(atom);
            }
        }
    }
    return unreachable_atoms;
}

void LandmarkFactoryRpgSasp::add_landmark_forward_orderings() {
    for (const auto &node : *landmark_graph) {
        for (const auto &node2_pair : forward_orderings[node.get()]) {
            if (landmark_graph->contains_atomic_landmark(node2_pair)) {
                LandmarkNode &node2 =
                    landmark_graph->get_atomic_landmark_node(node2_pair);
                add_or_replace_ordering_if_stronger(
                    *node, node2, OrderingType::NATURAL);
            }
        }
        forward_orderings[node.get()].clear();
    }
}

void LandmarkFactoryRpgSasp::discard_disjunctive_landmarks() const {
    /*
      Using disjunctive landmarks during landmark generation can be beneficial
      even if we don't want to use disjunctive landmarks during search. So we
      allow removing disjunctive landmarks after landmark generation.
    */
    if (landmark_graph->get_num_disjunctive_landmarks() > 0) {
        if (log.is_at_least_normal()) {
            log << "Discarding " << landmark_graph->get_num_disjunctive_landmarks()
                << " disjunctive landmarks" << endl;
        }
        landmark_graph->remove_node_if(
            [](const LandmarkNode &node) {
                return node.get_landmark().type == DISJUNCTIVE;
            });
    }
}

bool LandmarkFactoryRpgSasp::supports_conditional_effects() const {
    return true;
}

class LandmarkFactoryRpgSaspFeature
    : public plugins::TypedFeature<LandmarkFactory, LandmarkFactoryRpgSasp> {
public:
    LandmarkFactoryRpgSaspFeature() : TypedFeature("lm_rhw") {
        document_title("RHW Landmarks");
        document_synopsis(
            "The landmark generation method introduced by "
            "Richter, Helmert and Westphal (AAAI 2008).");

        add_option<bool>(
            "disjunctive_landmarks",
            "keep disjunctive landmarks",
            "true");
        add_use_orders_option_to_feature(*this);
        add_landmark_factory_options_to_feature(*this);

        document_language_support(
            "conditional_effects",
            "supported");
    }

    virtual shared_ptr<LandmarkFactoryRpgSasp> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryRpgSasp>(
            opts.get<bool>("disjunctive_landmarks"),
            get_use_orders_arguments_from_options(opts),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryRpgSaspFeature> _plugin;
}
