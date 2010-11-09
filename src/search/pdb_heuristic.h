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
    vector<int> variable_values; // contains entries for all variables, value = -10 for variables not contained in the abstraction/pdb
public:
    AbstractState(vector<int> var_vals);
    AbstractState(const State &state, vector<int> pattern);
    vector<int> get_variable_values() const;
};

class Operator;
typedef pair<int, pair<int, int> > PreEffect;
class AbstractOperator {
    vector<PreEffect> pre_effect;
    string name;
    int cost;
public:
    AbstractOperator(Operator &op, vector<int> pattern);
    bool is_applicable(const AbstractState &abstract_state) const;
    const AbstractState apply_operator(const AbstractState &abstract_state);
    string get_name() const { return name; }
    int get_cost() const {return cost; } 
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
    int get_goal_distance(const AbstractState &abstract_state);
};

class PDBHeuristic : public Heuristic {
    vector<int> pattern;
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
