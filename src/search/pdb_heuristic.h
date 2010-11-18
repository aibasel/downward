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
    vector<int> variable_values;
public:
    AbstractState(vector<int> var_vals);
    AbstractState(const State &state, const vector<int> &pattern);
    vector<int> get_variable_values() const;
    bool is_goal(const vector<pair<int, int> > &abstract_goal) const;
    void dump() const;
    int operator[](int index) const { return variable_values[index]; }
};

class Operator;
typedef pair<int, pair<int, int> > PreEffect;
class AbstractOperator {
    vector<PreEffect> pre_effect;
public:
    AbstractOperator(const Operator &op, vector<int> pattern);
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
class PDBAbstraction {
    vector<int> pattern;
    size_t size;
    vector<int> distances;
    vector<vector<Edge > > back_edges;
    vector<int> n_i;
    priority_queue<Node, vector<Node>, compare> pq;
    int hash_index(const AbstractState &state) const;
    AbstractState inv_hash_index(int index) const;
public:
    PDBAbstraction(vector<int> pattern);
    void create_pdb();
    void compute_goal_distances();
    int get_heuristic_value(const State &state) const;
    void dump() const;
};

class PDBHeuristic : public Heuristic {
    PDBAbstraction *pdb_abstraction;
    void verify_no_axioms_no_cond_effects() const;
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
