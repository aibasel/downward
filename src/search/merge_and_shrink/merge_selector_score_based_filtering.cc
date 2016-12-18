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

MergeSelectorScoreBasedFiltering::MergeSelectorScoreBasedFiltering(
    vector<shared_ptr<MergeScoringFunction>> scoring_functions)
    : merge_scoring_functions(move(scoring_functions)) {
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
    FactoredTransitionSystem &fts,
    const vector<int> &indices_subset) const {
    vector<pair<int, int>> merge_candidates =
        compute_merge_candidates(fts, indices_subset);

    for (size_t i = 0; i < merge_scoring_functions.size(); ++i) {
        const shared_ptr<MergeScoringFunction> &scoring_function =
            merge_scoring_functions[i];
        vector<double> scores = scoring_function->compute_scores(
            fts, merge_candidates);
        int old_size = merge_candidates.size();
        merge_candidates = get_remaining_candidates(merge_candidates, scores);
        int new_size = merge_candidates.size();
        if (new_size - old_size == 0 &&
            i != merge_scoring_functions.size() - 1 &&
            scoring_function->get_name() == "goal relevance" &&
            merge_scoring_functions[i + 1]->get_name() == "dfp" &&
            scores.front() == INF) {
            /*
              "skip" computation of dfp if no goal relevance pair has been
              found (which is the case if no candidates have been filtered,
              because all scores are identical, and the scores must be
              infinity) to mimic previous MergeDFP behavior.
            */
            ++i;
            continue;
        }
        if (merge_candidates.size() == 1) {
            break;
        }
    }

    if (merge_candidates.size() > 1) {
        cerr << "More than one merge candidate remained after computing all "
            "scores! Did you forget to include a uniquely tie-breaking "
            "scoring function, e.g. total_order or single_random?" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
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

static options::PluginShared<MergeSelector> _plugin("score_based_filtering", _parse);
}
