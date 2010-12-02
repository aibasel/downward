#ifndef PDB_ABSTRACTION_H
#define PDB_ABSTRACTION_H

#include "state.h"

#include <vector>

class Edge {
public:
    int cost;
    size_t target;
    Edge(int c, size_t t) : cost(c), target(t) {}
    /*bool operator<(const Edge &other) const { // TODO: Get rid of this again
        return to_pair() < other.to_pair();
    }
    std::pair<size_t, int> to_pair() const { // TODO: Get rid of this again
        return std::make_pair(target, cost);
    }*/
};

class Operator;
class AbstractOperator {
    std::vector<std::pair<int, int> > conditions;
    std::vector<std::pair<int, int> > effects;
public:
    AbstractOperator(const Operator &op, const std::vector<int> &pattern);
    ~AbstractOperator();
    const std::vector<std::pair<int, int> > &get_conditions() const { return conditions; }
    const std::vector<std::pair<int, int> > &get_effects() const { return effects; }
    //bool is_applicable(const AbstractState &abstract_state) const;
    //AbstractState apply_operator(const AbstractState &abstract_state) const;
    void dump() const;
};

class AbstractState {
    std::vector<int> variable_values;
public:
    explicit AbstractState(const std::vector<int> &var_vals); // for construction after applying an operator
    AbstractState(const State &state, const std::vector<int> &pattern); // for construction from a concrete state
    ~AbstractState();
    int operator[](int index) const { return variable_values[index]; }
    int &operator[](int index) { return variable_values[index]; }
    bool is_applicable(const AbstractOperator &op, const std::vector<int> &var_to_index) const;
    void apply_operator(const AbstractOperator &op, const std::vector<int> &var_to_index);
    bool is_goal_state(const std::vector<std::pair<int, int> > &abstract_goal, const std::vector<int> &var_to_index) const;
    void dump() const;
};

// Impelements a single PDB
class PDBAbstraction {
    std::vector<int> pattern;
    int size;
    size_t num_states;
    std::vector<int> variable_to_index;
    std::vector<int> distances; // final h-values for abstract-states
    std::vector<int> n_i; 
    void create_pdb(); // builds the graph-structure and does a dijkstra-backward-search
    size_t hash_index(const AbstractState &state) const; // maps an abstract state to an index
    //AbstractState inv_hash_index(int index) const; // inverts the hash-index-function
public:
    explicit PDBAbstraction(const std::vector<int> &pattern);
    ~PDBAbstraction();
    int get_heuristic_value(const State &state) const; // returns the precomputed h-value (optimal cost 
        // in the abstraction induced by the pattern) for a state
    void dump() const;
};

#endif