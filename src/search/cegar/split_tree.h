#ifndef CEGAR_SPLIT_TREE_H
#define CEGAR_SPLIT_TREE_H

#include "utils.h"
#include "../state.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

namespace cegar_heuristic {

class AbstractState;

class Node {
private:
    AbstractState *abs_state;
    int var;
    int value;
    Node *left_child;
    Node *right_child;
public:
    explicit Node(AbstractState *state = 0);
    void split(int var, int value, AbstractState *left, AbstractState *right);
    // Additionally to the two normal child nodes Node(left) and Node(right)
    // add |values|-1 OR-nodes that all have Node(right) as their right child
    // and the next OR-node as their left child.
    FRIEND_TEST(CegarTest, split_values);
    void split(int var, const vector<int> &values, AbstractState *left, AbstractState *right);
    bool is_split();
    int get_var() const;
    Node *get_child(int value) const;
};

class SplitTree {
private:
    Node *root;
public:
    explicit SplitTree(AbstractState *single);
    Node *get_node(const State conc_state);
    FRIEND_TEST(CegarTest, split_values);
};
}

#endif
