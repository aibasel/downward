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

using namespace std;
using utils::ExitCode;

namespace landmarks {
static unordered_map<int, int> _intersect(
    const unordered_map<int, int> &a, const unordered_map<int, int> &b) {
    if (a.size() > b.size())
        return _intersect(b, a);
    unordered_map<int, int> result;
    for (const auto &pair_a : a) {
        const auto it_b = b.find(pair_a.first);
        if (it_b != b.end() && it_b->second == pair_a.second)
            result.insert(pair_a);
    }
    return result;
}

LandmarkFactoryRpgSasp::LandmarkFactoryRpgSasp(
    bool disjunctive_landmarks, bool use_orders, utils::Verbosity verbosity)
    : LandmarkFactoryRelaxation(verbosity),
      disjunctive_landmarks(disjunctive_landmarks),
      use_orders(use_orders) {
}

void LandmarkFactoryRpgSasp::build_dtg_successors(const TaskProxy &task_proxy) {
    // resize data structure
    VariablesProxy variables = task_proxy.get_variables();
    dtg_successors.resize(variables.size());
    for (VariableProxy var : variables)
        dtg_successors[var.get_id()].resize(var.get_domain_size());

    for (OperatorProxy op : task_proxy.get_operators()) {
        // build map for precondition
        unordered_map<int, int> precondition_map;
        for (FactProxy precondition : op.get_preconditions())
            precondition_map[precondition.get_variable().get_id()] = precondition.get_value();

        for (EffectProxy effect : op.get_effects()) {
            // build map for effect condition
            unordered_map<int, int> eff_condition;
            for (FactProxy effect_condition : effect.get_conditions())
                eff_condition[effect_condition.get_variable().get_id()] = effect_condition.get_value();

            // whenever the operator can change the value of a variable from pre to
            // post, we insert post into dtg_successors[var_id][pre]
            FactProxy effect_fact = effect.get_fact();
            int var_id = effect_fact.get_variable().get_id();
            int post = effect_fact.get_value();
            if (precondition_map.count(var_id)) {
                int pre = precondition_map[var_id];
                if (eff_condition.count(var_id) && eff_condition[var_id] != pre)
                    continue; // confliction pre- and effect condition
                add_dtg_successor(var_id, pre, post);
            } else {
                if (eff_condition.count(var_id)) {
                    add_dtg_successor(var_id, eff_condition[var_id], post);
                } else {
                    int dom_size = effect_fact.get_variable().get_domain_size();
                    for (int pre = 0; pre < dom_size; ++pre)
                        add_dtg_successor(var_id, pre, post);
                }
            }
        }
    }
}

void LandmarkFactoryRpgSasp::add_dtg_successor(int var_id, int pre, int post) {
    if (pre != post)
        dtg_successors[var_id][pre].insert(post);
}

/*
  Compute a subset of the actual preconditions of `op` for achieving `landmark`.
  It takes into account operator preconditions, but only reports those effect
  conditions that are true for ALL effects achieving the landmark.
  TODO: Make this comment clearer.
*/
void LandmarkFactoryRpgSasp::get_greedy_preconditions_for_landmark(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const OperatorProxy &op, unordered_map<int, int> &result) const {
    vector<bool> has_precondition_on_var(task_proxy.get_variables().size(), false);
    for (FactProxy precondition : op.get_preconditions()) {
        result.emplace(precondition.get_variable().get_id(), precondition.get_value());
        has_precondition_on_var[precondition.get_variable().get_id()] = true;
    }

    // If there is an effect but no precondition on a variable v with domain
    // size 2 and initially the variable has the other value than required by
    // the landmark then at the first time the landmark is reached the
    // variable must still have the initial value.
    State initial_state = task_proxy.get_initial_state();
    EffectsProxy effects = op.get_effects();
    for (EffectProxy effect : effects) {
        FactProxy effect_fact = effect.get_fact();
        int var_id = effect_fact.get_variable().get_id();
        if (!has_precondition_on_var[var_id] &&
            effect_fact.get_variable().get_domain_size() == 2) {
            for (const FactPair &atom : landmark.atoms) {
                if (atom.var == var_id &&
                    initial_state[var_id].get_value() != atom.value) {
                    result.emplace(var_id, initial_state[var_id].get_value());
                    break;
                }
            }
        }
    }

    // Check if `landmark` could be achieved by conditional effects.
    unordered_set<int> achievable_atom_indices;
    for (EffectProxy effect : effects) {
        FactProxy effect_fact = effect.get_fact();
        for (size_t j = 0; j < landmark.atoms.size(); ++j)
            if (landmark.atoms[j] == effect_fact.get_pair())
                achievable_atom_indices.insert(j);
    }
    // Intersect effect conditions of all effects that can achieve lmp
    unordered_map<int, int> intersection;
    bool init = true;
    for (int index : achievable_atom_indices) {
        for (EffectProxy effect : effects) {
            FactProxy effect_fact = effect.get_fact();
            if (!init && intersection.empty())
                break;
            unordered_map<int, int> current_cond;
            if (landmark.atoms[index] == effect_fact.get_pair()) {
                EffectConditionsProxy effect_conditions = effect.get_conditions();
                if (effect_conditions.empty()) {
                    intersection.clear();
                    break;
                } else {
                    for (FactProxy effect_condition : effect_conditions)
                        current_cond.emplace(effect_condition.get_variable().get_id(),
                                             effect_condition.get_value());
                }
            }
            if (init) {
                init = false;
                intersection = current_cond;
            } else
                intersection = _intersect(intersection, current_cond);
        }
    }
    result.insert(intersection.begin(), intersection.end());
}

void LandmarkFactoryRpgSasp::found_simple_landmark_and_ordering(
    const FactPair &atom, LandmarkNode &node, OrderingType type) {
    if (landmark_graph->contains_simple_landmark(atom)) {
        LandmarkNode &simple_landmark =
            landmark_graph->get_simple_landmark_node(atom);
        add_ordering(simple_landmark, node, type);
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
        add_ordering(simple_landmark, node, type);

        /* Add incoming orderings of replaced `disjunctive_landmark_node` as
           natural orderings to `simple_landmark`. */
        for (LandmarkNode *pred : predecessors) {
            add_ordering(*pred, simple_landmark, OrderingType::NATURAL);
        }
    } else {
        LandmarkNode &simple_landmark =
            landmark_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&simple_landmark);
        add_ordering(simple_landmark, node, type);
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
            add_ordering(*new_landmark_node, node, type);
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
    add_ordering(*new_landmark_node, node, type);
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
                unordered_map<int, int> next_pre;
                get_greedy_preconditions_for_landmark(task_proxy, landmark,
                                                      op, next_pre);
                if (init) {
                    init = false;
                    shared_pre = next_pre;
                } else
                    shared_pre = _intersect(shared_pre, next_pre);
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
            unordered_map<int, int> next_pre;
            get_greedy_preconditions_for_landmark(task_proxy, landmark, op, next_pre);
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
                add_ordering(*node, node2, OrderingType::NATURAL);
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
