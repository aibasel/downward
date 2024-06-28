#ifndef PDBS_PDB_HEURISTIC_H
#define PDBS_PDB_HEURISTIC_H

#include "pattern_generator.h"

#include "../heuristic.h"

namespace pdbs {
class PatternDatabase;

// Implements a heuristic for a single PDB.
class PDBHeuristic : public Heuristic {
    std::shared_ptr<PatternDatabase> pdb;
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    /*
      Important: It is assumed that the pattern (passed via
      pattern_generator) is sorted, contains no duplicates and is small
      enough so that the number of abstract states is below
      numeric_limits<int>::max()
      Parameters:
       operator_costs: Can specify individual operator costs for each
       operator. This is useful for action cost partitioning. If left
       empty, default operator costs are used.
    */
    PDBHeuristic(
        const std::shared_ptr<PatternGenerator> &pattern_generator,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
};
}

#endif
