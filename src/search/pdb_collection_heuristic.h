#ifndef PDBCOLLECTION_HEURISTIC_H
#define PDBCOLLECTION_HEURISTIC_H

#include "heuristic.h"
#include "pdb_heuristic.h"
#include <vector>
#include <string>

// Implements the canonical heuristic function.
class PDBCollectionHeuristic : public Heuristic {
    std::vector<std::vector<int> > pattern_collection;
    std::vector<std::vector<int> > max_cliques; // final computed max_cliques
    std::vector<std::vector<bool> > are_additive; // variables which are additive
    std::vector<PDBHeuristic *> pattern_databases; // final pattern databases
    bool are_pattern_additive(int pattern1, int pattern2) const;
    void precompute_max_cliques();
    void precompute_additive_vars();
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBCollectionHeuristic();
    virtual ~PDBCollectionHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config, int start, int &end, bool dry_run);
    void dump(const std::vector<std::vector<int> > &cgraph) const;
};

#endif
