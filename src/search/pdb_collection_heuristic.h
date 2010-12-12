#ifndef PDBCOLLECTION_HEURISTIC_H
#define PDBCOLLECTION_HEURISTIC_H

#include "heuristic.h"
#include "pdb_heuristic.h"
#include <vector>
#include <string>

// Implements the canonical heuristic function.
class PDBCollectionHeuristic : public Heuristic {
    std::vector<std::vector<int> > pattern_collection;
    void precompute_additive_vars();
    void compute_max_cliques();
    std::vector<std::vector<int> > cgraph; // compatibility graph for the pattern collection
    std::vector<std::vector<int> > max_cliques; // final computed max_cliques
    std::vector<std::vector<bool> > are_additive; // variables which are additive
    bool are_pattern_additive(int pattern1, int pattern2) const;
    void build_cgraph();
    int get_maxi_vertex(const std::vector<int> &subg, const std::vector<int> &cand) const; // TODO find nicer name :)
    void max_cliques_expand(std::vector<int> &subg, std::vector<int> &cand, std::vector<int> &q_clique); // implements the CLIQUES-algorithmn from Tomita et al
    std::vector<PDBHeuristic> pattern_databases; // final pattern databases
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBCollectionHeuristic();
    virtual ~PDBCollectionHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config, int start, int &end, bool dry_run);
    void dump() const;
};

#endif
