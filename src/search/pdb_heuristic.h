#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"
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
    map<int, int> variale_to_index_mapping;
    map<int, int> index_to_variable_mapping;
    AbstractState(vector<int> var_vals, vector<int> pattern);
    AbstractState(const State &state, vector<int> pattern);
    vector<int> get_variable_values() const;
    void dump() const;
};

class Operator;
typedef pair<int, pair<int, int> > PreEffect;
class AbstractOperator {
    vector<int> pattern; // only use: in order to construct new AbstractState, which unfortunately needs a pattern for the moment
    vector<PreEffect> pre_effect;
public:
    AbstractOperator(Operator &op, vector<int> pattern);
    bool is_applicable(const AbstractState &abstract_state) const;
    const AbstractState apply_operator(const AbstractState &abstract_state);
    void dump() const;
};

class PDBAbstraction {
    vector<int> pattern;
    size_t size;
    vector<int> distances;
    vector<vector<Edge > > back_edges;
    vector<int> n_i;
    int hash_index(const AbstractState &state);
    const AbstractState *inv_hash_index(int index);
public:
    PDBAbstraction(vector<int> pattern);
    void create_pdb();
    void compute_goal_distances();
    int get_heuristic_value(const State &state);
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
