#include "landmark_factory_rpg_sasp.h"

#include "landmark_graph.h"
#include "util.h"

#include "../domain_transition_graph.h"
#include "../global_operator.h"
#include "../global_state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>

using namespace std;

LandmarkFactoryRpgSasp::LandmarkFactoryRpgSasp(const Options &opts)
    : LandmarkFactory(opts) {
}

void LandmarkFactoryRpgSasp::get_greedy_preconditions_for_lm(
    const LandmarkNode *lmp, const GlobalOperator &o, unordered_map<int, int> &result) const {
    // Computes a subset of the actual preconditions of o for achieving lmp - takes into account
    // operator preconditions, but only reports those effect conditions that are true for ALL
    // effects achieving the LM.

    vector<bool> has_precondition_on_var(g_variable_domain.size(), false);
    const vector<GlobalCondition> &preconditions = o.get_preconditions();
    for (size_t i = 0; i < preconditions.size(); ++i) {
        result.insert(make_pair(preconditions[i].var, preconditions[i].val));
        has_precondition_on_var[preconditions[i].var] = true;
    }

    // If there is an effect but no precondition on a variable v with domain
    // size 2 and initially the variable has the other value than required by
    // the landmark then at the first time the landmark is reached the
    // variable must still have the initial value.
    const GlobalState &initial_state = g_initial_state();
    const vector<GlobalEffect> &effects = o.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        int var = effects[i].var;
        if (!has_precondition_on_var[var] && g_variable_domain[var] == 2) {
            for (size_t j = 0; j < lmp->vars.size(); ++j) {
                if (lmp->vars[j] == var
                    && initial_state[var] != lmp->vals[j]) {
                    result.insert(make_pair(var, initial_state[var]));
                    break;
                }
            }
        }
    }

    // Check for lmp in conditional effects
    set<int> lm_props_achievable;
    for (size_t i = 0; i < effects.size(); ++i)
        for (size_t j = 0; j < lmp->vars.size(); ++j)
            if (lmp->vars[j] == effects[i].var && lmp->vals[j] == effects[i].val)
                lm_props_achievable.insert(j);
    // Intersect effect conditions of all effects that can achieve lmp
    unordered_map<int, int> intersection;
    bool init = true;
    set<int>::iterator it = lm_props_achievable.begin();
    for (; it != lm_props_achievable.end(); ++it) {
        for (size_t i = 0; i < effects.size(); ++i) {
            if (!init && intersection.empty())
                break;
            unordered_map<int, int> current_cond;
            if (lmp->vars[*it] == effects[i].var &&
                lmp->vals[*it] == effects[i].val) {
                if (effects[i].conditions.empty()) {
                    intersection.clear();
                    break;
                } else {
                    for (size_t j = 0; j < effects[i].conditions.size(); ++j)
                        current_cond.insert(make_pair(effects[i].conditions[j].var,
                                                      effects[i].conditions[j].val));
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

int LandmarkFactoryRpgSasp::min_cost_for_landmark(LandmarkNode *bp, vector<vector<
                                                                               int> > &lvl_var) {
    int min_cost = numeric_limits<int>::max();
    // For each proposition in bp...
    for (size_t i = 0; i < bp->vars.size(); ++i) {
        pair<int, int> b = make_pair(bp->vars[i], bp->vals[i]);
        // ...look at all achieving operators
        const vector<int> &ops = lm_graph->get_operators_including_eff(b);
        for (size_t j = 0; j < ops.size(); ++j) {
            const GlobalOperator &op = lm_graph->get_operator_for_lookup_index(ops[j]);
            // and calculate the minimum cost of those that can make
            // bp true for the first time according to lvl_var
            if (_possibly_reaches_lm(op, lvl_var, bp))
                min_cost = min(min_cost, get_adjusted_action_cost(op, lm_graph->get_lm_cost_type()));
        }
    }
    /*
      TODO: The following assertion fails for the unsolvable tasks that are
      created if the translator detects unsolvability. To reproduce, search
      with "astar(lmcount(lm_rhw()))" on mystery/prob07.pddl in debug mode.
      See issue 467
    */
    assert(min_cost < numeric_limits<int>::max());
    return min_cost;
}

void LandmarkFactoryRpgSasp::found_simple_lm_and_order(const pair<int, int> a,
                                                       LandmarkNode &b, edge_type t) {
    LandmarkNode *new_lm;
    if (lm_graph->simple_landmark_exists(a)) {
        new_lm = &lm_graph->get_simple_lm_node(a);
        edge_add(*new_lm, b, t);
        return;
    }
    set<pair<int, int> > a_set;
    a_set.insert(a);
    if (lm_graph->disj_landmark_exists(a_set)) {
        // Simple landmarks are more informative than disjunctive ones,
        // change disj. landmark into simple

        // old: call to methode
        LandmarkNode &node = lm_graph->make_disj_node_simple(a);

        /* TODO: Problem: Schon diese jetzige Implementierung ist nicht mehr korrekt,
        da rm_landmark_node nicht nur bei allen children die parents-zeiger auf sich selbst
        loescht, sondern auch bei allen parents die children-zeiger auf sich selbst. Ein
        einfaches Speichern aller Attribute von node funktioniert also nicht - entweder man
        muss dann manuell bei den parents des alten node alle children-Zeiger neu setzen auf
        den neuen node oder man ueberarbeitet das ganze komplett anders... Eine andere Vermutung
        meinerseits waere, dass die alte Version verbugt ist und eigentlich auch die children-
        Zeiger der parents von node geloescht werden muessten, wie es in rm_landmark_node passiert.
        */
        // TODO: avoid copy constructor, save attributes locally and assign to new lm
        // new: replace by new program logic
        /*LandmarkNode &node2 = lm_graph->get_disj_lm_node(a);
        LandmarkNode node(node2);
        lm_graph->rm_landmark_node(&node2);
        lm_graph->landmark_add_simple(a);*/

        node.vars.clear();
        node.vals.clear();
        node.vars.push_back(a.first);
        node.vals.push_back(a.second);
        // Clean orders: let disj. LM {D1,...,Dn} be ordered before L. We
        // cannot infer that any one of D1,...Dn by itself is ordered before L
        for (const auto &child : node.children) {
            child.first->parents.erase(&node);
        }
        node.children.clear();
        node.forward_orders.clear();

        edge_add(node, b, t);
        // Node has changed, reexamine it again. This also fixes min_cost.
        for (list<LandmarkNode *>::const_iterator it = open_landmarks.begin(); it
             != open_landmarks.end(); ++it)
            if (*it == &node)
                return;
        open_landmarks.push_back(&node);
    } else {
        new_lm = &lm_graph->landmark_add_simple(a);
        open_landmarks.push_back(new_lm);
        edge_add(*new_lm, b, t);
    }
}

void LandmarkFactoryRpgSasp::found_disj_lm_and_order(const set<pair<int, int> > a,
                                                     LandmarkNode &b, edge_type t) {
    bool simple_lm_exists = false;
    pair<int, int> lm_prop;
    const GlobalState &initial_state = g_initial_state();
    for (set<pair<int, int> >::iterator it = a.begin(); it != a.end(); ++it) {
        if (initial_state[it->first] == it->second) {
            //cout << endl << "not adding LM that's true in initial state: "
            //<< g_variable_name[it->first] << " -> " << it->second << endl;
            return;
        }
        if (lm_graph->simple_landmark_exists(*it)) {
            // Propositions in this disj. LM exist already as simple LMs.
            simple_lm_exists = true;
            lm_prop = *it;
            break;
        }
    }
    LandmarkNode *new_lm;
    if (simple_lm_exists) {
        // Note: don't add orders as we can't be sure that they're correct
        return;
    } else if (lm_graph->disj_landmark_exists(a)) {
        if (lm_graph->exact_same_disj_landmark_exists(a)) {
            // LM already exists, just add order.
            new_lm = &lm_graph->get_disj_lm_node(*a.begin());
            edge_add(*new_lm, b, t);
            return;
        }
        // LM overlaps with existing disj. LM, do not add.
        return;
    }
    // This LM and no part of it exist, add the LM to the landmarks graph.
    new_lm = &lm_graph->landmark_add_disjunctive(a);
    open_landmarks.push_back(new_lm);
    edge_add(*new_lm, b, t);
}

void LandmarkFactoryRpgSasp::compute_shared_preconditions(
    unordered_map<int, int> &shared_pre, vector<vector<int> > &lvl_var,
    LandmarkNode *bp) {
    /* Compute the shared preconditions of all operators that can potentially
     achieve landmark bp, given lvl_var (reachability in relaxed planning graph) */
    bool init = true;
    for (size_t i = 0; i < bp->vars.size(); ++i) {
        pair<int, int> b = make_pair(bp->vars[i], bp->vals[i]);
        const vector<int> &ops = lm_graph->get_operators_including_eff(b);

        for (size_t j = 0; j < ops.size(); ++j) {
            const GlobalOperator &op = lm_graph->get_operator_for_lookup_index(ops[j]);
            if (!init && shared_pre.empty())
                break;

            if (_possibly_reaches_lm(op, lvl_var, bp)) {
                unordered_map<int, int> next_pre;
                get_greedy_preconditions_for_lm(bp, op, next_pre);
                if (init) {
                    init = false;
                    shared_pre = next_pre;
                } else
                    shared_pre = _intersect(shared_pre, next_pre);
            }
        }
    }
}

static string get_predicate_for_fact(int var_no, int value) {
    const string &fact_name = g_fact_names[var_no][value];
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
        exit_with(EXIT_INPUT_ERROR);
    }
    return string(fact_name.begin() + predicate_pos, fact_name.begin() + paren_pos);
}

void LandmarkFactoryRpgSasp::build_disjunction_classes() {
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

    int num_variables = g_variable_domain.size();
    disjunction_classes.resize(num_variables);
    for (int var = 0; var < num_variables; ++var) {
        int range = g_variable_domain[var];
        disjunction_classes[var].reserve(range);
        for (int value = 0; value < range; ++value) {
            string predicate = get_predicate_for_fact(var, value);
            int disj_class;
            if (predicate.empty()) {
                disj_class = -1;
            } else {
                // Insert predicate into unordered_map or extract value that
                // is already there.
                pair<string, int> entry(predicate, predicate_to_index.size());
                disj_class = predicate_to_index.insert(entry).first->second;
            }
            disjunction_classes[var].push_back(disj_class);
        }
    }
}

void LandmarkFactoryRpgSasp::compute_disjunctive_preconditions(vector<set<pair<int,
                                                                               int> > > &disjunctive_pre, vector<vector<int> > &lvl_var,
                                                               LandmarkNode *bp) {
    /* Compute disjunctive preconditions from all operators than can potentially
     achieve landmark bp, given lvl_var (reachability in relaxed planning graph).
     A disj. precondition is a set of facts which contains one precondition fact
     from each of the operators, which we additionally restrict so that each fact
     in the set stems from the same PDDL predicate. */

    vector<int> ops;
    for (size_t i = 0; i < bp->vars.size(); ++i) {
        pair<int, int> b = make_pair(bp->vars[i], bp->vals[i]);
        const vector<int> &tmp_ops = lm_graph->get_operators_including_eff(b);
        for (size_t j = 0; j < tmp_ops.size(); ++j)
            ops.push_back(tmp_ops[j]);
    }
    int num_ops = 0;
    unordered_map<int, vector<pair<int, int> > > preconditions; // maps from
    // pddl_proposition_indeces to props
    unordered_map<int, set<int> > used_operators; // tells for each
    // proposition which operators use it
    for (size_t i = 0; i < ops.size(); ++i) {
        const GlobalOperator &op = lm_graph->get_operator_for_lookup_index(ops[i]);
        if (_possibly_reaches_lm(op, lvl_var, bp)) {
            ++num_ops;
            unordered_map<int, int> next_pre;
            get_greedy_preconditions_for_lm(bp, op, next_pre);
            for (const auto &pre : next_pre) {
                int disj_class = disjunction_classes[pre.first][pre.second];
                if (disj_class == -1) {
                    // This fact may not participate in any disjunctive LMs
                    // since it has no associated predicate.
                    continue;
                }

                // Only deal with propositions that are not shared preconditions
                // (those have been found already and are simple landmarks).
                if (!lm_graph->simple_landmark_exists(pre)) {
                    preconditions[disj_class].push_back(pre);
                    used_operators[disj_class].insert(i);
                }
            }
        }
    }
    for (const auto &pre : preconditions) {
        if (static_cast<int>(used_operators[pre.first].size()) == num_ops) {
            set<pair<int, int> > pre_set; // the set gets rid of duplicate predicates
            pre_set.insert(pre.second.begin(), pre.second.end());
            if (pre_set.size() > 1) { // otherwise this LM is not actually a disjunctive LM
                disjunctive_pre.push_back(pre_set);
            }
        }
    }
}

void LandmarkFactoryRpgSasp::generate_landmarks() {
    cout << "Generating landmarks using the RPG/SAS+ approach\n";
    build_disjunction_classes();

    for (size_t i = 0; i < g_goal.size(); ++i) {
        LandmarkNode &lmn = lm_graph->landmark_add_simple(g_goal[i]);
        lmn.in_goal = true;
        open_landmarks.push_back(&lmn);
    }

    while (!open_landmarks.empty()) {
        LandmarkNode *bp = open_landmarks.front();
        open_landmarks.pop_front();
        assert(bp->forward_orders.empty());

        if (!bp->is_true_in_state(g_initial_state())) {
            // Backchain from landmark bp and compute greedy necessary predecessors.
            // Firstly, collect information about the earliest possible time step in a
            // relaxed plan that propositions are achieved (in lvl_var) and operators
            // applied (in lvl_ops).
            vector<vector<int> > lvl_var;
            vector<unordered_map<pair<int, int>, int> > lvl_op;
            compute_predecessor_information(bp, lvl_var, lvl_op);
            // Use this information to determine all operators that can possibly achieve bp
            // for the first time, and collect any precondition propositions that all such
            // operators share (if there are any).
            unordered_map<int, int> shared_pre;
            compute_shared_preconditions(shared_pre, lvl_var, bp);
            // All such shared preconditions are landmarks, and greedy necessary predecessors of bp.
            for (const auto &pre : shared_pre) {
                found_simple_lm_and_order(pre, *bp, greedy_necessary);
            }
            // Extract additional orders from relaxed planning graph and DTG.
            approximate_lookahead_orders(lvl_var, bp);
            // Use the information about possibly achieving operators of bp to set its min cost.
            bp->min_cost = min_cost_for_landmark(bp, lvl_var);

            // Process achieving operators again to find disj. LMs
            vector<set<pair<int, int> > > disjunctive_pre;
            compute_disjunctive_preconditions(disjunctive_pre, lvl_var, bp);
            for (size_t i = 0; i < disjunctive_pre.size(); ++i)
                if (disjunctive_pre[i].size() < 5) { // We don't want disj. LMs to get too big
                    found_disj_lm_and_order(disjunctive_pre[i], *bp, greedy_necessary);
                }

        }
    }
    add_lm_forward_orders();
}

void LandmarkFactoryRpgSasp::approximate_lookahead_orders(
    const vector<vector<int> > &lvl_var, LandmarkNode *lmp) {
    // Find all var-val pairs that can only be reached after the landmark
    // (according to relaxed plan graph as captured in lvl_var)
    // the result is saved in the node member variable forward_orders, and will be
    // used later, when the phase of finding LMs has ended (because at the
    // moment we don't know which of these var-val pairs will be LMs).
    find_forward_orders(lvl_var, lmp);

    // Use domain transition graphs to find further orders. Only possible if lmp is
    // a simple landmark.
    if (lmp->disjunctive)
        return;
    const pair<int, int> lmk = make_pair(lmp->vars[0], lmp->vals[0]);

    // Collect in "unreached" all values of the LM variable that cannot be reached
    // before the LM value (in the relaxed plan graph)
    unordered_set<int> unreached(g_variable_domain[lmk.first]);
    for (int i = 0; i < g_variable_domain[lmk.first]; ++i)
        if (lvl_var[lmk.first][i] == numeric_limits<int>::max()
            && lmk.second != i)
            unreached.insert(i);
    // The set "exclude" will contain all those values of the LM variable that
    // cannot be reached before the LM value (as in "unreached") PLUS
    // one value that CAN be reached
    for (int i = 0; i < g_variable_domain[lmk.first]; ++i)
        if (unreached.find(i) == unreached.end() && lmk.second != i) {
            unordered_set<int> exclude(g_variable_domain[lmk.first]);
            exclude = unreached;
            exclude.insert(i);
            // If that value is crucial for achieving the LM from the initial state,
            // we have found a new landmark.
            if (!domain_connectivity(lmk, exclude))
                found_simple_lm_and_order(make_pair(lmk.first, i), *lmp, natural);
        }

}

bool LandmarkFactoryRpgSasp::domain_connectivity(const pair<int, int> &landmark,
                                                 const unordered_set<int> &exclude) {
    /* Tests whether in the domain transition graph of the LM variable, there is
     a path from the initial state value to the LM value, without passing through
     any value in "exclude". If not, that means that one of the values in "exclude"
     is crucial for achieving the landmark (i.e. is on every path to the LM).
     */
    const GlobalState &initial_state = g_initial_state();
    assert(landmark.second != initial_state[landmark.first]); // no initial state landmarks
    // The value that we want to achieve must not be excluded:
    assert(exclude.find(landmark.second) == exclude.end());
    // If the value in the initial state is excluded, we won't achieve our goal value:
    if (exclude.find(initial_state[landmark.first]) != exclude.end())
        return false;
    list<int> open;
    unordered_set<int> closed(g_variable_domain[landmark.first]);
    closed = exclude;
    open.push_back(initial_state[landmark.first]);
    closed.insert(initial_state[landmark.first]);
    while (closed.find(landmark.second) == closed.end()) {
        if (open.empty()) // landmark not in closed and nothing more to insert
            return false;
        const int c = open.front();
        open.pop_front();
        vector<int> succ;
        g_transition_graphs[landmark.first]->get_successors(c, succ);
        for (size_t i = 0; i < succ.size(); ++i)
            if (closed.find(succ[i]) == closed.end()) {
                open.push_back(succ[i]);
                closed.insert(succ[i]);
            }

    }
    return true;
}

void LandmarkFactoryRpgSasp::find_forward_orders(
    const vector<vector<int> > &lvl_var, LandmarkNode *lmp) {
    /* lmp is ordered before any var-val pair that cannot be reached before lmp according to
     relaxed planning graph (as captured in lvl_var).
     These orders are saved in the node member variable "forward_orders".
     */
    int num_variables = g_variable_domain.size();
    for (int var = 0; var < num_variables; ++var)
        for (int value = 0; value < g_variable_domain[var]; ++value) {
            if (lvl_var[var][value] != numeric_limits<int>::max())
                continue;

            bool insert = true;
            for (size_t i = 0; i < lmp->vars.size() && insert; ++i) {
                const pair<int, int> lm = make_pair(lmp->vars[i], lmp->vals[i]);

                if (make_pair(var, value) != lm) {
                    // Make sure there is no operator that reaches both lm and (var, value) at the same time
                    bool intersection_empty = true;
                    const vector<int> &reach_fact = lm_graph->get_operators_including_eff(
                        make_pair(var, value));
                    const vector<int> &reach_lm = lm_graph->get_operators_including_eff(
                        lm);

                    for (size_t j = 0; j < reach_fact.size() && intersection_empty; ++j)
                        for (size_t k = 0; k < reach_lm.size()
                             && intersection_empty; ++k)
                            if (reach_fact[j] == reach_lm[k])
                                intersection_empty = false;

                    if (!intersection_empty)
                        insert = false;
                } else
                    insert = false;
            }
            if (insert)
                lmp->forward_orders.insert(make_pair(var, value));
        }

}

void LandmarkFactoryRpgSasp::add_lm_forward_orders() {
    for (LandmarkNode *node : lm_graph->get_nodes()) {
        for (const auto &node2_pair : node->forward_orders) {
            if (lm_graph->simple_landmark_exists(node2_pair)) {
                LandmarkNode &node2 = lm_graph->get_simple_lm_node(node2_pair);
                edge_add(*node, node2, natural);
            }
        }
        node->forward_orders.clear();
    }
}

static LandmarkGraph *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "RHW Landmarks",
        "The landmark generation method introduced by "
        "Richter, Helmert and Westphal (AAAI 2008).");
    parser.document_note(
        "Relevant Options",
        "reasonable_orders, only_causal_landmarks, "
        "disjunctive_landmarks, no_orders");
    LandmarkGraph::add_options_to_parser(parser);

    Options opts = parser.parse();

    parser.document_language_support("conditional_effects",
                                     "supported");
    opts.set<bool>("supports_conditional_effects", true);

    if (parser.dry_run()) {
        return 0;
    } else {
        opts.set<Exploration *>("explor", new Exploration(opts));
        LandmarkFactoryRpgSasp lm_graph_factory(opts);
        LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
        return graph;
    }
}

static Plugin<LandmarkGraph> _plugin("lm_rhw", _parse);
