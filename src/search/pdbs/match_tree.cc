#include "match_tree.h"

#include "pdb_heuristic.h"

#include "../globals.h"

#include <cassert>
#include <iostream>

using namespace std;

struct MatchTree::Node {
    Node(int test_var = -1, int test_var_size = 0);
    ~Node();
    std::vector<const AbstractOperator *> applicable_operators;
    int test_var; // variable which this node represents
    int var_size;
    Node **successors; // edges with specific values for test_var_
    Node *star_successor; // star-edge (unspecified value for test_var)
};

MatchTree::Node::Node(int test_var_, int test_var_size) : test_var(test_var_), var_size(test_var_size), star_successor(0) {
    if (test_var_size == 0) { // construct a default node (test_var = -1), successors doesn't get initialized
        successors = 0;
    } else { // a test var has been specified, initialize node accordingly
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

MatchTree::MatchTree(const vector<int> &pattern_, const vector<size_t> &hash_multipliers_)
    : pattern(pattern_), hash_multipliers(hash_multipliers_), root(0) {
}

MatchTree::~MatchTree() {
    delete root;
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
        node->applicable_operators.push_back(&op);
    } else {
        const pair<int, int> &var_val = regression_preconditions[pre_index];

        // setup node correctly or insert a new node if necessary
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

        // setup edge to the correct child (for which we want to call this function recursively)
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
    build_recursively(op, 0, &root);
}

void MatchTree::traverse(Node *node, const size_t state_index,
                         vector<const AbstractOperator *> &applicable_operators) const {
    /*
      Note: different from the code that builds the match tree, we do
      the test if node == 0 *before* calling traverse rather than *at
      the start* of traverse since this turned out to be faster in
      some informal experiments.
     */

    applicable_operators.insert(applicable_operators.end(),
                                node->applicable_operators.begin(),
                                node->applicable_operators.end());

    // leaf reached, return
    if (node->test_var == -1)
        return;

    int var_index = node->test_var;
    int temp = state_index / hash_multipliers[var_index];
    int val = temp % node->var_size;

    if (node->successors[val] != 0) { // follow the correct successor-edge, if exists
        traverse(node->successors[val], state_index, applicable_operators);
    }
    if (node->star_successor != 0) { // always follow the *-edge, if exists
        traverse(node->star_successor, state_index, applicable_operators);
    }
}

void MatchTree::get_applicable_operators(size_t state_index,
                                         vector<const AbstractOperator *> &applicable_operators) const {
    if (root != 0)
        traverse(root, state_index, applicable_operators);
}

void MatchTree::_dump(Node *node) const {
    if (node == 0) { // root == 0
        cout << "Empty MatchTree" << endl;
        return;
    }
    cout << endl;
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
                _dump(node->successors[i]);
                cout << "back from recursive call (for successors[" << i << "]) to node with test_var = " << node->test_var << endl;
            }
        }
        if (node->star_successor == 0)
            cout << "no star_successor" << endl;
        else {
            cout << "recursive call for star_successor" << endl;
            _dump(node->star_successor);
            cout << "back from recursive call (for star_successor) to node with test_var = " << node->test_var << endl;
        }
    }
}

void MatchTree::dump() const {
    _dump(root);
}
