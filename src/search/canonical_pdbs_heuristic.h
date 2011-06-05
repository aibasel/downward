#ifndef CANONICAL_PDBS_HEURISTIC_H
#define CANONICAL_PDBS_HEURISTIC_H

#include "heuristic.h"

#include <vector>

// Implements the canonical heuristic function.
class PDBHeuristic;
class CanonicalPDBsHeuristic : public Heuristic {
    int size; // the sum of all abstract state sizes of all pdbs in the collection
    std::vector<std::vector<PDBHeuristic *> > max_cliques; // final computed max_cliques
    std::vector<std::vector<bool> > are_additive; // variables which are additive
    std::vector<PDBHeuristic *> pattern_databases; // final pattern databases
    bool are_patterns_additive(const std::vector<int> &patt1, 
                               const std::vector<int> &patt2) const;
    void compute_max_cliques();
    void compute_additive_vars();

    // does not recompute max_cliques
    void _add_pattern(const std::vector<int> &pattern);

    void dump_cgraph(const std::vector<std::vector<int> > &cgraph) const;
    void dump_cliques() const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    CanonicalPDBsHeuristic(const Options &opts);
    virtual ~CanonicalPDBsHeuristic();
    void add_pattern(const std::vector<int> &pattern);

    // checks for all max cliques if they would be additive to this pattern
    void get_max_additive_subsets(const std::vector<int> &new_pattern,
                                  std::vector<std::vector<PDBHeuristic *> > &max_additive_subsets);
    const std::vector<PDBHeuristic *> &get_pattern_databases() const { return pattern_databases; }
    int get_size() const { return size; }
    void dump() const;
};

#endif
