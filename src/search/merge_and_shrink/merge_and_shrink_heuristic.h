#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace merge_and_shrink {
class MergeAndShrinkRepresentation;

class MergeAndShrinkHeuristic : public Heuristic {
    // The final merge-and-shrink representation, storing goal distances.
    std::unique_ptr<MergeAndShrinkRepresentation> mas_representation;
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);
};
}

#endif
