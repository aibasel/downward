#include "pdb_heuristic.h"
#include "abstract_state_iterator.h"
#include "globals.h"
#include "operator.h"
#include "plugin.h"
#include "priority_queue.h"
#include "raz_variable_order_finder.h"
#include "state.h"
#include "timer.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <limits>
#include <queue>
#include <string>
#include <vector>

using namespace std;

// AbstractOperator -------------------------------------------------------------------------------

AbstractOperator::AbstractOperator(const Operator &o, const vector<int> &var_to_index) : cost(o.get_cost()) {
    const vector<Prevail> &prevail = o.get_prevail();
    const vector<PrePost> &pre_post = o.get_pre_post();
    for (size_t j = 0; j < prevail.size(); ++j) {
        if (var_to_index[prevail[j].var] != -1) {
            conditions.push_back(make_pair(var_to_index[prevail[j].var], prevail[j].prev));
        }
    }
    for (size_t j = 0; j < pre_post.size(); ++j) {
        if (var_to_index[pre_post[j].var] != -1) {
            if (pre_post[j].pre != -1)
                conditions.push_back(make_pair(var_to_index[pre_post[j].var], pre_post[j].pre));
            effects.push_back(make_pair(var_to_index[pre_post[j].var], pre_post[j].post));
        }
    }
}

AbstractOperator::AbstractOperator(const vector<pair<int, int> > &prev_pairs,
                                   const vector<pair<int, int> > &pre_pairs,
                                   const vector<pair<int, int> > &eff_pairs, int c,
                                   const vector<int> &n_i)
                                   : cost(c), regression_preconditions(prev_pairs) {
    for (size_t i = 0; i < eff_pairs.size(); ++i) {
        regression_preconditions.push_back(eff_pairs[i]);
    }
    sort(regression_preconditions.begin(), regression_preconditions.end()); // for MatchTree construction
    regression_effects.reserve(pre_pairs.size());
    hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        regression_effects.push_back(pre_pairs[i]);
        int var = pre_pairs[i].first;
        assert(var == eff_pairs[i].first);
        int old_val = eff_pairs[i].second;
        int new_val = pre_pairs[i].second;
        int effect = (new_val - old_val) * n_i[var];
        hash_effect += effect;
    }
}

AbstractOperator::~AbstractOperator() {
}

void AbstractOperator::dump(const vector<int> &pattern) const {
    cout << "AbstractOperator:" << endl;
    cout << "Conditions:" << endl;
    for (size_t i = 0; i < conditions.size(); ++i) {
        cout << "Variable: " << conditions[i].first << " (True name: "
        << g_variable_name[pattern[conditions[i].first]] << ") Value: " << conditions[i].second << endl;
    }
    cout << "Effects:" << endl;
    for (size_t i = 0; i < effects.size(); ++i) {
        cout << "Variable: " << effects[i].first << " (True name: "
        << g_variable_name[pattern[effects[i].first]] << ") Value: " << effects[i].second << endl;
    }
}

void AbstractOperator::dump2(const vector<int> &pattern) const {
    cout << "AbstractOperator:" << endl;
    cout << "Regression preconditions:" << endl;
    for (size_t i = 0; i < regression_preconditions.size(); ++i) {
        cout << "Variable: " << regression_preconditions[i].first << " (True name: "
        << g_variable_name[pattern[regression_preconditions[i].first]] << ") Value: " << regression_preconditions[i].second << endl;
    }
    /*cout << "Regression effects:" << endl;
    for (size_t i = 0; i < regression_effects.size(); ++i) {
        cout << "Variable: " << regression_effects[i].first << " (True name: "
        << g_variable_name[pattern[regression_effects[i].first]] << ") Value: " << regression_effects[i].second << endl;
    }*/
    cout << "Hash effect:" << hash_effect << endl;
}

// AbstractState ----------------------------------------------------------------------------------

AbstractState::AbstractState(const vector<int> &var_vals) : variable_values(var_vals) {
}

AbstractState::AbstractState(const State &state, const vector<int> &pattern) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_values.push_back(state[pattern[i]]);
    }
}

AbstractState::~AbstractState() {
}

bool AbstractState::is_applicable(const AbstractOperator &op) const {
    const vector<pair<int, int> > &conditions = op.get_conditions();
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (variable_values[conditions[i].first] != conditions[i].second)
            return false;
    }
    return true;
}

void AbstractState::apply_operator(const AbstractOperator &op) {
    assert(is_applicable(op));
    const vector<pair<int, int> > &effects = op.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        int var = effects[i].first;
        int val = effects[i].second;
        variable_values[var] = val;
    }
}

bool AbstractState::is_goal_state(const vector<pair<int, int> > &abstract_goal) const {
    for (size_t i = 0; i < abstract_goal.size(); ++i) {
        if (variable_values[abstract_goal[i].first] != abstract_goal[i].second) {
            return false;
        }
    }
    return true;
}

void AbstractState::dump(const vector<int> &pattern) const {
    cout << "AbstractState: " << endl;
    for (size_t i = 0; i < variable_values.size(); ++i) {
        cout << "Variable: " << pattern[i] << " (True name: " 
        << g_variable_name[pattern[i]] << ") Value: " << variable_values[i] << endl;
    }
}

// MatchTree ------------------------------------------------------------------

MatchTree::MatchTree(const vector<int> &pattern_, const vector<int> &n_i_)
    : pattern(pattern_), n_i(n_i_), root(0) {
}

MatchTree::~MatchTree() {
    //TODO: delete all Nodes!
}

MatchTree::Node::Node(int test_var_, int test_var_size) : test_var(test_var_), star_successor(0) {
    if (test_var_size == 0) {
        successors = 0;
    } else {
        successors = new Node *[test_var_size];
        for (int i = 0; i < test_var_size; ++i) {
            successors[i] = 0;
        }
    }
}

MatchTree::Node::~Node() {
    delete[] successors;
    delete star_successor;
}

void MatchTree::build_recursively(const AbstractOperator &op, int pre_index, int old_index, Node *node, Node *parent) {
    cout << endl;
    cout << "node->test_var = " << node->test_var << endl;
    cout << "node->applicable_operators.size() = " << node->applicable_operators.size() << endl;
    cout << "pre_index = " << pre_index << endl;
    const vector<pair<int, int> > &regression_preconditions = op.get_regression_preconditions();
    if (pre_index == regression_preconditions.size()) { // all preconditions have been checked, insert op
        cout << "inserting operator" << endl;
        node->applicable_operators.push_back(&op);
    } else {
        const pair<int, int> &var_val = regression_preconditions[pre_index];
        if (node->test_var == var_val.first) { // operator has a precondition on test_var
            cout << "case 1: operator has precondition on actual test_var" << endl;
            if (node->successors[var_val.second] == 0) { // child with correct value doesn't exist, create
                cout << "child doesn't exist, create" << endl;
                Node *new_node = new Node();
                node->successors[var_val.second] = new_node;
                build_recursively(op, pre_index + 1, pre_index, new_node, node);
            } else { // child already exists, follow
                cout << "child exists, follow" << endl;
                build_recursively(op, pre_index + 1, pre_index, node->successors[var_val.second], node);
            }
        } else if (node->test_var == -1) { // node is a leaf
            cout << "case 2: leaf node" << endl;
            node->test_var = var_val.first;
            cout << "assigning new value of " << var_val.first << " to node->test_var" << endl;
            node->successors = new Node *[g_variable_domain[pattern[var_val.first]]];
            for (int i = 0; i < g_variable_domain[pattern[var_val.first]]; ++i) {
                node->successors[i] = 0;
            }
            Node *new_node = new Node();
            node->successors[var_val.second] = new_node;
            build_recursively(op, pre_index + 1, pre_index, new_node, node);
        } else if (node->test_var > var_val.first) { // variable has been left out
            cout << "case 3: variable has been left out" << endl;
            assert(parent != 0); // if node is root and therefore parent 0, we should never get in this if clause
            Node *new_node = new Node(var_val.first, g_variable_domain[pattern[var_val.first]]); // new node gets the left out variable as test_var
            if (old_index == pre_index) { // method was called from the last case, parent -> node is a *-edge
                assert(parent->star_successor == node);
                parent->star_successor = new_node; // parent points to new_node
            } else { // method was called from any other case, i.e. pre_index == old_index + 1, parent -> node is not a *-edge
                assert(parent->successors[regression_preconditions[old_index].second] == node);
                assert(old_index + 1 == pre_index);
                parent->successors[regression_preconditions[old_index].second] = new_node; // parent points to new_node
            }
            new_node->star_successor = node; // new_node's *-edge points to node
            Node *new_nodes_child = new Node();
            new_node->successors[var_val.second] = new_nodes_child; // new_node gets a default child
            build_recursively(op, pre_index + 1, pre_index, new_nodes_child, new_node);
        } else { // operator doesn't have a precondition on test_var, follow/create *-edge
            cout << "case 4: *-edge" << endl;
            if (node->star_successor == 0) { // *-edge doesn't exist, create
                cout << "*-edge doesn't exist, create" << endl;
                Node *new_node = new Node();
                node->star_successor = new_node;
                build_recursively(op, pre_index, pre_index, new_node, node);
            } else { // *-edge exists, follow
                cout << "*-edge exists, follow" << endl;
                build_recursively(op, pre_index, pre_index, node->star_successor, node);
            }
        }
    }
}

void MatchTree::insert(const AbstractOperator &op) {
    cout << "inserting operator into MatchTree:" << endl;
    op.dump2(pattern);
    if (root == 0)
        root = new Node(0, g_variable_domain[pattern[0]]); // initialize root-node with var0
    build_recursively(op, 0, 0, root, 0);
    cout << endl;
}

void MatchTree::traverse(Node *node, int var_index, const size_t state_index,
                         vector<const AbstractOperator *> &applicable_operators) const {
    cout << "node->test_var = " << node->test_var << endl;
    //TODO: handle the case where a variable has been left out in the MatchTree, var_index++ or something
    if (node->applicable_operators.empty())
        cout << "no applicable operators at this node" << endl;
    else {
        cout << "applicable_operators.size() = " << node->applicable_operators.size() << endl;
        for (size_t i = 0; i < node->applicable_operators.size(); ++i) {
            node->applicable_operators[i]->dump2(pattern);
            applicable_operators.push_back(node->applicable_operators[i]);
        }
    }
    cout << "var_index = " << var_index << endl;
    if (var_index == pattern.size()) // all state vars have been checked
        return;
    int temp = state_index / n_i[var_index];
    int val = temp % g_variable_domain[pattern[var_index]];
    if (node->successors[val] != 0) // no leaf reached
        traverse(node->successors[val], var_index + 1, state_index, applicable_operators);
    if (node->star_successor != 0) // always follow the *-edge, if exists
        traverse(node->star_successor, var_index + 1, state_index, applicable_operators);
}

void MatchTree::get_applicable_operators(size_t state_index,
                                         vector<const AbstractOperator *> &applicable_operators) const {
    cout << "getting applicable operators for state_index = " << state_index << endl;
    traverse(root, 0, state_index, applicable_operators);
    cout << endl;
}

void MatchTree::dump(Node *node) const {
    cout << endl;
    if (node == 0)
        node = root;
    cout << "node->test_var = " << node->test_var << endl;
    if (node->applicable_operators.empty())
        cout << "no applicable operators at this node" << endl;
    else {
        cout << "applicable_operators.size() = " << node->applicable_operators.size() << endl;
        for (size_t i = 0; i < node->applicable_operators.size(); ++i) {
            node->applicable_operators[i]->dump2(pattern);
        }
    }
    if (node->test_var == -1) {
        cout << "leaf node!" << endl;
        assert(node->successors == 0);
        assert(node->star_successor == 0);
    } else {
        for (int i = 0; i < g_variable_domain[pattern[node->test_var]]; ++i) {
            if (node->successors[i] == 0)
                cout << "no child for value " << i << " of test_var" << endl;
            else {
                cout << "recursive call for child with value " << i << " of test_var" << endl;
                dump(node->successors[i]);
                cout << "back from recursive call (for successors[" << i << "]) to node with test_var = " << node->test_var << endl;
            }
        }
        if (node->star_successor == 0)
            cout << "no star_successor" << endl;
        else {
            cout << "recursive call for star_successor" << endl;
            dump(node->star_successor);
            cout << "back from recursive call (for star_successor) to node with test_var = " << node->test_var << endl;
        }
    }
}

// PDBHeuristic ---------------------------------------------------------------

/*PDBHeuristic::PDBHeuristic(int max_abstract_states) {
    verify_no_axioms_no_cond_effects();
    Timer timer;
    generate_pattern(max_abstract_states);
    cout << "PDB construction time: " << timer << endl;
}*/

PDBHeuristic::PDBHeuristic(const vector<int> &pattern, bool dump)
    : Heuristic(Heuristic::default_options()) {
    /*
      TODO: support cost options.
     */
    verify_no_axioms_no_cond_effects();
    Timer timer;
    set_pattern(pattern);
    if (dump)
        cout << "PDB construction time: " << timer << endl;
}

PDBHeuristic::~PDBHeuristic() {
}

void PDBHeuristic::verify_no_axioms_no_cond_effects() const {
    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl << "Terminating." << endl;
        exit(1);
    }
    for (int i = 0; i < g_operators.size(); ++i) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for (int j = 0; j < pre_post.size(); ++j) {
            const vector<Prevail> &cond = pre_post[j].cond;
            if (cond.empty())
                continue;
            // Accept conditions that are redundant, but nothing else.
            // In a better world, these would never be included in the
            // input in the first place.
            int var = pre_post[j].var;
            int pre = pre_post[j].pre;
            int post = pre_post[j].post;
            if (pre == -1 && cond.size() == 1 &&
                cond[0].var == var && cond[0].prev != post &&
                g_variable_domain[var] == 2)
                continue;
            
            cerr << "Heuristic does not support conditional effects "
                 << "(operator " << g_operators[i].get_name() << ")"
                 << endl << "Terminating." << endl;
            exit(1);
        }
    }
}

void PDBHeuristic::build_recursively(int pos, int cost, vector<pair<int, int> > &prev_pairs,
                                     vector<pair<int, int> > &pre_pairs,
                                     vector<pair<int, int> > &eff_pairs,
                                     const vector<pair<int, int> > &effects_without_pre,
                                     vector<AbstractOperator> &operators) {
    if (pos == effects_without_pre.size()) {
        if (!eff_pairs.empty()) {
            operators.push_back(AbstractOperator(prev_pairs, pre_pairs, eff_pairs, cost, n_i));
        }
    } else {
        int var = effects_without_pre[pos].first;
        int eff = effects_without_pre[pos].second;
        for (size_t i = 0; i < g_variable_domain[pattern[var]]; ++i) {
            if (i != eff) {
                pre_pairs.push_back(make_pair(var, i));
                eff_pairs.push_back(make_pair(var, eff));
            } else {
                prev_pairs.push_back(make_pair(var, i));
            }
            build_recursively(pos+1, cost, prev_pairs, pre_pairs, eff_pairs,
                              effects_without_pre, operators);
            if (i != eff) {
                pre_pairs.pop_back();
                eff_pairs.pop_back();
            } else {
                prev_pairs.pop_back();
            }
        }
    }
}

void PDBHeuristic::build_abstract_operators(const Operator &op, vector<AbstractOperator> &operators) {
    vector<pair<int, int> > prev_pairs;
    vector<pair<int, int> > pre_pairs;
    vector<pair<int, int> > eff_pairs;
    vector<pair<int, int> > effects_without_pre;
    const vector<Prevail> &prevail = op.get_prevail();
    const vector<PrePost> &pre_post = op.get_pre_post();
    for (size_t i = 0; i < prevail.size(); ++i) {
        if (variable_to_index[prevail[i].var] != -1) { // variable occurs in pattern
            prev_pairs.push_back(make_pair(variable_to_index[prevail[i].var], prevail[i].prev));
        }
    }
    for (size_t i = 0; i < pre_post.size(); ++i) {
        if (variable_to_index[pre_post[i].var] != -1) {
            if (pre_post[i].pre != -1) {
                pre_pairs.push_back(make_pair(variable_to_index[pre_post[i].var], pre_post[i].pre));
                eff_pairs.push_back(make_pair(variable_to_index[pre_post[i].var], pre_post[i].post));
            } else {
                effects_without_pre.push_back(make_pair(variable_to_index[pre_post[i].var], pre_post[i].post));
            }
        }
    }
    build_recursively(0, op.get_cost(), prev_pairs, pre_pairs, eff_pairs, effects_without_pre, operators);
}

void PDBHeuristic::create_pdb() {
    vector<AbstractOperator> operators;
    //size_t index = 0;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        build_abstract_operators(g_operators[i], operators);
        /*g_operators[i].dump();
        cout << "abstracted and multiplied out operators:" << endl;
        while (index < operators.size()) {
            operators[i].dump(pattern);
            ++index;
        }*/
    }

    // so far still use the multiplied out operators in order to have no more
    // operators with pre = -1. Better: build abstract operators on the fly
    // while creating the op_tree
    /*MatchTree match_tree(pattern, n_i);
    for (size_t i = 0; i < operators.size(); ++i) {
        match_tree.insert(operators[i]);
    }*/
    //match_tree.dump();

    // old method for comparison reasons
    /*vector<AbstractOperator> operators2;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        AbstractOperator ao(g_operators[i], variable_to_index);
        if (!ao.get_effects().empty())
            operators2.push_back(ao);
    }*/

    // assertion tests for domain in which no operators with pre = -1 exist
    // in this case, both methods should generate the exact same abstract operators
    /*assert(operators.size() == operators2.size());
    for (size_t i = 0; i < operators.size(); ++i) {
        //cout << "i = " << i << endl;
        const vector<pair<int, int> > &conditions = operators[i].get_conditions();
        const vector<pair<int, int> > &conditions2 = operators2[i].get_conditions();
        assert(conditions.size() == conditions2.size());
        for (size_t j = 0; j < conditions.size(); ++j) {
            assert(conditions[j].first == conditions2[j].first);
            assert(conditions[j].second == conditions2[j].second);
        }
        const vector<pair<int, int> > &effects = operators[i].get_effects();
        const vector<pair<int, int> > &effects2 = operators2[i].get_effects();
        assert(effects.size() == effects2.size());
        for (size_t j = 0; j < effects.size(); ++j) {
            assert(effects[j].first == effects2[j].first);
            assert(effects[j].second == effects2[j].second);
        }
    }*/

    vector<pair<int, int> > abstracted_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (variable_to_index[g_goal[i].first] != -1) {
            abstracted_goal.push_back(make_pair(variable_to_index[g_goal[i].first], g_goal[i].second));
        }
    }

    // all data structures ending on "2" is just for comparison (implementing old create_pdb)
    /*vector<vector<Edge> > back_edges;
    back_edges.resize(num_states);
    vector<int> distances2;
    distances2.reserve(num_states);
    AdaptiveQueue<size_t> pq2; // (first implicit entry: priority,) second entry: index for an abstract state*/

    distances.reserve(num_states);
    AdaptiveQueue<size_t> pq; // (first implicit entry: priority,) second entry: index for an abstract state

    vector<int> ranges;
    for (size_t i = 0; i < pattern.size(); ++i) {
        ranges.push_back(g_variable_domain[pattern[i]]);
    }
    for (AbstractStateIterator it(ranges); !it.is_at_end(); it.next()) {
        AbstractState abstract_state(it.get_current());
        int counter = it.get_counter();
        assert(hash_index(abstract_state) == counter);

        if (abstract_state.is_goal_state(abstracted_goal)) {
            pq.push(0, counter);
            //pq2.push(0, counter);
            distances.push_back(0);
            //distances2.push_back(0);
        }
        else {
            distances.push_back(numeric_limits<int>::max());
            //distances2.push_back(numeric_limits<int>::max());
        }

        // old edges generation
        /*for (size_t j = 0; j < operators2.size(); ++j) {
            if (abstract_state.is_applicable(operators2[j])) {
                //cout << "applying operator:" << endl;
                //operators[j].dump(pattern);
                AbstractState next_state = abstract_state;
                next_state.apply_operator(operators2[j]);
                //next_state.dump(pattern);
                size_t state_index = hash_index(next_state);
                if (state_index == counter) {
                    //cout << "operator didn't lead to a new state, ignoring!" << endl;
                    continue;
                } else {
                    assert(counter != state_index);
                    back_edges[state_index].push_back(Edge(operators[j].get_cost(), counter));
                }
            }
        }*/
    }

    while (!pq.empty()/* && !pq2.empty()*/) {
        pair<int, size_t> node = pq.pop();
        //pair<int, size_t> node2 = pq2.pop();
        //cout << "popped (new method): " << node.first << " " << node.second << endl;
        //cout << "popped (old method): " << node2.first << " " << node2.second << endl;
        //assert(node == node2);
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances[state_index]) {
            //assert(distance > distances2[state_index]);
            continue;
        }

        // old Dijkstra
        /*const vector<Edge> &edges = back_edges[state_index];
        for (size_t i = 0; i < edges.size(); ++i) {
            size_t predecessor = edges[i].target;
            //cout << "predecessor index (old): " << predecessor << endl;
            int cost = edges[i].cost;
            int alternative_cost = distances2[state_index] + cost;
            if (alternative_cost < distances2[predecessor]) {
                distances2[predecessor] = alternative_cost;
                pq2.push(alternative_cost, predecessor);
            }
        }*/

        /*vector<const AbstractOperator *> applicable_operators;
        match_tree.get_applicable_operators(state_index, applicable_operators);
        cout << "applicable operators according to match_tree:" << endl;
        for (size_t i = 0; i < applicable_operators.size(); ++i) {
            applicable_operators[i]->dump2(pattern);
            cout << endl;
        }*/

        //cout << "applicable operators according to old method:" << endl;
        // regress abstract_state
        AbstractState abstract_state = inv_hash_index(state_index);
        for (size_t i = 0; i < operators.size(); ++i) {
            const vector<pair<int, int> > &regr_pre = operators[i].get_regression_preconditions();
            //const vector<pair<int, int> > &regr_eff = operators[i].get_regression_effects();
            int cost = operators[i].get_cost();
            bool eff_in_state = true;
            for (size_t j = 0; j < regr_pre.size(); ++j) {
                if (abstract_state[regr_pre[j].first] != regr_pre[j].second) {
                    eff_in_state = false;
                    break;
                }
            }
            if (eff_in_state) {
                //operators[i].dump2(pattern);
                //cout << endl;

                /*vector<int> var_vals = abstract_state.get_var_vals();
                for (size_t j = 0; j < regr_eff.size(); ++j)
                    var_vals[regr_eff[j].first] = regr_eff[j].second;
                AbstractState regressed_state(var_vals);
                size_t predecessor = hash_index(regressed_state);
                //cout << "predecessor index (new): " << predecessor << endl;*/
                size_t predecessor = state_index + operators[i].get_hash_effect();

                // Dijkstra
                int alternative_cost = distances[state_index] + cost;
                if (alternative_cost < distances[predecessor]) {
                    distances[predecessor] = alternative_cost;
                    pq.push(alternative_cost, predecessor);
                }
            }
        }
    }

    /*assert(distances2.size() == distances.size());
    for (size_t i = 0; i < distances2.size(); ++i) {
        if (distances2[i] != distances[i]) {
            cout << "distances2[" << i << "] = " << distances2[i] << " != " << distances[i] << " = distances[" << i << "]" << endl;
        }
        assert(distances2[i] == distances[i]);
    }
    cout << "assertion checked - distances correctly calculated" << endl;*/
    //cout << "done creating" << endl;
}

void PDBHeuristic::set_pattern(const vector<int> &pat) {
    pattern = pat;
    n_i.reserve(pattern.size());
    variable_to_index.resize(g_variable_name.size(), -1);
    num_states = 1;
    // int p = 1; ; same as num_states; incrementally computed!
    for (size_t i = 0; i < pattern.size(); ++i) {
        n_i.push_back(num_states);
        variable_to_index[pattern[i]] = i;
        num_states *= g_variable_domain[pattern[i]];
        //p *= g_variable_domain[pattern[i]];
    }
    create_pdb();
}

size_t PDBHeuristic::hash_index(const AbstractState &abstract_state) const {
    size_t index = 0;
    for (int i = 0; i < pattern.size(); ++i) {
        index += n_i[i] * abstract_state[variable_to_index[pattern[i]]];
    }
    return index;
}

AbstractState PDBHeuristic::inv_hash_index(int index) const {
    vector<int> var_vals;
    var_vals.resize(pattern.size());
    for (int n = 0; n < pattern.size(); ++n) {
        int temp = index / n_i[n];
        var_vals[n] = temp % g_variable_domain[pattern[n]];
    }

    /*vector<int> var_vals2;
    var_vals2.resize(pattern.size());
    for (int n = 1; n < pattern.size(); ++n) {
        int d = index % n_i[n];
        var_vals2[n - 1] = d / n_i[n - 1];
        index -= d;
    }
    var_vals2[pattern.size() - 1] = index / n_i[pattern.size() - 1];
    assert(var_vals.size() == var_vals2.size());
    for (size_t i = 0; i < var_vals.size(); ++i) {
        //cout << "alt: " << var_vals[i] << " neu: " << var_vals2[i] << endl;
        assert(var_vals[i] == var_vals2[i]);
    }*/

    return AbstractState(var_vals);
}

void PDBHeuristic::initialize() {
    //cout << "Initializing pattern database heuristic..." << endl;
    //cout << "Didn't do anything. Done initializing." << endl;
}

int PDBHeuristic::compute_heuristic(const State &state) {
    int h = distances[hash_index(AbstractState(state, pattern))];
    if (h == numeric_limits<int>::max())
        return -1;
    return h;
}

void PDBHeuristic::dump() const {
    for (size_t i = 0; i < num_states; ++i) {
        //AbstractState abs_state = inv_hash_index(i);
        //abs_state.dump();
        cout << "h-value: " << distances[i] << endl;
    }
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states", 1000000, "maximum abstraction size");
    parser.add_list_option<int>("pattern", vector<int>(), "the pattern");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    vector<int> pattern = opts.get_list<int>("pattern");
    if (!pattern.empty()) {
        //cout << "Reading pattern from option." << endl;
        // TODO: This code is called twice. Why?
        sort(pattern.begin(), pattern.end());
        int old_size = pattern.size();
        vector<int>::const_iterator it = unique(pattern.begin(), pattern.end());
        pattern.resize(it - pattern.begin());
        if (pattern.size() != old_size)
            parser.error("there are duplicates of variables in the pattern");
        if (pattern[0] < 0)
            parser.error("there is a variable < 0");
        if (pattern[pattern.size() - 1] > g_variable_domain.size())
            parser.error("there is a variable > number of variables");
        /*cout << "Pattern is ";
        for (size_t i = 0; i < pattern.size(); ++i) {
            cout << pattern[i] << ", ";
        }
        cout << endl;*/
    }
    if (opts.get<int>("max_states") < 1)
        parser.error("abstraction size must be at least 1");

    if (parser.dry_run())
        return 0;

#define DEBUG false
#if DEBUG
    // function tests
    // 1. blocks-7-2 test-pattern
    //int patt[] = {9, 10, 11, 12, 13, 14};
    //int patt[] = {13, 14};

    // 2. driverlog-6 test-pattern
    //int patt[] = {4, 5, 7, 9, 10, 11, 12};

    // 3. logistics00-6-2 test-pattern
    //int patt[] = {3, 4, 5, 6, 7, 8};

    // 4. blocks-9-0 test-pattern
    //int patt[] = {0};

    // 5. logistics00-5-1 test-pattern
    //int patt[] = {0, 1, 2, 3, 4, 5, 6, 7};

    // 6. some other test
    int patt[] = {9};

    pattern = vector<int>(patt, patt + sizeof(patt) / sizeof(int));
#else
    // if no pattern is specified as option
    if (pattern.empty()) {
        VariableOrderFinder vof(MERGE_LINEAR_GOAL_CG_LEVEL, 0.0);
        int var = vof.next();
        int num_states = g_variable_domain[var];
        while (num_states <= opts.get<int>("max_states")) {
            //cout << "Number of abstract states = " << num_states << endl;
            //cout << "Including variable: " << var << " (True name:" << g_variable_name[var] << ")" << endl;
            pattern.push_back(var);
            if (!vof.done()) {
                var = vof.next();
                num_states *= g_variable_domain[var];
            }
            else
                break;
        }
    }

#endif
    return new PDBHeuristic(/*opts, */pattern, true);
}

static Plugin<ScalarEvaluator> _plugin("pdb", _parse);
