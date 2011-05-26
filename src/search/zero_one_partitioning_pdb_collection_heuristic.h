#ifndef ZERO_ONE_PARTITIONING_PDB_COLLECTION_HEURISTIC_H
#define ZERO_ONE_PARTITIONING_PDB_COLLECTION_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class PDBHeuristic;
class ZeroOnePartitioningPdbCollectionHeuristic : public Heuristic {
    double fitness; // summed up mean h-values of all PDBs
    std::vector<PDBHeuristic *> pattern_databases; // final pattern databases
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    ZeroOnePartitioningPdbCollectionHeuristic(const Options &opts);
    virtual ~ZeroOnePartitioningPdbCollectionHeuristic();
    double get_fitness() const { return fitness; }
    void dump() const;
};

#endif
