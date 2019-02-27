#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeAndShrinkRepresentation;
enum class Verbosity;

class MergeAndShrinkHeuristic : public Heuristic {
    Verbosity verbosity;

    // The final merge-and-shrink representations, storing goal distances.
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;

    void finalize_factor(FactoredTransitionSystem &fts, int index);
    void finalize(FactoredTransitionSystem &fts);
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);
};
}

#endif
