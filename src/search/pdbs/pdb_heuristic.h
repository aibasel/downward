#ifndef PDBS_PDB_HEURISTIC_H
#define PDBS_PDB_HEURISTIC_H

#include "pattern_database.h"

#include "../heuristic.h"

class GlobalState;
class OperatorProxy;

// Implements a heuristic for a single PDB.
class PDBHeuristic : public Heuristic {
    PatternDatabase pdb;
protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    /*
      Important: It is assumed that the pattern (passed via Options) is
      sorted, contains no duplicates and is small enough so that the
      number of abstract states is below numeric_limits<int>::max()
      Parameters:
       operator_costs: Can specify individual operator costs for each
       operator. This is useful for action cost partitioning. If left
       empty, default operator costs are used.
    */
    PDBHeuristic(const Options &opts,
                 const std::vector<int> &operator_costs = std::vector<int>());
    virtual ~PDBHeuristic() override;
};

#endif
