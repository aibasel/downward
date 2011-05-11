#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class AbstractOperator {
    int cost;
    std::vector<std::pair<int, int> > regression_preconditions; // normal effects and prevail combined
    //std::vector<std::pair<int, int> > regression_effects; // normal preconditions
    size_t hash_effect;
public:
    AbstractOperator(const std::vector<std::pair<int, int> > &prevail,
                     const std::vector<std::pair<int, int> > &conditions,
                     const std::vector<std::pair<int, int> > &effects, int cost,
                     const std::vector<size_t> &n_i);
    ~AbstractOperator();
    const std::vector<std::pair<int, int> > &get_regression_preconditions() const { return regression_preconditions; }
    //const std::vector<std::pair<int, int> > &get_regression_effects() const { return regression_effects; }
    size_t get_hash_effect() const { return hash_effect; }
    int get_cost() const { return cost; }
    void dump(const std::vector<int> &pattern) const;
};

// Implements a Successor Generator for abstract operators
class MatchTree {
    struct Node {
        Node(int test_var = -1, int test_var_size = 0);
        ~Node();
        std::vector<const AbstractOperator *> applicable_operators;
        int test_var;
        int var_size; // introduced for destructor (sizeof somehow didn't work)
        Node **successors;
        Node *star_successor;
    };
    std::vector<int> pattern; // as in PDBHeuristic
    std::vector<size_t> n_i; // as in PDBHeuristic
    Node *root;
    void build_recursively(const AbstractOperator &op, int pre_index, Node **edge_from_parent);
    void traverse(Node *node, size_t var_index, const size_t state_index,
                  std::vector<const AbstractOperator *> &applicable_operators) const;
public:
    MatchTree(const std::vector<int> &pattern, const std::vector<size_t> &n_i);
    ~MatchTree();
    void insert(const AbstractOperator &op); // recursively (calls build_recursively) builds/extends the MatchTree

    // recursively (calls traverse) goes through the MatchTree, converts state_index back to variable/values and
    // returns all applicable abstract operators
    void get_applicable_operators(size_t state_index,
                                  std::vector<const AbstractOperator *> &applicable_operators) const;
    void dump(Node *node = 0) const;
};

// Implements a single PDB
class Operator;
class State;
class PDBHeuristic : public Heuristic {
    std::vector<int> pattern;
    std::vector<int> operator_costs;
    std::vector<bool> used_operators;
    size_t num_states;
    std::vector<int> variable_to_index;
    std::vector<int> distances; // final h-values for abstract-states
    std::vector<size_t> n_i; // multipliers for perfect hash function
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only

    // build_abstract_operators computes all abstract operators for each normal operator
    // in the case of pre = -1, for each possible value of the concerned variable, an abstract
    // operator is computed. calls the recursive method "build_recursively"
    void build_recursively(int pos, int op_no, int cost,  std::vector<std::pair<int, int> > &prev_pairs,
                           std::vector<std::pair<int, int> > &pre_pairs,
                           std::vector<std::pair<int, int> > &eff_pairs,
                           const std::vector<std::pair<int, int> > &effects_without_pre,
                           std::vector<AbstractOperator> &operators);
    void build_abstract_operators(int op_no, std::vector<AbstractOperator> &operators);
    
    void create_pdb(); // builds the graph-structure and does a Dijkstra-backward-search
    void set_pattern(const std::vector<int> &pattern); // initializes n_i and num_states
    bool is_goal_state(const size_t state_index, const std::vector<std::pair<int, int> > &abstract_goal) const;
    size_t hash_index(const State &state) const; // maps a state to an index
    //AbstractState inv_hash_index(const size_t index) const; // inverts the hash-index-function (returns an abstract state)
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic(const Options &opts,
                 bool dump=true,
                 const std::vector<int> &op_costs=std::vector<int>());
    virtual ~PDBHeuristic();
    const std::vector<int> &get_pattern() const { return pattern; }
    const std::vector<int> &get_h_values() const { return distances; }
    const std::vector<bool> &get_used_ops() const { return used_operators; }
    void dump() const;
};

#endif
