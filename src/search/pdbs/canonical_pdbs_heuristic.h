#ifndef PDBS_CANONICAL_PDBS_HEURISTIC_H
#define PDBS_CANONICAL_PDBS_HEURISTIC_H

#include "canonical_pdbs.h"
#include "pattern_generator.h"

#include "../heuristic.h"

#include <memory>

namespace plugins {
class Feature;
}

namespace pdbs {
// Implements the canonical heuristic function.
class CanonicalPDBsHeuristic : public Heuristic {
    CanonicalPDBs canonical_pdbs;

protected:
    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    CanonicalPDBsHeuristic(
        const std::shared_ptr<PatternCollectionGenerator> &pattern_generator,
        double max_time_dominance_pruning,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates,
        const std::string &description,
        utils::Verbosity verbosity);
};

void add_canonical_pdbs_options_to_feature(plugins::Feature &feature);
}

#endif
