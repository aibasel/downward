#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"
#include <vector>
using namespace std;

class Edge {
    const Operator *op;
    size_t source;
    size_t target;
};

class AbstractState {
    variable_values *vars;

public:
    AbstractState(State *state, vector<int> pattern);
};

class PDBAbstraction {
    vector<int> pattern;
    size_t size;
    vector<int> distance;
    vector<vector<Edge > > back-edges;

public:
    PDBAbstraction(vector<int> pattern);
    void createPDB();

};

class PDBHeuristic : public Heuristic {

protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);

public:
    PDBHeuristic();
    ~PDBHeuristic();

    int hash_index(const AbstractState &state);
    *AbstracState inv_hash_index(int index);

    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};



#endif
