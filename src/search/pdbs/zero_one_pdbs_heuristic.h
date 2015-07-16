#ifndef PDBS_ZERO_ONE_PDBS_HEURISTIC_H
#define PDBS_ZERO_ONE_PDBS_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

class PatternDatabase;

class ZeroOnePDBsHeuristic : public Heuristic {
    /* Summed up mean finite h-values of all PDBs.
       This is an approximation only, see get-method. */
    double approx_mean_finite_h;
    // Final pattern databases.
    std::vector<PatternDatabase *> pattern_databases;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);
public:
    ZeroOnePDBsHeuristic(const Options &opts,
                         const std::vector<int> &op_costs = std::vector<int>());
    virtual ~ZeroOnePDBsHeuristic();
    /*
      Returns the sum of all mean finite h-values of every PDB.
      This is an approximation of the real mean finite h-value of the Heuristic,
      because dead-ends are ignored for the computation of the mean finite
      h-values for a PDB. As a consequence, if different PDBs have different
      states which are dead-end, we do not calculate the real mean h-value for
      these states.
    */
    double get_approx_mean_finite_h() const {return approx_mean_finite_h; }
    void dump() const;
};

#endif
