#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"
#include "pdb_abstraction.h"

#include <vector>

class PDBHeuristic : public Heuristic {
    PDBAbstraction *pdb_abstraction;
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
