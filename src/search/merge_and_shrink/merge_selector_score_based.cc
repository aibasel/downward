#include "merge_selector_score_based.h"

#include "factored_transition_system.h"
#include "merge_scoring_function.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include <cassert>


// for the alias merge_dfp
#include "merge_scoring_function_dfp.h"
#include "merge_scoring_function_goal_relevance.h"
#include "merge_scoring_function_tiebreaking_dfp.h"
#include "merge_strategy_factory_stateless.h"

#include "../utils/markup.h"


using namespace std;

namespace merge_and_shrink {
MergeSelectorScoreBased::MergeSelectorScoreBased(const options::Options &options)
    : merge_scoring_functions(
        options.get_list<shared_ptr<MergeScoringFunction>>(
            "scoring_functions")) {
}

MergeSelectorScoreBased::MergeSelectorScoreBased(
    vector<shared_ptr<MergeScoringFunction>> scoring_functions)
    : merge_scoring_functions(move(scoring_functions)) {
}

vector<pair<int, int>> MergeSelectorScoreBased::get_remaining_candidates(
    const vector<pair<int, int>> &merge_candidates,
    const vector<int> &scores) const {
    assert(merge_candidates.size() == scores.size());
    int best_score = INF;
    for (int score : scores) {
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

pair<int, int> MergeSelectorScoreBased::select_merge(
    FactoredTransitionSystem &fts) {
    vector<pair<int, int>> merge_candidates;
    for (int ts_index1 = 0; ts_index1 < fts.get_size(); ++ts_index1) {
        if (fts.is_active(ts_index1)) {
            for (int ts_index2 = ts_index1 + 1; ts_index2 < fts.get_size();
                 ++ts_index2) {
                if (fts.is_active(ts_index2)) {
                    merge_candidates.emplace_back(ts_index1, ts_index2);
                }
            }
        }
    }

    for (shared_ptr<MergeScoringFunction> &scoring_function :
         merge_scoring_functions) {
        vector<int> scores = scoring_function->compute_scores(fts,
                                                              merge_candidates);
        merge_candidates = get_remaining_candidates(merge_candidates, scores);
        if (merge_candidates.size() == 1) {
            break;
        }
    }

    if (merge_candidates.size() > 1) {
        cerr << "More than one merge candidate remained after computing all "
                "scores! Did you forget to include a randomizing scoring "
                "function?" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }

    return merge_candidates.front();
}

void MergeSelectorScoreBased::initialize(shared_ptr<AbstractTask> task) {
    for (shared_ptr<MergeScoringFunction> &scoring_function
         : merge_scoring_functions) {
        scoring_function->initialize(task);
    }
}

void MergeSelectorScoreBased::dump_specific_options() const {
    for (const shared_ptr<MergeScoringFunction> &scoring_function
         : merge_scoring_functions) {
        scoring_function->dump_options();
    }
}

static shared_ptr<MergeSelector>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Score based merge selector",
        "This merge selector has a list of scoring functions, which are used "
        "iteratively to compute scores for merge candidates until one best "
        "candidate has been determined.");
    parser.add_list_option<shared_ptr<MergeScoringFunction>>(
        "scoring_functions",
        "The list of scoring functions used to compute scores for candidates.");

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeSelectorScoreBased>(opts);
}

static options::PluginShared<MergeSelector> _plugin("score_based", _parse);

// TODO: this MergeDFP compatibilty parsing does *not* support the old option
// to randomize the entire transition system order (this requires a different
// merge scoring function to be used.)
static shared_ptr<MergeStrategyFactory>_parse_dfp(options::OptionParser &parser) {
    parser.document_synopsis(
        "Merge strategy DFP",
        "This merge strategy implements the algorithm originally described in the "
        "paper \"Directed model checking with distance-preserving abstractions\" "
        "by Draeger, Finkbeiner and Podelski (SPIN 2006), adapted to planning in "
        "the following paper:" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "Generalized Label Reduction for Merge-and-Shrink Heuristics",
            "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014"));
    MergeScoringFunctionTiebreakingDFP::add_options_to_parser(parser);
    if (parser.dry_run())
        return nullptr;

    options::Options options = parser.parse();
    shared_ptr<MergeScoringFunction> scoring_tiebreaking_dfp =
        make_shared<MergeScoringFunctionTiebreakingDFP>(options);

    vector<shared_ptr<MergeScoringFunction>> scoring_functions;
    scoring_functions.push_back(make_shared<MergeScoringFunctionGoalRelevance>());
    scoring_functions.push_back(make_shared<MergeScoringFunctionDFP>());
    scoring_functions.push_back(scoring_tiebreaking_dfp);

    // TODO: the option parser does not handle this
//    options::Options selector_options;
//    selector_options.set<vector<shared_ptr<MergeScoringFunction>>(
//        "scoring_functions",
//        scoring_functions);
    shared_ptr<MergeSelector> selector =
        make_shared<MergeSelectorScoreBased>(move(scoring_functions));

    options::Options strategy_options;
    strategy_options.set<shared_ptr<MergeSelector>>(
        "merge_selector",
        selector);

    return make_shared<MergeStrategyFactoryStateless>(strategy_options);
}

static options::PluginShared<MergeStrategyFactory> _plugin_dfp("merge_dfp", _parse_dfp);
}
