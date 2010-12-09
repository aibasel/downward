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
    std::vector<std::pair<int, int> > conditions;
    std::vector<std::pair<int, int> > effects;
public:
    AbstractOperator(const Operator &o, const std::vector<int> &var_to_index);
    ~AbstractOperator();
    const std::vector<std::pair<int, int> > &get_conditions() const { return conditions; }
    const std::vector<std::pair<int, int> > &get_effects() const { return effects; }
    void dump(const std::vector<int> &pattern) const;
};

class State;
class AbstractState {
    std::vector<int> variable_values;
public:
    explicit AbstractState(const std::vector<int> &var_vals); // for construction after applying an operator
    AbstractState(const State &state, const std::vector<int> &pattern); // for construction from a concrete state
    ~AbstractState();
    int operator[](int index) const { return variable_values[index]; }
    bool is_applicable(const AbstractOperator &op) const;
    void apply_operator(const AbstractOperator &op);
    bool is_goal_state(const std::vector<std::pair<int, int> > &abstract_goal) const;
    void dump(const std::vector<int> &pattern) const;
};

// Implements a single PDB
class PDBAbstraction {
    std::vector<int> pattern;
    size_t num_states;
    std::vector<int> variable_to_index;
    std::vector<int> distances; // final h-values for abstract-states
    std::vector<int> n_i; // multipliers for perfect hash function
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only
    void create_pdb(); // builds the graph-structure and does a dijkstra-backward-search
    size_t hash_index(const AbstractState &state) const; // maps an abstract state to an index
    //AbstractState inv_hash_index(int index) const; // inverts the hash-index-function
public:
    explicit PDBAbstraction(const std::vector<int> &pattern);
    ~PDBAbstraction();
    int get_heuristic_value(const State &state) const; // returns the precomputed h-value (optimal cost 
    // in the abstraction induced by the pattern) for a state. int max value for dead end
    void dump() const;
};

class PDBHeuristic : public Heuristic {
    PDBAbstraction *pdb_abstraction;
    int max_abstract_states;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic(int max_abstract_states);
    virtual ~PDBHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config, int start, int &end, bool dry_run);
};

#endif
