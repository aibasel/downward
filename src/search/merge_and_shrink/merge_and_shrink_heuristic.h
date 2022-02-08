#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include "../utils/logging.h"

#include <memory>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeAndShrinkRepresentation;

class MergeAndShrinkHeuristic : public Heuristic {
    utils::LogProxy log;

    // The final merge-and-shrink representations, storing goal distances.
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;

    void extract_factor(FactoredTransitionSystem &fts, int index);
    bool extract_unsolvable_factor(FactoredTransitionSystem &fts);
    void extract_nontrivial_factors(FactoredTransitionSystem &fts);
    void extract_factors(FactoredTransitionSystem &fts);
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);
};
}

#endif
