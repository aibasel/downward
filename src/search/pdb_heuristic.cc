#include "pdb_heuristic.h"
//#include "abstract_state_iterator.h"
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
#include <string>
#include <vector>

using namespace std;

// AbstractOperator ------------------------------------------------------------------------------.

AbstractOperator::AbstractOperator(const vector<pair<int, int> > &prev_pairs,
                                   const vector<pair<int, int> > &pre_pairs,
                                   const vector<pair<int, int> > &eff_pairs, int c,
                                   const vector<size_t> &n_i)
                                   : cost(c), regression_preconditions(prev_pairs) {
    for (size_t i = 0; i < eff_pairs.size(); ++i) {
        regression_preconditions.push_back(eff_pairs[i]);
    }
    sort(regression_preconditions.begin(), regression_preconditions.end()); // for MatchTree construction
    //regression_effects.reserve(pre_pairs.size());
    hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        //regression_effects.push_back(pre_pairs[i]);
        int var = pre_pairs[i].first;
        assert(var == eff_pairs[i].first);
        int old_val = eff_pairs[i].second;
        int new_val = pre_pairs[i].second;
        size_t effect = (new_val - old_val) * n_i[var];
        hash_effect += effect;
    }
}

AbstractOperator::~AbstractOperator() {
}

void AbstractOperator::dump(const vector<int> &pattern) const {
    cout << "AbstractOperator:" << endl;
    cout << "Regression preconditions:" << endl;
    for (size_t i = 0; i < regression_preconditions.size(); ++i) {
        cout << "Variable: " << regression_preconditions[i].first << " (True name: "
        << g_variable_name[pattern[regression_preconditions[i].first]] << ", Index: "
        << i << ") Value: " << regression_preconditions[i].second << endl;
    }
    /*cout << "Regression effects:" << endl;
    for (size_t i = 0; i < regression_effects.size(); ++i) {
        cout << "Variable: " << regression_effects[i].first << " (True name: "
        << g_variable_name[pattern[regression_effects[i].first]] << ") Value: " << regression_effects[i].second << endl;
    }*/
    cout << "Hash effect:" << hash_effect << endl;
}

// MatchTree --------------------------------------------------------------------------------------

MatchTree::MatchTree(const vector<int> &pattern_, const vector<size_t> &n_i_)
    : pattern(pattern_), n_i(n_i_), root(0) {
}

MatchTree::~MatchTree() {
    delete root;
}

MatchTree::Node::Node(int test_var_, int test_var_size) : test_var(test_var_), var_size(test_var_size), star_successor(0) {
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
    for (int i = 0; i < var_size; ++i) {
        delete successors[i];
    }
    delete[] successors;
    delete star_successor;
}

void MatchTree::build_recursively(
    const AbstractOperator &op, int pre_index, Node **edge_from_parent) {

    if (*edge_from_parent == 0) {
        // We don't exist yet: create a new node.
        *edge_from_parent = new Node();
    }

    const vector<pair<int, int> > &regression_preconditions = op.get_regression_preconditions();
    Node *node = *edge_from_parent;
    if (pre_index == regression_preconditions.size()) { // all preconditions have been checked, insert op
        //cout << "inserting operator" << endl;
        node->applicable_operators.push_back(&op);
    } else {
        const pair<int, int> &var_val = regression_preconditions[pre_index];

        if (node->test_var == -1) { // node is a leaf
            node->test_var = var_val.first;
            int test_var_size = g_variable_domain[pattern[var_val.first]];
            node->successors = new Node *[test_var_size];
            node->var_size = test_var_size;
            for (int i = 0; i < test_var_size; ++i) {
                node->successors[i] = 0;
            }
        } else if (node->test_var > var_val.first) {
            // The variable to test has been left out: must insert new node and treat it as the "node".
            Node *new_node = new Node(var_val.first, g_variable_domain[pattern[var_val.first]]);
            // The new node gets the left out variable as test_var.
            *edge_from_parent = new_node;
            new_node->star_successor = node;
            node = new_node; // The new node is now the node of interest.
        }

        Node **edge_to_child = 0;
        if (node->test_var == var_val.first) { // operator has a precondition on test_var
            edge_to_child = &node->successors[var_val.second];
            ++pre_index;
        } else { // operator doesn't have a precondition on test_var, follow/create *-edge
            assert(node->test_var < var_val.first);
            edge_to_child = &node->star_successor;
        }

        build_recursively(op, pre_index, edge_to_child);
    }
}

void MatchTree::insert(const AbstractOperator &op) {
    //cout << "inserting operator into MatchTree:" << endl;
    //op.dump(pattern);
    build_recursively(op, 0, &root);
    //cout << endl;
}

void MatchTree::traverse(Node *node, size_t var_index, const size_t state_index,
                         vector<const AbstractOperator *> &applicable_operators) const {
    //cout << "node->test_var = " << node->test_var << endl;
    //cout << "var_index = " << var_index << endl;
    if (!node->applicable_operators.empty()) {
        //cout << "applicable_operators.size() = " << node->applicable_operators.size() << endl;
        for (size_t i = 0; i < node->applicable_operators.size(); ++i) {
            applicable_operators.push_back(node->applicable_operators[i]);
        }
    } //else
        //cout << "no applicable operators at this node" << endl;
    if (var_index == pattern.size()) // all state vars have been checked
        return;
    if (node->test_var != -1) { // not a leaf
        while (var_index != node->test_var) {
            ++var_index;
            //cout << "skipping var index, next one is: " << var_index << endl;
            if (var_index == pattern.size()) { // I think this cannot happen in this if-block
                assert(false);
            }
        }
    } else
        return;
    int temp = state_index / n_i[var_index];
    int val = temp % node->var_size;
    //cout << "calculated value for var_index: " << val << endl;
    if (node->successors[val] != 0) { // no leaf reached
        //cout << "recursive call for child with value " << val << " of test_var" << endl;
        traverse(node->successors[val], var_index + 1, state_index, applicable_operators);
        //cout << "back from recursive call (for successors[" << val << "]) to node with test_var = " << node->test_var << endl;
    } //else
        //cout << "no child for this value of test_var" << endl;
    if (node->star_successor != 0) { // always follow the *-edge, if exists
        //cout << "recursive call for star_successor" << endl;
        traverse(node->star_successor, var_index + 1, state_index, applicable_operators);
        //cout << "back from recursive call (for star_successor) to node with test_var = " << node->test_var << endl;
    } //else
        //cout << "no star_successor" << endl;
}

void MatchTree::get_applicable_operators(size_t state_index,
                                         vector<const AbstractOperator *> &applicable_operators) const {
    if (root == 0) // empty MatchTree, i.e. pattern is empty
        return;
    //cout << "getting applicable operators for state_index = " << state_index << endl;
    traverse(root, 0, state_index, applicable_operators);
    //cout << endl;
}

void MatchTree::dump(Node *node) const {
    cout << endl;
    if (node == 0)
        node = root;
    if (node == 0) // root == 0
        cout << "Empty MatchTree" << endl;
    cout << "node->test_var = " << node->test_var << endl;
    if (node->applicable_operators.empty())
        cout << "no applicable operators at this node" << endl;
    else {
        cout << "applicable_operators.size() = " << node->applicable_operators.size() << endl;
        for (size_t i = 0; i < node->applicable_operators.size(); ++i) {
            node->applicable_operators[i]->dump(pattern);
        }
    }
    if (node->test_var == -1) {
        cout << "leaf node!" << endl;
        assert(node->successors == 0);
        assert(node->star_successor == 0);
    } else {
        for (int i = 0; i < node->var_size; ++i) {
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

PDBHeuristic::PDBHeuristic(
    const Options &opts, bool dump,
    const vector<int> &op_costs)
    : Heuristic(opts) {

    verify_no_axioms_no_cond_effects();

    if (op_costs.empty()) {
        operator_costs.reserve(g_operators.size());
        for (size_t i = 0; i < g_operators.size(); ++i)
            operator_costs.push_back(get_adjusted_cost(g_operators[i]));
    } else {
        assert(op_costs.size() == g_operators.size());
        operator_costs = op_costs;
    }
    used_operators.resize(g_operators.size(), false);

    Timer timer;
    set_pattern(opts.get_list<int>("pattern"));
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

void PDBHeuristic::build_recursively(int pos, int op_no, int cost, vector<pair<int, int> > &prev_pairs,
                                     vector<pair<int, int> > &pre_pairs,
                                     vector<pair<int, int> > &eff_pairs,
                                     const vector<pair<int, int> > &effects_without_pre,
                                     vector<AbstractOperator> &operators) {
    if (pos == effects_without_pre.size()) {
        if (!eff_pairs.empty()) {
            operators.push_back(AbstractOperator(prev_pairs, pre_pairs, eff_pairs, cost, n_i));
            used_operators[op_no] = true;
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
            build_recursively(pos+1, op_no, cost, prev_pairs, pre_pairs, eff_pairs,
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

void PDBHeuristic::build_abstract_operators(
    int op_no, vector<AbstractOperator> &operators) {
    const Operator &op = g_operators[op_no];
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
    build_recursively(0, op_no, operator_costs[op_no], prev_pairs, pre_pairs, eff_pairs, effects_without_pre, operators);
}

void PDBHeuristic::create_pdb() {
    vector<AbstractOperator> operators;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        build_abstract_operators(i, operators);
    }

    MatchTree match_tree(pattern, n_i);
    for (size_t j = 0; j < operators.size(); ++j) {
        match_tree.insert(operators[j]);
    }

    // compute abstract goal var-val pairs
    vector<pair<int, int> > abstract_goal;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (variable_to_index[g_goal[i].first] != -1) {
            abstract_goal.push_back(make_pair(variable_to_index[g_goal[i].first], g_goal[i].second));
        }
    }

    distances.reserve(num_states);
    AdaptiveQueue<size_t> pq; // (first implicit entry: priority,) second entry: index for an abstract state

    // initialize queue
    for (size_t state_index = 0; state_index < num_states; ++state_index) {
        if (is_goal_state(state_index, abstract_goal)) {
            pq.push(0, state_index);
            distances.push_back(0);
        }
        else {
            distances.push_back(numeric_limits<int>::max());
        }
    }

    // Dijkstra loop
    while (!pq.empty()) {
        pair<int, size_t> node = pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances[state_index]) {
            continue;
        }

        // regress abstract_state
        vector<const AbstractOperator *> applicable_operators;
        match_tree.get_applicable_operators(state_index, applicable_operators);
        for (size_t i = 0; i < applicable_operators.size(); ++i) {
            size_t predecessor = state_index + applicable_operators[i]->get_hash_effect();
            int alternative_cost = distances[state_index] + applicable_operators[i]->get_cost();
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
            }
        }
    }
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

bool PDBHeuristic::is_goal_state(const size_t state_index, const vector<pair<int, int> > &abstract_goal) const {
    for (size_t i = 0; i < abstract_goal.size(); ++i) {
        int var = abstract_goal[i].first;
        int temp = state_index / n_i[var];
        int val = temp % g_variable_domain[pattern[var]];
        if (val != abstract_goal[i].second) {
            return false;
        }
    }
    return true;
}

size_t PDBHeuristic::hash_index(const State &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += n_i[i] * state[pattern[i]];
    }
    return index;
}

/*AbstractState PDBHeuristic::inv_hash_index(const size_t index) const {
    vector<int> var_vals;
    var_vals.resize(pattern.size());
    for (size_t i = 0; i < pattern.size(); ++i) {
        int temp = index / n_i[i];
        var_vals[i] = temp % g_variable_domain[pattern[i]];
    }
    return AbstractState(var_vals);
}*/

void PDBHeuristic::initialize() {
}

int PDBHeuristic::compute_heuristic(const State &state) {
    int h = distances[hash_index(state)];
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
    if (parser.dry_run() && !pattern.empty()) {
        sort(pattern.begin(), pattern.end());
        int old_size = pattern.size();
        vector<int>::const_iterator it = unique(pattern.begin(), pattern.end());
        pattern.resize(it - pattern.begin());
        if (pattern.size() != old_size)
            parser.error("there are duplicates of variables in the pattern");
        if (pattern[0] < 0)
            parser.error("there is a variable < 0");
        if (pattern[pattern.size() - 1] >= g_variable_domain.size())
            parser.error("there is a variable > number of variables");
    }
    if (opts.get<int>("max_states") < 1)
        parser.error("abstraction size must be at least 1");

    if (parser.dry_run())
        return 0;

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
        opts.set("pattern", pattern);
    }

    return new PDBHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("pdb", _parse);
