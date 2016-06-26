#include "merge_scoring_function_goal_relevance.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../options/option_parser.h"
#include "../options/plugin.h"

using namespace std;

namespace merge_and_shrink {
bool is_goal_relevant(const TransitionSystem &ts) {
    int num_states = ts.get_size();
    for (int state = 0; state < num_states; ++state) {
        if (!ts.is_goal_state(state)) {
            return true;
        }
    }
    return false;
}

vector<int> MergeScoringFunctionGoalRelevance::compute_scores(
    FactoredTransitionSystem &fts,
    const vector<pair<int, int>> &merge_candidates) {
    int num_ts = fts.get_size();
    vector<bool> goal_relevant(num_ts, false);
    for (int ts_index = 0; ts_index < num_ts; ++ts_index) {
        if (fts.is_active(ts_index)) {
            const TransitionSystem &ts = fts.get_ts(ts_index);
            if (is_goal_relevant(ts)) {
                goal_relevant[ts_index] = true;
            }
        }
    }

    vector<int> scores;
    scores.reserve(merge_candidates.size());
    for (pair<int, int> merge_candidate : merge_candidates ) {
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

static shared_ptr<MergeScoringFunction>_parse(options::OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeScoringFunctionGoalRelevance>();
}

static options::PluginShared<MergeScoringFunction> _plugin("goal_relevance", _parse);
}
