#ifndef PDBCOLLECTION_HEURISTIC_H
#define PDBCOLLECTION_HEURISTIC_H

#include "heuristic.h"

#include <vector>

// Implements the canonical heuristic function.
class PDBHeuristic;
class PDBCollectionHeuristic : public Heuristic {
    std::vector<std::vector<PDBHeuristic *> > max_cliques; // final computed max_cliques
    std::vector<std::vector<bool> > are_additive; // variables which are additive
    std::vector<PDBHeuristic *> pattern_databases; // final pattern databases
    bool are_pattern_additive(const std::vector<int> &patt1, const std::vector<int> &patt2) const;
    void precompute_max_cliques();
    void precompute_additive_vars();
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBCollectionHeuristic(const std::vector<std::vector<int> > &pattern_collection);
    virtual ~PDBCollectionHeuristic();
    void add_new_pattern(PDBHeuristic *pdb);
    // checks for all max cliques if they would be additive to this pattern
    void get_max_additive_subsets(const std::vector<int> &new_pattern,
                                  std::vector<std::vector<PDBHeuristic *> > &max_additive_subsets);
    void dump(const std::vector<std::vector<int> > &cgraph) const;
};

#endif
