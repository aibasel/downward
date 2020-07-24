#include "merge_selector_score_based_filtering.h"

#include "factored_transition_system.h"
#include "merge_scoring_function.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
MergeSelectorScoreBasedFiltering::MergeSelectorScoreBasedFiltering(
    const options::Options &options)
    : merge_scoring_functions(
          options.get_list<shared_ptr<MergeScoringFunction>>(
              "scoring_functions")) {
}

vector<pair<int, int>> MergeSelectorScoreBasedFiltering::get_remaining_candidates(
    const vector<pair<int, int>> &merge_candidates,
    const vector<double> &scores) const {
    assert(merge_candidates.size() == scores.size());
    double best_score = INF;
    for (double score : scores) {
        if (score < best_score) {
            best_score = score;
        }
    }

    vector<pair<int, int>> result;
    for (size_t i = 0; i < scores.size(); ++i) {
        if (scores[i] == best_score) {
            result.push_back(merge_candidates[i]);
        }
    }
    return result;
}

pair<int, int> MergeSelectorScoreBasedFiltering::select_merge(
    const FactoredTransitionSystem &fts,
    const vector<int> &indices_subset) const {
    vector<pair<int, int>> merge_candidates =
        compute_merge_candidates(fts, indices_subset);

    for (const shared_ptr<MergeScoringFunction> &scoring_function :
         merge_scoring_functions) {
        vector<double> scores = scoring_function->compute_scores(
            fts, merge_candidates);
        merge_candidates = get_remaining_candidates(merge_candidates, scores);
        if (merge_candidates.size() == 1) {
            break;
        }
    }

    if (merge_candidates.size() > 1) {
        cerr << "More than one merge candidate remained after computing all "
            "scores! Did you forget to include a uniquely tie-breaking "
            "scoring function, e.g. total_order or single_random?" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

    return merge_candidates.front();
}

void MergeSelectorScoreBasedFiltering::initialize(const TaskProxy &task_proxy) {
    for (shared_ptr<MergeScoringFunction> &scoring_function
         : merge_scoring_functions) {
        scoring_function->initialize(task_proxy);
    }
}

string MergeSelectorScoreBasedFiltering::name() const {
    return "score based filtering";
}

void MergeSelectorScoreBasedFiltering::dump_specific_options() const {
    for (const shared_ptr<MergeScoringFunction> &scoring_function
         : merge_scoring_functions) {
        scoring_function->dump_options();
    }
}

bool MergeSelectorScoreBasedFiltering::requires_init_distances() const {
    for (const shared_ptr<MergeScoringFunction> &scoring_function
         : merge_scoring_functions) {
        if (scoring_function->requires_init_distances()) {
            return true;
        }
    }
    return false;
}

bool MergeSelectorScoreBasedFiltering::requires_goal_distances() const {
    for (const shared_ptr<MergeScoringFunction> &scoring_function
         : merge_scoring_functions) {
        if (scoring_function->requires_goal_distances()) {
            return true;
        }
    }
    return false;
}

static shared_ptr<MergeSelector>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Score based filtering merge selector",
        "This merge selector has a list of scoring functions, which are used "
        "iteratively to compute scores for merge candidates, keeping the best "
        "ones (with minimal scores) until only one is left.");
    parser.add_list_option<shared_ptr<MergeScoringFunction>>(
        "scoring_functions",
        "The list of scoring functions used to compute scores for candidates.");

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeSelectorScoreBasedFiltering>(opts);
}

static options::Plugin<MergeSelector> _plugin("score_based_filtering", _parse);
}
