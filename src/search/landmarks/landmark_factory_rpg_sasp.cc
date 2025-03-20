#include "landmark_factory_rpg_sasp.h"

#include "exploration.h"
#include "landmark.h"
#include "landmark_graph.h"
#include "util.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#include <cassert>
#include <map>
#include <unordered_map>

using namespace std;
using utils::ExitCode;

namespace landmarks {
/* TODO: Can we combine this with the intersection defined in reasonable
    order factory? */
static unordered_map<int, int> get_intersection(
    const unordered_map<int, int> &map1, const unordered_map<int, int> &map2) {
    unordered_map<int, int> intersection;
    for (auto [key, value] : map1) {
        if (map2.contains(key) && map2.at(key) == value) {
            intersection[key] = value;
        }
    }
    return intersection;
}

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
    const EffectProxy &effect,
    const std::unordered_map<int, int> &preconditions,
    const std::unordered_map<int, int> &effect_conditions) {
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
    const OperatorProxy &op, unordered_map<int, int> &result) {
    unordered_set<int> precondition_variables;
    for (FactProxy precondition : op.get_preconditions()) {
        result[precondition.get_variable().get_id()] = precondition.get_value();
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
    unordered_map<int, int> &result) {
    State initial_state = task_proxy.get_initial_state();
    for (EffectProxy effect : effects) {
        FactProxy effect_atom = effect.get_fact();
        int var_id = effect_atom.get_variable().get_id();
        if (!precondition_variables.contains(var_id) &&
            effect_atom.get_variable().get_domain_size() == 2) {
            for (const FactPair &atom : landmark.atoms) {
                if (atom.var == var_id &&
                    initial_state[var_id].get_value() != atom.value) {
                    result[var_id] = initial_state[var_id].get_value();
                    break;
                }
            }
        }
    }
}

static void add_effect_conditions(
    const Landmark &landmark, const EffectsProxy &effects,
    unordered_map<int, int> &result) {
    // Intersect effect conditions of all effects that can achieve `landmark`.
    unordered_map<int, int> intersection;
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

        unordered_map<int, int> effect_condition;
        for (FactProxy atom : effect.get_conditions()) {
            effect_condition[atom.get_variable().get_id()] = atom.get_value();
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
static unordered_map<int, int> approximate_preconditions_to_achieve_landmark(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const OperatorProxy &op) {
    unordered_map<int, int> result;
    unordered_set<int> precondition_variables = add_preconditions(op, result);
    EffectsProxy effects = op.get_effects();
    add_binary_variable_conditions(
        task_proxy, landmark, effects, precondition_variables, result);
    add_effect_conditions(landmark, effects, result);
    return result;
}

void LandmarkFactoryRpgSasp::found_simple_landmark_and_ordering(
    const FactPair &atom, LandmarkNode &node, OrderingType type) {
    if (landmark_graph->contains_simple_landmark(atom)) {
        LandmarkNode &simple_landmark =
            landmark_graph->get_simple_landmark_node(atom);
        add_ordering_or_replace_if_stronger(simple_landmark, node, type);
        return;
    }

    Landmark landmark({atom}, false, false);
    if (landmark_graph->contains_disjunctive_landmark(atom)) {
        // In issue1004, we fixed a bug in this part of the code. It now removes
        // the disjunctive landmark along with all its orderings from the
        // landmark graph and adds a new simple landmark node. Before this
        // change, incoming orderings were maintained, which is not always
        // correct for greedy necessary orderings. We now replace those
        // incoming orderings with natural orderings.

        // Simple landmarks are more informative than disjunctive ones,
        // remove disj. landmark and add simple one
        LandmarkNode *disjunctive_landmark_node =
            &landmark_graph->get_disjunctive_landmark_node(atom);

        /* Remove all pointers to `disjunctive_landmark_node` from internal data
           structures (i.e., the list of open landmarks and forward orders). */
        auto it = find(open_landmarks.begin(), open_landmarks.end(), disjunctive_landmark_node);
        if (it != open_landmarks.end()) {
            open_landmarks.erase(it);
        }
        forward_orders.erase(disjunctive_landmark_node);

        // Retrieve incoming orderings from `disjunctive_landmark_node`.
        vector<LandmarkNode *> predecessors;
        predecessors.reserve(disjunctive_landmark_node->parents.size());
        for (auto &pred : disjunctive_landmark_node->parents) {
            predecessors.push_back(pred.first);
        }

        // Remove `disjunctive_landmark_node` from landmark graph.
        landmark_graph->remove_node(disjunctive_landmark_node);

        // Add simple landmark node.
        LandmarkNode &simple_landmark =
            landmark_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&simple_landmark);
        add_ordering_or_replace_if_stronger(simple_landmark, node, type);

        /* Add incoming orderings of replaced `disjunctive_landmark_node` as
           natural orderings to `simple_landmark`. */
        for (LandmarkNode *pred : predecessors) {
            add_ordering_or_replace_if_stronger(
                *pred, simple_landmark, OrderingType::NATURAL);
        }
    } else {
        LandmarkNode &simple_landmark =
            landmark_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&simple_landmark);
        add_ordering_or_replace_if_stronger(simple_landmark, node, type);
    }
}

void LandmarkFactoryRpgSasp::found_disjunctive_landmark_and_ordering(
    const TaskProxy &task_proxy, const set<FactPair> &atoms,
    LandmarkNode &node, OrderingType type) {
    bool simple_landmark_exists = false;
    State initial_state = task_proxy.get_initial_state();
    for (const FactPair &atom : atoms) {
        if (initial_state[atom.var].get_value() == atom.value) {
            return;
        }
        if (landmark_graph->contains_simple_landmark(atom)) {
            simple_landmark_exists = true;
            break;
        }
    }
    LandmarkNode *new_landmark_node;
    if (simple_landmark_exists) {
        // Note: don't add orders as we can't be sure that they're correct
        return;
    } else if (landmark_graph->contains_overlapping_disjunctive_landmark(atoms)) {
        if (landmark_graph->contains_identical_disjunctive_landmark(atoms)) {
            new_landmark_node =
                &landmark_graph->get_disjunctive_landmark_node(*atoms.begin());
            add_ordering_or_replace_if_stronger(*new_landmark_node, node, type);
            return;
        }
        // Landmark overlaps with existing disjunctive landmark, do not add.
        return;
    }
    /* None of the atoms in this landmark occur in an existing landmark, so
       we add the landmark to the landmark graph. */
    Landmark landmark(vector<FactPair>(atoms.begin(),
                                       atoms.end()), true, false);
    new_landmark_node = &landmark_graph->add_landmark(move(landmark));
    open_landmarks.push_back(new_landmark_node);
    add_ordering_or_replace_if_stronger(*new_landmark_node, node, type);
}

void LandmarkFactoryRpgSasp::compute_shared_preconditions(
    const TaskProxy &task_proxy, unordered_map<int, int> &shared_pre,
    vector<vector<bool>> &reached, const Landmark &landmark) {
    /*
      Compute the shared preconditions of all operators that can potentially
      achieve landmark bp, given the reachability in the relaxed planning graph.
    */
    bool init = true;
    for (const FactPair &atom : landmark.atoms) {
        const vector<int> &op_ids = get_operators_including_effect(atom);

        for (int op_or_axiom_id : op_ids) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axiom_id);
            if (!init && shared_pre.empty())
                break;

            if (possibly_reaches_landmark(op, reached, landmark)) {
                unordered_map<int, int> next_pre =
                    approximate_preconditions_to_achieve_landmark(
                        task_proxy, landmark, op);
                if (init) {
                    init = false;
                    shared_pre = next_pre;
                } else
                    shared_pre = get_intersection(shared_pre, next_pre);
            }
        }
    }
}

static string get_predicate_for_atom(const VariablesProxy &variables,
                                     int var_no, int value) {
    const string atom_name = variables[var_no].get_fact(value).get_name();
    if (atom_name == "<none of those>")
        return "";
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
    return string(atom_name.begin() +
                  predicate_pos, atom_name.begin() + paren_pos);
}

void LandmarkFactoryRpgSasp::build_disjunction_classes(
    const TaskProxy &task_proxy) {
    /* The RHW landmark generation method only allows disjunctive
       landmarks where all atoms stem from the same PDDL predicate.
       This functionality is implemented via this method.

       The approach we use is to map each atom (var/value pair) to an
       equivalence class (representing all atoms with the same
       predicate). The special class "-1" means "cannot be part of any
       disjunctive landmark". This is used for atoms that do not
       belong to any predicate.

       Similar methods for restricting disjunctive landmarks could be
       implemented by just changing this function, as long as the
       restriction could also be implemented as an equivalence class.
       For example, we might simply use the finite-domain variable
       number as the equivalence class, which would be a cleaner
       method than what we currently use since it doesn't care about
       where the finite-domain representation comes from. (But of
       course making such a change would require a performance
       evaluation.)
    */

    typedef map<string, int> PredicateIndex;
    PredicateIndex predicate_to_index;

    VariablesProxy variables = task_proxy.get_variables();
    disjunction_classes.resize(variables.size());
    for (VariableProxy var : variables) {
        int num_values = var.get_domain_size();
        disjunction_classes[var.get_id()].reserve(num_values);
        for (int value = 0; value < num_values; ++value) {
            string predicate = get_predicate_for_atom(variables, var.get_id(), value);
            int disj_class;
            if (predicate.empty()) {
                disj_class = -1;
            } else {
                // Insert predicate into unordered_map or extract value that
                // is already there.
                pair<string, int> entry(predicate, predicate_to_index.size());
                disj_class = predicate_to_index.insert(entry).first->second;
            }
            disjunction_classes[var.get_id()].push_back(disj_class);
        }
    }
}

void LandmarkFactoryRpgSasp::compute_disjunctive_preconditions(
    const TaskProxy &task_proxy,
    vector<set<FactPair>> &disjunctive_pre,
    vector<vector<bool>> &reached, const Landmark &landmark) {
    /*
      Compute disjunctive preconditions from all operators than can potentially
      achieve landmark bp, given the reachability in the relaxed planning graph.
      A disj. precondition is a set of atoms which contains one precondition
      atom from each of the operators, which we additionally restrict so that
      each atom in the set stems from the same PDDL predicate.
    */

    vector<int> op_or_axiom_ids;
    for (const FactPair &atom : landmark.atoms) {
        const vector<int> &tmp_op_or_axiom_ids =
            get_operators_including_effect(atom);
        for (int op_or_axiom_id : tmp_op_or_axiom_ids)
            op_or_axiom_ids.push_back(op_or_axiom_id);
    }
    int num_ops = 0;
    unordered_map<int, vector<FactPair>> preconditions;   // maps from
    // pddl_proposition_indeces to props
    unordered_map<int, unordered_set<int>> used_operators;  // tells for each
    // proposition which operators use it
    for (size_t i = 0; i < op_or_axiom_ids.size(); ++i) {
        OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axiom_ids[i]);
        if (possibly_reaches_landmark(op, reached, landmark)) {
            ++num_ops;
            unordered_map<int, int> next_pre =
                approximate_preconditions_to_achieve_landmark(
                    task_proxy, landmark, op);
            for (const auto &pre : next_pre) {
                int disj_class = disjunction_classes[pre.first][pre.second];
                if (disj_class == -1) {
                    /* This atom may not participate in any disjunctive
                       landmarks since it has no associated predicate. */
                    continue;
                }

                // Only deal with propositions that are not shared preconditions
                // (those have been found already and are simple landmarks).
                const FactPair precondition(pre.first, pre.second);
                if (!landmark_graph->contains_simple_landmark(precondition)) {
                    preconditions[disj_class].push_back(precondition);
                    used_operators[disj_class].insert(i);
                }
            }
        }
    }
    for (const auto &pre : preconditions) {
        if (static_cast<int>(used_operators[pre.first].size()) == num_ops) {
            set<FactPair> pre_set;
            pre_set.insert(pre.second.begin(), pre.second.end());
            if (pre_set.size() > 1) { // otherwise this LM is not actually a disjunctive LM
                disjunctive_pre.push_back(pre_set);
            }
        }
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

    for (FactProxy goal : task_proxy.get_goals()) {
        Landmark landmark({goal.get_pair()}, false, false, true);
        LandmarkNode &node = landmark_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&node);
    }

    State initial_state = task_proxy.get_initial_state();
    while (!open_landmarks.empty()) {
        LandmarkNode *node = open_landmarks.front();
        Landmark &landmark = node->get_landmark();
        open_landmarks.pop_front();
        assert(forward_orders[node].empty());

        if (!landmark.is_true_in_state(initial_state)) {
            /*
              Backchain from *landmark* and compute greedy necessary
              predecessors.
              Firstly, collect which propositions can be reached without
              achieving the landmark.
            */
            vector<vector<bool>> reached =
                exploration.compute_relaxed_reachability(landmark.atoms, false);
            /*
              Use this information to determine all operators that can
              possibly achieve *landmark* for the first time, and collect
              any precondition propositions that all such operators share
              (if there are any).
            */
            unordered_map<int, int> shared_pre;
            compute_shared_preconditions(task_proxy, shared_pre,
                                         reached, landmark);
            /*
              All such shared preconditions are landmarks, and greedy
              necessary predecessors of *landmark*.
            */
            for (const auto &pre : shared_pre) {
                found_simple_landmark_and_ordering(
                    FactPair(pre.first, pre.second), *node,
                    OrderingType::GREEDY_NECESSARY);
            }
            // Extract additional orders from the relaxed planning graph and DTG.
            approximate_lookahead_orders(task_proxy, reached, node);

            // Process achieving operators again to find disjunctive LMs
            vector<set<FactPair>> disjunctive_pre;
            compute_disjunctive_preconditions(
                task_proxy, disjunctive_pre, reached, landmark);
            for (const auto &preconditions : disjunctive_pre)
                // We don't want disjunctive LMs to get too big.
                if (preconditions.size() < 5) {
                    found_disjunctive_landmark_and_ordering(
                        task_proxy, preconditions, *node,
                        OrderingType::GREEDY_NECESSARY);
                }
        }
    }
    add_landmark_forward_orderings();

    if (!disjunctive_landmarks) {
        discard_disjunctive_landmarks();
    }

    /* TODO: Ensure that landmark orderings are not even added if
        `use_orders` is false. */
    if (!use_orders) {
        discard_all_orderings();
    }
}

void LandmarkFactoryRpgSasp::approximate_lookahead_orders(
    const TaskProxy &task_proxy, const vector<vector<bool>> &reached,
    LandmarkNode *node) {
    /*
      Find all var-val pairs that can only be reached after the landmark
      (according to relaxed plan graph as captured in reached).
      The result is saved in the node member variable forward_orders, and
      will be used later, when the phase of finding LMs has ended (because
      at the moment we don't know which of these var-val pairs will be LMs).
    */
    VariablesProxy variables = task_proxy.get_variables();
    find_forward_orders(variables, reached, node);

    /* Use domain transition graphs to find further orders. Only possible
       if landmark is simple. */
    const Landmark &landmark = node->get_landmark();
    if (landmark.is_disjunctive)
        return;
    const FactPair &atom = landmark.atoms[0];

    /*
      Collect in *unreached* all values of the LM variable that cannot be
      reached before the LM value (in the relaxed plan graph).
    */
    int domain_size = variables[atom.var].get_domain_size();
    unordered_set<int> unreached(domain_size);
    for (int value = 0; value < domain_size; ++value)
        if (!reached[atom.var][value] && atom.value != value)
            unreached.insert(value);
    /*
      The set *exclude* will contain all those values of the LM variable that
      cannot be reached before the LM value (as in *unreached*) PLUS
      one value that CAN be reached.
    */
    State initial_state = task_proxy.get_initial_state();
    for (int value = 0; value < domain_size; ++value)
        if (unreached.find(value) == unreached.end() && atom.value != value) {
            unordered_set<int> exclude(domain_size);
            exclude = unreached;
            exclude.insert(value);
            /*
              If that value is crucial for achieving the LM from the
              initial state, we have found a new landmark.
            */
            if (!domain_connectivity(initial_state, atom, exclude))
                found_simple_landmark_and_ordering(FactPair(atom.var, value), *node,
                                                   OrderingType::NATURAL);
        }
}

bool LandmarkFactoryRpgSasp::domain_connectivity(
    const State &initial_state, const FactPair &landmark,
    const unordered_set<int> &exclude) {
    /*
      Tests whether in the domain transition graph of the LM variable, there is
      a path from the initial state value to the LM value, without passing through
      any value in "exclude". If not, that means that one of the values in "exclude"
      is crucial for achieving the landmark (i.e. is on every path to the LM).
    */
    int var = landmark.var;
    assert(landmark.value != initial_state[var].get_value()); // no initial state landmarks
    // The value that we want to achieve must not be excluded:
    assert(exclude.find(landmark.value) == exclude.end());
    // If the value in the initial state is excluded, we won't achieve our goal value:
    if (exclude.find(initial_state[var].get_value()) != exclude.end())
        return false;
    list<int> open;
    unordered_set<int> closed(initial_state[var].get_variable().get_domain_size());
    closed = exclude;
    open.push_back(initial_state[var].get_value());
    closed.insert(initial_state[var].get_value());
    const vector<unordered_set<int>> &successors = dtg_successors[var];
    while (closed.find(landmark.value) == closed.end()) {
        if (open.empty()) // landmark not in closed and nothing more to insert
            return false;
        const int c = open.front();
        open.pop_front();
        for (int val : successors[c]) {
            if (closed.find(val) == closed.end()) {
                open.push_back(val);
                closed.insert(val);
            }
        }
    }
    return true;
}

void LandmarkFactoryRpgSasp::find_forward_orders(
    const VariablesProxy &variables, const vector<vector<bool>> &reached,
    LandmarkNode *node) {
    /*
      The landmark of `node` is ordered before any atom that cannot be reached
      before the landmark of `node` according to relaxed planning graph (as
      captured in `reached`). These orderings are saved in the `forward_orders`
      and added to the landmark graph in `add_landmark_forward_orderings`.
    */
    for (VariableProxy var : variables) {
        for (int value = 0; value < var.get_domain_size(); ++value) {
            if (reached[var.get_id()][value])
                continue;
            const FactPair atom(var.get_id(), value);

            bool insert = true;
            for (const FactPair &landmark_atom : node->get_landmark().atoms) {
                if (atom != landmark_atom) {
                    /* Make sure there is no operator that reaches both `atom`
                       and (var, value) at the same time. */
                    bool intersection_empty = true;
                    const vector<int> &atom_achievers =
                        get_operators_including_effect(atom);
                    const vector<int> &landmark_achievers =
                        get_operators_including_effect(landmark_atom);
                    for (size_t j = 0; j < atom_achievers.size() && intersection_empty; ++j)
                        for (size_t k = 0; k < landmark_achievers.size()
                             && intersection_empty; ++k)
                            if (atom_achievers[j] == landmark_achievers[k])
                                intersection_empty = false;

                    if (!intersection_empty) {
                        insert = false;
                        break;
                    }
                } else {
                    insert = false;
                    break;
                }
            }
            if (insert)
                forward_orders[node].insert(atom);
        }
    }
}

void LandmarkFactoryRpgSasp::add_landmark_forward_orderings() {
    for (const auto &node : *landmark_graph) {
        for (const auto &node2_pair : forward_orders[node.get()]) {
            if (landmark_graph->contains_simple_landmark(node2_pair)) {
                LandmarkNode &node2 =
                    landmark_graph->get_simple_landmark_node(node2_pair);
                add_ordering_or_replace_if_stronger(
                    *node, node2, OrderingType::NATURAL);
            }
        }
        forward_orders[node.get()].clear();
    }
}

void LandmarkFactoryRpgSasp::discard_disjunctive_landmarks() {
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
                return node.get_landmark().is_disjunctive;
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
