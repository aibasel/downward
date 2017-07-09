#ifndef MERGE_AND_SHRINK_MERGE_SELECTOR_SCORE_BASED_FILTERING_H
#define MERGE_AND_SHRINK_MERGE_SELECTOR_SCORE_BASED_FILTERING_H

#include "merge_selector.h"

#include "merge_scoring_function.h"

#include <memory>
#include <vector>

namespace options {
class Options;
}

namespace merge_and_shrink {
class MergeSelectorScoreBasedFiltering : public MergeSelector {
    std::vector<std::shared_ptr<MergeScoringFunction>> merge_scoring_functions;

    std::vector<std::pair<int, int>> get_remaining_candidates(
        const std::vector<std::pair<int, int>> &merge_candidates,
        const std::vector<double> &scores) const;
protected:
    virtual std::string name() const override;
    virtual void dump_specific_options() const override;
public:
    explicit MergeSelectorScoreBasedFiltering(const options::Options &options);
    virtual ~MergeSelectorScoreBasedFiltering() override = default;
    // TODO: get rid of this extra constructor
    explicit MergeSelectorScoreBasedFiltering(
        std::vector<std::shared_ptr<MergeScoringFunction>> scoring_functions);
    virtual std::pair<int, int> select_merge(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices_subset = std::vector<int>()) const override;
    virtual void initialize(const TaskProxy &task_proxy) override;
    virtual bool requires_init_distances() const override;
    virtual bool requires_goal_distances() const override;
};
}

#endif
