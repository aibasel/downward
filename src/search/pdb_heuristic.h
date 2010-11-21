#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"
#include <queue>
using namespace std;

class Edge {
public:
    const Operator *op;
    size_t target;
    Edge(const Operator *o, size_t t) : op(o), target(t) {}
};

class AbstractState {
    map<int, int> variable_values; // maps variable to values
public:
    AbstractState(map<int, int> var_vals); // for construction after applying an operator
    AbstractState(const State &state, const vector<int> &pattern); // for construction from a concrete state
    map<int, int> get_variable_values() const;
    bool is_goal(const vector<pair<int, int> > &abstract_goal) const;
    void dump() const;
};

class Operator;
typedef pair<int, pair<int, int> > PreEffect;
class AbstractOperator {
    vector<PreEffect> pre_effect; // pair<int, pair<int, int> > : variable with value before and after operator.
        // Prevail (see operator.cc) is assumed to be non-existent, as we deal with SAS+ tasks.
public:
    AbstractOperator(const Operator &op, const vector<int> &pattern);
    bool is_applicable(const AbstractState &abstract_state) const;
    AbstractState apply_operator(const AbstractState &abstract_state) const;
    void dump() const;
};

typedef pair<int, int> Node;
struct compare {
    bool operator()(Node a, Node b) const {
        return a.second > b.second;
    }
};
#define QUITE_A_LOT 1000000000
// Impelements a single PDB
class PDBAbstraction {
    vector<int> pattern;
    size_t size; // number of abstract states
    vector<int> distances; // final h-values for abstract-states
    vector<vector<Edge > > back_edges; // contains the abstract state space in form of a graph
    vector<int> n_i; 
    priority_queue<Node, vector<Node>, compare> pq;
    void create_pdb(); // builds the graph-structure and everything needed for the backward-search
    void compute_goal_distances(); // does a dijkstra-backward-search
    int hash_index(const AbstractState &state) const; // maps an abstract state to an index
    AbstractState inv_hash_index(int index) const; // inverts the hash-index-function
public:
    PDBAbstraction(vector<int> pattern);
    int get_heuristic_value(const State &state) const; // returns the precomputed h-values for a 
        // concrete state when called from PDBHeuristic
    void dump() const;
};

class CanonicalHeuristic {
    vector<vector<int> > pattern_collection;
    vector<vector<int> > cgraph;
    vector<vector<int> > max_cliques;
    vector<int> q_clique;
    bool are_additive(int pattern1, int pattern2); // better name for are_additive?
    void build_cgraph();
    void compute_maximal_cliques();
    int get_maxi_vertex(vector<int> &subg, const vector<int> &cand); // TODO find nicer name :)
    void print_cgraph();
    void expand(vector<int> &subg, vector<int> &cand);
public:
    CanonicalHeuristic(vector<vector<int> > pat_coll);
    int get_heuristic_value(const State &state) const;
    void print_max_cliques();
};

class PDBHeuristic : public Heuristic {
    PDBAbstraction *pdb_abstraction;
    //CanonicalHeuristic *canonical_heuristic;
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic();
    ~PDBHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
