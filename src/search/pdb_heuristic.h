#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"

#include <vector>

/*struct Edge {
    int cost;
    size_t target;
    Edge(int c, size_t t) : cost(c), target(t) {}
};*/

class Operator;
class AbstractOperator {
    int cost;
    //std::vector<std::pair<int, int> > conditions;
    //std::vector<std::pair<int, int> > effects;
    std::vector<std::pair<int, int> > regression_preconditions; // normal effects and prevail combined
    //std::vector<std::pair<int, int> > regression_effects; // normal preconditions
    size_t hash_effect;
public:
    //AbstractOperator(const Operator &o, const std::vector<int> &variable_to_index);
    AbstractOperator(const std::vector<std::pair<int, int> > &prevail,
                     const std::vector<std::pair<int, int> > &conditions,
                     const std::vector<std::pair<int, int> > &effects, int cost,
                     const std::vector<size_t> &n_i);
    ~AbstractOperator();
    //const std::vector<std::pair<int, int> > &get_conditions() const { return conditions; }
    //const std::vector<std::pair<int, int> > &get_effects() const { return effects; }
    const std::vector<std::pair<int, int> > &get_regression_preconditions() const { return regression_preconditions; }
    //const std::vector<std::pair<int, int> > &get_regression_effects() const { return regression_effects; }
    size_t get_hash_effect() const { return hash_effect; }
    int get_cost() const { return cost; }
    //void dump(const std::vector<int> &pattern) const;
    void dump2(const std::vector<int> &pattern) const;
};

class State;
/*class AbstractState {
    std::vector<int> variable_values;
public:
    explicit AbstractState(const std::vector<int> &variable_values); // for construction after applying an operator
    AbstractState(const State &state, const std::vector<int> &pattern); // for construction from a concrete state
    ~AbstractState();
    int operator[](int index) const { return variable_values[index]; }
    const std::vector<int> &get_var_vals() const { return variable_values; }
    //bool is_applicable(const AbstractOperator &op) const;
    //void apply_operator(const AbstractOperator &op);
    bool is_goal_state(const std::vector<std::pair<int, int> > &abstract_goal) const;
    void dump(const std::vector<int> &pattern) const;
};*/

// Implements a Successor Generator for abstract operators
class MatchTree {
    struct Node {
        Node(int test_var = -1, int test_var_size = 0);
        ~Node();
        std::vector<const AbstractOperator *> applicable_operators;
        int test_var;
        Node **successors;
        int array_size;
        Node *star_successor;
    };
    std::vector<int> pattern; // as in PDBHeuristic TODO: idea - get rid of pattern by having a static method
    // in PDBHeuristic that returns the domain_size for a give variable (as index of PDBHeuristic::pattern)
    std::vector<size_t> n_i; // as in PDBHeuristic TODO: idea - get rid of n_i and use a static inv_hash_index method
    // (e.g. in PDBHeuristic) to get the value of a single variable for a given state index
    Node *root;
    void build_recursively(const AbstractOperator &op, int pre_index, int old_index, Node *node, Node *parent);
    void traverse(Node *node, size_t var_index, const size_t state_index,
                  std::vector<const AbstractOperator *> &applicable_operators) const;
public:
    MatchTree(const std::vector<int> &pattern, const std::vector<size_t> &n_i);
    ~MatchTree();
    void insert(const AbstractOperator &op);
    void get_applicable_operators(size_t state_index,
                                  std::vector<const AbstractOperator *> &applicable_operators) const;
    void dump(Node *node = 0) const;
};

// Implements a single PDB
class PDBHeuristic : public Heuristic {
    std::vector<int> pattern;
    size_t num_states;
    std::vector<int> variable_to_index;
    std::vector<int> distances; // final h-values for abstract-states
    std::vector<size_t> n_i; // multipliers for perfect hash function
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only
    void set_pattern(const std::vector<int> &pattern);
    void build_recursively(int pos, int cost,  std::vector<std::pair<int, int> > &prev_pairs,
                           std::vector<std::pair<int, int> > &pre_pairs,
                           std::vector<std::pair<int, int> > &eff_pairs,
                           const std::vector<std::pair<int, int> > &effects_without_pre,
                           std::vector<AbstractOperator> &operators);
    // computes all abstract operators for each normal operator. in the case of pre = -1, for each
    // possible value of the concerned variable, an abstract operator is computed. calls the
    // recursive method "build_recursively"
    void build_abstract_operators(const Operator &op, std::vector<AbstractOperator> &operators);
    void create_pdb(); // builds the graph-structure and does a Dijkstra-backward-search
    bool is_goal_state(const size_t state_index, const std::vector<std::pair<int, int> > &abstract_goal) const;
    size_t hash_index(const State &state) const; // maps a state to an index
    //AbstractState inv_hash_index(const size_t index) const; // inverts the hash-index-function (returns an abstract state)
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic(/*const Options &opts, */
                 const std::vector<int> &pattern, bool dump);
    virtual ~PDBHeuristic();
    const std::vector<int> &get_pattern() const { return pattern; };
    const std::vector<int> &get_h_values() const { return distances; };
    void dump() const;
};

#endif
