#include "merge_scoring_function_goal_relevance.h"

#include "factored_transition_system.h"
#include "transition_system.h"
#include "utils.h"

#include "../plugins/plugin.h"

using namespace std;

namespace merge_and_shrink {
vector<double> MergeScoringFunctionGoalRelevance::compute_scores(
    const FactoredTransitionSystem &fts,
    const vector<pair<int, int>> &merge_candidates) {
    int num_ts = fts.get_size();
    vector<bool> goal_relevant(num_ts, false);
    for (int ts_index : fts) {
        const TransitionSystem &ts = fts.get_transition_system(ts_index);
        if (is_goal_relevant(ts)) {
            goal_relevant[ts_index] = true;
        }
    }

    vector<double> scores;
    scores.reserve(merge_candidates.size());
    for (pair<int, int> merge_candidate : merge_candidates) {
        int ts_index1 = merge_candidate.first;
        int ts_index2 = merge_candidate.second;
        int score = INF;
        if (goal_relevant[ts_index1] || goal_relevant[ts_index2]) {
            score = 0;
        }
        scores.push_back(score);
    }
    return scores;
}

string MergeScoringFunctionGoalRelevance::name() const {
    return "goal relevance";
}

class MergeScoringFunctionGoalRelevanceFeature : public plugins::TypedFeature<MergeScoringFunction, MergeScoringFunctionGoalRelevance> {
public:
    MergeScoringFunctionGoalRelevanceFeature() : TypedFeature("goal_relevance") {
        document_title("Goal relevance scoring");
        document_synopsis(
            "This scoring function assigns a merge candidate a value of 0 iff at "
            "least one of the two transition systems of the merge candidate is "
            "goal relevant in the sense that there is an abstract non-goal state. "
            "All other candidates get a score of positive infinity.");
    }

    virtual shared_ptr<MergeScoringFunctionGoalRelevance> create_component(const plugins::Options &, const utils::Context &) const override {
        return make_shared<MergeScoringFunctionGoalRelevance>();
    }
};

static plugins::FeaturePlugin<MergeScoringFunctionGoalRelevanceFeature> _plugin;
}
