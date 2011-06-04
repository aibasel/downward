#ifndef ZERO_ONE_PDBS_HEURISTIC_H
#define ZERO_ONE_PDBS_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class PDBHeuristic;
class ZeroOnePDBsHeuristic : public Heuristic {
    double approx_mean_finite_h; // summed up mean h-values of all PDBs
    std::vector<PDBHeuristic *> pattern_databases; // final pattern databases
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    ZeroOnePDBsHeuristic(const Options &opts,
                                              const std::vector<int> &op_costs=std::vector<int>());
    virtual ~ZeroOnePDBsHeuristic();
    /* Returns the sum of all mean finite h-values of every PDB. As in the mean finite h-values
       of PDBs, dead ends are ignored, this sum is only an approximation of the real mean
       h-value of the collection (in case where there are dead ends). */
    double get_approx_mean_finite_h() const { return approx_mean_finite_h; }
    void dump() const;
};

#endif
