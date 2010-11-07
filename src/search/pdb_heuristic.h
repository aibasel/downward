#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"
using namespace std;

class Edge {
    const Operator *op;
    size_t source;
    size_t target;
};

class AbstractState {
    vector<int> variable_values;
public:
    AbstractState(vector<int> values);
};

class PDBHeuristic;
class PDBAbstraction {
    vector<int> pattern;
    size_t size;
    vector<int> distance;
    vector<vector<Edge > > back_edges;
public:
    PDBAbstraction(vector<int> pattern);
    void createPDB(PDBHeuristic *pdbheuristic);

};

class PDBHeuristic : public Heuristic {
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic();
    ~PDBHeuristic();

    int hash_index(const AbstractState &state);
    AbstractState* inv_hash_index(int index);

    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
