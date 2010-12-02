#ifndef CANONICAL_HEURISTIC_H
#define CANONICAL_HEURISTIC_H

#include "heuristic.h"
#include "pattern_collection.h"
#include <vector>

// Implements the canonical heuristic function.
class CanonicalHeuristic : public Heuristic {
    PatternCollection *pattern_collection;
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    CanonicalHeuristic();
    virtual ~CanonicalHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config, int start, int &end, bool dry_run);
};

#endif
