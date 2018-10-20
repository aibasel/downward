#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class FactorScoringFunction;
class MergeAndShrinkRepresentation;
enum class Verbosity;

enum class PartialMASMethod {
    None,
    Single,
    Maximum
};

class MergeAndShrinkHeuristic : public Heuristic {
    Verbosity verbosity;

    // Options related to computing partial abstractions
    const PartialMASMethod partial_mas_method;
    std::vector<std::shared_ptr<FactorScoringFunction>> factor_scoring_functions;

    // The final merge-and-shrink representations, storing goal distances.
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;

    int find_best_factor(const FactoredTransitionSystem &fts) const;
    void finalize_factor(FactoredTransitionSystem &fts, int index);
    void finalize(FactoredTransitionSystem &fts);
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);
};
}

#endif
