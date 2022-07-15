#include "landmark_factory_rpg_sasp.h"

#include "landmark.h"
#include "landmark_graph.h"
#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/system.h"

#include <cassert>
#include <limits>

using namespace std;
using utils::ExitCode;

namespace landmarks {
LandmarkFactoryRpgSasp::LandmarkFactoryRpgSasp(const Options &opts)
    : LandmarkFactoryRelaxation(opts),
      disjunctive_landmarks(opts.get<bool>("disjunctive_landmarks")),
      use_orders(opts.get<bool>("use_orders")),
      only_causal_landmarks(opts.get<bool>("only_causal_landmarks")) {
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

void LandmarkFactoryRpgSasp::get_greedy_preconditions_for_lm(
    const TaskProxy &task_proxy, const Landmark &landmark,
    const OperatorProxy &op, unordered_map<int, int> &result) const {
    // Computes a subset of the actual preconditions of o for achieving lmp - takes into account
    // operator preconditions, but only reports those effect conditions that are true for ALL
    // effects achieving the LM.

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
        if (!has_precondition_on_var[var_id] && effect_fact.get_variable().get_domain_size() == 2) {
            for (const FactPair &lm_fact : landmark.facts) {
                if (lm_fact.var == var_id &&
                    initial_state[var_id].get_value() != lm_fact.value) {
                    result.emplace(var_id, initial_state[var_id].get_value());
                    break;
                }
            }
        }
    }

    // Check for lmp in conditional effects
    set<int> lm_props_achievable;
    for (EffectProxy effect : effects) {
        FactProxy effect_fact = effect.get_fact();
        for (size_t j = 0; j < landmark.facts.size(); ++j)
            if (landmark.facts[j] == effect_fact.get_pair())
                lm_props_achievable.insert(j);
    }
    // Intersect effect conditions of all effects that can achieve lmp
    unordered_map<int, int> intersection;
    bool init = true;
    for (int lm_prop : lm_props_achievable) {
        for (EffectProxy effect : effects) {
            FactProxy effect_fact = effect.get_fact();
            if (!init && intersection.empty())
                break;
            unordered_map<int, int> current_cond;
            if (landmark.facts[lm_prop] == effect_fact.get_pair()) {
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

int LandmarkFactoryRpgSasp::min_cost_for_landmark(
    const TaskProxy &task_proxy, const Landmark &landmark,
    vector<vector<bool>> &reached) {
    int min_cost = numeric_limits<int>::max();
    // For each proposition in bp...
    for (const FactPair &lm_fact : landmark.facts) {
        // ...look at all achieving operators
        const vector<int> &op_or_axiom_ids = get_operators_including_eff(lm_fact);
        for (int op_or_axiom_id : op_or_axiom_ids) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axiom_id);
            // and calculate the minimum cost of those that can make
            // bp true for the first time according to reached
            if (possibly_reaches_lm(op, reached, landmark)) {
                min_cost = min(min_cost, op.get_cost());
            }
        }
    }
    return min_cost;
}

void LandmarkFactoryRpgSasp::found_simple_lm_and_order(
    const FactPair &a, LandmarkNode &b, EdgeType t) {
    if (lm_graph->contains_simple_landmark(a)) {
        LandmarkNode &simple_lm = lm_graph->get_simple_landmark(a);
        edge_add(simple_lm, b, t);
        return;
    }

    Landmark landmark({a}, false, false);
    if (lm_graph->contains_disjunctive_landmark(a)) {
        // In issue1004, we fixed a bug in this part of the code. It now removes
        // the disjunctive landmark along with all its orderings from the
        // landmark graph and adds a new simple landmark node. Before this
        // change, incoming orderings were maintained, which is not always
        // correct for greedy necessary orderings. We now replace those
        // incoming orderings with natural orderings.

        // Simple landmarks are more informative than disjunctive ones,
        // remove disj. landmark and add simple one
        LandmarkNode *disj_lm = &lm_graph->get_disjunctive_landmark(a);

        // Remove all pointers to disj_lm from internal data structures (i.e.,
        // the list of open landmarks and forward orders)
        auto it = find(open_landmarks.begin(), open_landmarks.end(), disj_lm);
        if (it != open_landmarks.end()) {
            open_landmarks.erase(it);
        }
        forward_orders.erase(disj_lm);

        // Retrieve incoming edges from disj_lm
        vector<LandmarkNode *> predecessors;
        for (auto &pred : disj_lm->parents) {
            predecessors.push_back(pred.first);
        }

        // Remove disj_lm from landmark graph
        lm_graph->remove_node(disj_lm);

        // Add simple landmark node
        LandmarkNode &simple_lm = lm_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&simple_lm);
        edge_add(simple_lm, b, t);

        // Add incoming orderings of replaced disj_lm as natural orderings to
        // simple_lm
        for (LandmarkNode *pred : predecessors) {
            edge_add(*pred, simple_lm, EdgeType::NATURAL);
        }
    } else {
        LandmarkNode &simple_lm = lm_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&simple_lm);
        edge_add(simple_lm, b, t);
    }
}

void LandmarkFactoryRpgSasp::found_disj_lm_and_order(
    const TaskProxy &task_proxy, const set<FactPair> &a,
    LandmarkNode &b, EdgeType t) {
    bool simple_lm_exists = false;
    // TODO: assign with FactPair::no_fact
    FactPair lm_prop = FactPair::no_fact;
    State initial_state = task_proxy.get_initial_state();
    for (const FactPair &lm : a) {
        if (initial_state[lm.var].get_value() == lm.value) {
            return;
        }
        if (lm_graph->contains_simple_landmark(lm)) {
            // Propositions in this disj. LM exist already as simple LMs.
            simple_lm_exists = true;
            lm_prop = lm;
            break;
        }
    }
    LandmarkNode *new_lm_node;
    if (simple_lm_exists) {
        // Note: don't add orders as we can't be sure that they're correct
        return;
    } else if (lm_graph->contains_overlapping_disjunctive_landmark(a)) {
        if (lm_graph->contains_identical_disjunctive_landmark(a)) {
            // LM already exists, just add order.
            new_lm_node = &lm_graph->get_disjunctive_landmark(*a.begin());
            edge_add(*new_lm_node, b, t);
            return;
        }
        // LM overlaps with existing disj. LM, do not add.
        return;
    }
    // This LM and no part of it exist, add the LM to the landmarks graph.
    Landmark landmark(vector<FactPair>(a.begin(), a.end()), true, false);
    new_lm_node = &lm_graph->add_landmark(move(landmark));
    open_landmarks.push_back(new_lm_node);
    edge_add(*new_lm_node, b, t);
}

void LandmarkFactoryRpgSasp::compute_shared_preconditions(
    const TaskProxy &task_proxy, unordered_map<int, int> &shared_pre,
    vector<vector<bool>> &reached, const Landmark &landmark) {
    /*
      Compute the shared preconditions of all operators that can potentially
      achieve landmark bp, given the reachability in the relaxed planning graph.
    */
    bool init = true;
    for (const FactPair &lm_fact : landmark.facts) {
        const vector<int> &op_ids = get_operators_including_eff(lm_fact);

        for (int op_or_axiom_id : op_ids) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axiom_id);
            if (!init && shared_pre.empty())
                break;

            if (possibly_reaches_lm(op, reached, landmark)) {
                unordered_map<int, int> next_pre;
                get_greedy_preconditions_for_lm(task_proxy, landmark,
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

static string get_predicate_for_fact(const VariablesProxy &variables,
                                     int var_no, int value) {
    const string fact_name = variables[var_no].get_fact(value).get_name();
    if (fact_name == "<none of those>")
        return "";
    int predicate_pos = 0;
    if (fact_name.substr(0, 5) == "Atom ") {
        predicate_pos = 5;
    } else if (fact_name.substr(0, 12) == "NegatedAtom ") {
        predicate_pos = 12;
    }
    size_t paren_pos = fact_name.find('(', predicate_pos);
    if (predicate_pos == 0 || paren_pos == string::npos) {
        cerr << "error: cannot extract predicate from fact: "
             << fact_name << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    return string(fact_name.begin() + predicate_pos, fact_name.begin() + paren_pos);
}

void LandmarkFactoryRpgSasp::build_disjunction_classes(const TaskProxy &task_proxy) {
    /* The RHW landmark generation method only allows disjunctive
       landmarks where all atoms stem from the same PDDL predicate.
       This functionality is implemented via this method.

       The approach we use is to map each fact (var/value pair) to an
       equivalence class (representing all facts with the same
       predicate). The special class "-1" means "cannot be part of any
       disjunctive landmark". This is used for facts that do not
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
            string predicate = get_predicate_for_fact(variables, var.get_id(), value);
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
    const TaskProxy &task_proxy, vector<set<FactPair>> &disjunctive_pre,
    vector<vector<bool>> &reached, const Landmark &landmark) {
    /*
      Compute disjunctive preconditions from all operators than can potentially
      achieve landmark bp, given the reachability in the relaxed planning graph.
      A disj. precondition is a set of facts which contains one precondition
      fact from each of the operators, which we additionally restrict so that
      each fact in the set stems from the same PDDL predicate.
    */

    vector<int> op_or_axiom_ids;
    for (const FactPair &lm_fact : landmark.facts) {
        const vector<int> &tmp_op_or_axiom_ids = get_operators_including_eff(lm_fact);
        for (int op_or_axiom_id : tmp_op_or_axiom_ids)
            op_or_axiom_ids.push_back(op_or_axiom_id);
    }
    int num_ops = 0;
    unordered_map<int, vector<FactPair>> preconditions;   // maps from
    // pddl_proposition_indeces to props
    unordered_map<int, set<int>> used_operators;  // tells for each
    // proposition which operators use it
    for (size_t i = 0; i < op_or_axiom_ids.size(); ++i) {
        OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axiom_ids[i]);
        if (possibly_reaches_lm(op, reached, landmark)) {
            ++num_ops;
            unordered_map<int, int> next_pre;
            get_greedy_preconditions_for_lm(task_proxy, landmark, op, next_pre);
            for (const auto &pre : next_pre) {
                int disj_class = disjunction_classes[pre.first][pre.second];
                if (disj_class == -1) {
                    // This fact may not participate in any disjunctive LMs
                    // since it has no associated predicate.
                    continue;
                }

                // Only deal with propositions that are not shared preconditions
                // (those have been found already and are simple landmarks).
                const FactPair pre_fact(pre.first, pre.second);
                if (!lm_graph->contains_simple_landmark(pre_fact)) {
                    preconditions[disj_class].push_back(pre_fact);
                    used_operators[disj_class].insert(i);
                }
            }
        }
    }
    for (const auto &pre : preconditions) {
        if (static_cast<int>(used_operators[pre.first].size()) == num_ops) {
            set<FactPair> pre_set;  // the set gets rid of duplicate predicates
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
        LandmarkNode &lm_node = lm_graph->add_landmark(move(landmark));
        open_landmarks.push_back(&lm_node);
    }

    State initial_state = task_proxy.get_initial_state();
    while (!open_landmarks.empty()) {
        LandmarkNode *lm_node = open_landmarks.front();
        Landmark &landmark = lm_node->get_landmark();
        open_landmarks.pop_front();
        assert(forward_orders[lm_node].empty());

        if (!landmark.is_true_in_state(initial_state)) {
            /*
              Backchain from *landmark* and compute greedy necessary
              predecessors.
              Firstly, collect which propositions can be reached without
              achieving the landmark.
            */
            vector<vector<bool>> reached =
                compute_relaxed_reachability(exploration, landmark);
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
                found_simple_lm_and_order(
                    FactPair(pre.first, pre.second), *lm_node,
                    EdgeType::GREEDY_NECESSARY);
            }
            // Extract additional orders from the relaxed planning graph and DTG.
            approximate_lookahead_orders(task_proxy, reached, lm_node);
            /*
              Use the information about possibly achieving operators of
              *landmark* to set its min cost.
            */
            landmark.cost =
                min_cost_for_landmark(task_proxy, landmark, reached);

            // Process achieving operators again to find disjunctive LMs
            vector<set<FactPair>> disjunctive_pre;
            compute_disjunctive_preconditions(
                task_proxy, disjunctive_pre, reached, landmark);
            for (const auto &preconditions : disjunctive_pre)
                // We don't want disjunctive LMs to get too big.
                if (preconditions.size() < 5) {
                    found_disj_lm_and_order(
                        task_proxy, preconditions, *lm_node,
                        EdgeType::GREEDY_NECESSARY);
                }
        }
    }
    add_lm_forward_orders();

    if (!disjunctive_landmarks) {
        discard_disjunctive_landmarks();
    }

    if (!use_orders) {
        discard_all_orderings();
    }

    if (only_causal_landmarks) {
        discard_noncausal_landmarks(task_proxy, exploration);
    }
}

void LandmarkFactoryRpgSasp::approximate_lookahead_orders(
    const TaskProxy &task_proxy, const vector<vector<bool>> &reached, LandmarkNode *lmp) {
    /*
      Find all var-val pairs that can only be reached after the landmark
      (according to relaxed plan graph as captured in reached).
      The result is saved in the node member variable forward_orders, and
      will be used later, when the phase of finding LMs has ended (because
      at the moment we don't know which of these var-val pairs will be LMs).
    */
    VariablesProxy variables = task_proxy.get_variables();
    find_forward_orders(variables, reached, lmp);

    /*
      Use domain transition graphs to find further orders. Only possible
      if lmp is a simple landmark.
    */
    const Landmark &landmark = lmp->get_landmark();
    if (landmark.disjunctive)
        return;
    const FactPair &lm_fact = landmark.facts[0];

    /*
      Collect in *unreached* all values of the LM variable that cannot be
      reached before the LM value (in the relaxed plan graph).
    */
    int domain_size = variables[lm_fact.var].get_domain_size();
    unordered_set<int> unreached(domain_size);
    for (int value = 0; value < domain_size; ++value)
        if (!reached[lm_fact.var][value] && lm_fact.value != value)
            unreached.insert(value);
    /*
      The set *exclude* will contain all those values of the LM variable that
      cannot be reached before the LM value (as in *unreached*) PLUS
      one value that CAN be reached.
    */
    State initial_state = task_proxy.get_initial_state();
    for (int value = 0; value < domain_size; ++value)
        if (unreached.find(value) == unreached.end() && lm_fact.value != value) {
            unordered_set<int> exclude(domain_size);
            exclude = unreached;
            exclude.insert(value);
            /*
              If that value is crucial for achieving the LM from the
              initial state, we have found a new landmark.
            */
            if (!domain_connectivity(initial_state, lm_fact, exclude))
                found_simple_lm_and_order(FactPair(lm_fact.var, value), *lmp, EdgeType::NATURAL);
        }
}

bool LandmarkFactoryRpgSasp::domain_connectivity(const State &initial_state,
                                                 const FactPair &landmark,
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

void LandmarkFactoryRpgSasp::find_forward_orders(const VariablesProxy &variables,
                                                 const vector<vector<bool>> &reached,
                                                 LandmarkNode *lm_node) {
    /*
      lm_node is ordered before any var-val pair that cannot be reached before
      lm_node according to relaxed planning graph (as captured in reached).
      These orders are saved in the node member variable "forward_orders".
    */
    for (VariableProxy var : variables)
        for (int value = 0; value < var.get_domain_size(); ++value) {
            if (reached[var.get_id()][value])
                continue;
            const FactPair fact(var.get_id(), value);

            bool insert = true;
            for (const FactPair &lm_fact : lm_node->get_landmark().facts) {
                if (fact != lm_fact) {
                    // Make sure there is no operator that reaches both lm and (var, value) at the same time
                    bool intersection_empty = true;
                    const vector<int> &reach_fact =
                        get_operators_including_eff(fact);
                    const vector<int> &reach_lm =
                        get_operators_including_eff(lm_fact);
                    for (size_t j = 0; j < reach_fact.size() && intersection_empty; ++j)
                        for (size_t k = 0; k < reach_lm.size()
                             && intersection_empty; ++k)
                            if (reach_fact[j] == reach_lm[k])
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
                forward_orders[lm_node].insert(fact);
        }
}

void LandmarkFactoryRpgSasp::add_lm_forward_orders() {
    for (auto &node : lm_graph->get_nodes()) {
        for (const auto &node2_pair : forward_orders[node.get()]) {
            if (lm_graph->contains_simple_landmark(node2_pair)) {
                LandmarkNode &node2 = lm_graph->get_simple_landmark(node2_pair);
                edge_add(*node, node2, EdgeType::NATURAL);
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
    if (lm_graph->get_num_disjunctive_landmarks() > 0) {
        if (log.is_at_least_normal()) {
            log << "Discarding " << lm_graph->get_num_disjunctive_landmarks()
                << " disjunctive landmarks" << endl;
        }
        lm_graph->remove_node_if(
            [](const LandmarkNode &node) {return node.get_landmark().disjunctive;});
    }
}

bool LandmarkFactoryRpgSasp::computes_reasonable_orders() const {
    return false;
}

bool LandmarkFactoryRpgSasp::supports_conditional_effects() const {
    return true;
}

static shared_ptr<LandmarkFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "RHW Landmarks",
        "The landmark generation method introduced by "
        "Richter, Helmert and Westphal (AAAI 2008).");

    parser.add_option<bool>("disjunctive_landmarks",
                            "keep disjunctive landmarks",
                            "true");
    add_landmark_factory_options_to_parser(parser);
    add_use_orders_option_to_parser(parser);
    add_only_causal_landmarks_option_to_parser(parser);

    Options opts = parser.parse();

    parser.document_language_support("conditional_effects",
                                     "supported");

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkFactoryRpgSasp>(opts);
}

static Plugin<LandmarkFactory> _plugin("lm_rhw", _parse);
}
