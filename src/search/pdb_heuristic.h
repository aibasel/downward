#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"

#include <vector>

struct Edge {
    int cost;
    size_t target;
    Edge(int c, size_t t) : cost(c), target(t) {}
};

class Operator;
class AbstractOperator {
    int cost;
    std::vector<std::pair<int, int> > prevail;
    std::vector<std::pair<int, int> > conditions;
    std::vector<std::pair<int, int> > effects;
public:
    AbstractOperator(const Operator &o, const std::vector<int> &variable_to_index);
    AbstractOperator(const std::vector<std::pair<int, int> > &prevail,
                     const std::vector<std::pair<int, int> > &conditions,
                     const std::vector<std::pair<int, int> > &effects, int cost);
    ~AbstractOperator();
    const std::vector<std::pair<int, int> > &get_prevail() const { return prevail; }
    const std::vector<std::pair<int, int> > &get_conditions() const { return conditions; }
    const std::vector<std::pair<int, int> > &get_effects() const { return effects; }
    int get_cost() const { return cost; }
    void dump(const std::vector<int> &pattern) const;
};

class State;
class AbstractState {
    std::vector<int> variable_values;
public:
    explicit AbstractState(const std::vector<int> &variable_values); // for construction after applying an operator
    AbstractState(const State &state, const std::vector<int> &pattern); // for construction from a concrete state
    ~AbstractState();
    int operator[](int index) const { return variable_values[index]; }
    const std::vector<int> &get_var_vals() const { return variable_values; }
    bool is_applicable(const AbstractOperator &op) const;
    void apply_operator(const AbstractOperator &op);
    bool is_goal_state(const std::vector<std::pair<int, int> > &abstract_goal) const;
    void dump(const std::vector<int> &pattern) const;
};

// Implements a single PDB
class PDBHeuristic : public Heuristic {
    std::vector<int> pattern;
    size_t num_states;
    std::vector<int> variable_to_index;
    std::vector<int> distances; // final h-values for abstract-states
    //std::vector<int> distances2;
    std::vector<int> n_i; // multipliers for perfect hash function
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only
    void set_pattern(const std::vector<int> &pattern);
    void build_recursively(int pos, int cost, const std::vector<std::pair<int, int> > &prev_pairs,
                           std::vector<std::pair<int, int> > &pre_pairs,
                           std::vector<std::pair<int, int> > &eff_pairs,
                           const std::vector<std::pair<int, int> > &effects_without_pre,
                           std::vector<AbstractOperator> &operators);
    void build_abstract_operators(const Operator &op, std::vector<AbstractOperator> &operators);
    void create_pdb_new();
    void create_pdb(); // builds the graph-structure and does a dijkstra-backward-search
    size_t hash_index(const AbstractState &state) const; // maps an abstract state to an index
    AbstractState inv_hash_index(int index) const; // inverts the hash-index-function
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    //PDBHeuristic(int max_abstract_states);
    PDBHeuristic(const std::vector<int> &pattern);
    virtual ~PDBHeuristic();
    const std::vector<int> &get_pattern() const { return pattern; };
    const std::vector<int> &get_h_values() const { return distances; };
    void dump() const;
};

#endif
