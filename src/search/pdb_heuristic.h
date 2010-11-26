#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"
#include <vector>
#include <map>

class Edge {
public:
    int cost;
    size_t target;
    Edge(int c, size_t t) : cost(c), target(t) {}
};

class Operator;
//typedef pair<int, pair<int, int> > PreEffect;
class AbstractOperator {
    //vector<PreEffect> pre_effect; // pair<int, pair<int, int> > : variable with value before and after operator.
    // Prevail (see operator.cc) is assumed to be non-existent, as we deal with SAS+ tasks.
    std::vector<std::pair<int, int> > conditions;
    std::vector<std::pair<int, int> > effects;
public:
    AbstractOperator(const Operator &op, const std::vector<int> &pattern);
    const std::vector<std::pair<int, int> > &get_conditions() const { return conditions; }
    const std::vector<std::pair<int, int> > &get_effects() const { return effects; }
    //bool is_applicable(const AbstractState &abstract_state) const;
    //AbstractState apply_operator(const AbstractState &abstract_state) const;
    void dump() const;
};

class AbstractState {
    std::map<int, int> variable_values; // maps variable to values
public:
    AbstractState(std::map<int, int> var_vals); // for construction after applying an operator
    AbstractState(const State &state, const std::vector<int> &pattern); // for construction from a concrete state
    std::map<int, int> get_variable_values() const;
    int operator[](int index) const { return variable_values.find(index)->second; }
    bool is_applicable(const AbstractOperator &op) const;
    void apply_operator(const AbstractOperator &op);
    bool is_goal_state(const std::vector<std::pair<int, int> > &abstract_goal) const;
    void dump() const;
};

typedef std::pair<int, int> Node;
struct compare {
    bool operator()(Node a, Node b) const {
        return a.second > b.second;
    }
};
#define QUITE_A_LOT 1000000000
// Impelements a single PDB
class PDBAbstraction {
    std::vector<int> pattern;
    size_t num_states;
    std::vector<int> distances; // final h-values for abstract-states
    //std::vector<std::vector<Edge > > back_edges; // contains the abstract state space in form of a graph
    std::vector<int> n_i; 
    //priority_queue<Node, std::vector<Node>, compare> pq;
    void create_pdb(); // builds the graph-structure and everything needed for the backward-search
    //void compute_goal_distances(); // does a dijkstra-backward-search
    int hash_index(const AbstractState &state) const; // maps an abstract state to an index
    AbstractState inv_hash_index(int index) const; // inverts the hash-index-function
public:
    PDBAbstraction(std::vector<int> pattern);
    int get_heuristic_value(const State &state) const; // returns the precomputed h-value (optimal cost 
        // in the abstraction induced by the pattern) for a state
    void dump() const;
};


// Implements the canonical heuristic function.
class CanonicalHeuristic {
    std::vector<std::vector<int> > pattern_collection;
    std::vector<std::vector<int> > cgraph; // compatibility graph for the pattern collection TODO should not be global
    std::vector<std::vector<int> > max_cliques; // final computed max_cliques
    std::vector<int> q_clique; // TODO should not be global
    bool are_additive(int pattern1, int pattern2) const;
    void build_cgraph();
    int get_maxi_vertex(const std::vector<int> &subg, const std::vector<int> &cand) const; // TODO find nicer name :)
    void expand(std::vector<int> &subg, std::vector<int> &cand); // implements the CLIQUES-algorithmn from Tomita et al
public:
    std::map<int, PDBAbstraction> pattern_databases; // pattern in pattern collection --> final pdb
    CanonicalHeuristic(std::vector<std::vector<int> > pat_coll);
    int get_heuristic_value(const State &state) const; // returns the canonical heuristic value (with respect
        // to the pattern collection) for a state
    void dump() const;
};

class PDBHeuristic : public Heuristic {
    //PDBAbstraction *pdb_abstraction;
    CanonicalHeuristic *canonical_heuristic;
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic();
    ~PDBHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config, int start, int &end, bool dry_run);
};

#endif
