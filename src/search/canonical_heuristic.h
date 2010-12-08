#ifndef CANONICAL_HEURISTIC_H
#define CANONICAL_HEURISTIC_H

#include "heuristic.h"
#include "pattern_collection.h"
#include <vector>

// Implements the canonical heuristic function.
class PDBCollectionHeuristic : public Heuristic {
    PatternCollection *pattern_collection;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBCollectionHeuristic();
    virtual ~PDBCollectionHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config, int start, int &end, bool dry_run);
};

#endif
