#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"
using namespace std;

class Edge {
    const Operator *op;
    size_t target;
public:
    Edge(const Operator *o, size_t t) : op(o), target(t) {}
};

class AbstractState {
    vector<int> variable_values;
public:
    AbstractState(vector<int> values);
    AbstractState(const State &state);
};

class Operator;
class AbstractOperator {
    vector<int> pattern;
public:
    AbstractOperator(Operator op, vector<int> pattern);
    bool is_applicable(AbstractState &abstract_state);
    const AbstractState apply_operator(AbstractState &abstract_state);
};

class PDBAbstraction {
    vector<int> pattern;
    size_t size;
    vector<int> distances;
    vector<vector<Edge > > back_edges;
    int hash_index(const AbstractState &state);
    AbstractState *inv_hash_index(int index);
public:
    PDBAbstraction(vector<int> pattern);
    void create_pdb();
    int get_goal_distance(const AbstractState &abstract_state);
};

class PDBHeuristic : public Heuristic {
    PDBAbstraction *pdb_abstraction;
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
