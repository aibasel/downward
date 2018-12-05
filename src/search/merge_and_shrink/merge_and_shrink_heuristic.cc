#include "merge_and_shrink_heuristic.h"

#include "distances.h"
#include "factor_scoring_functions.h"
#include "factored_transition_system.h"
#include "merge_and_shrink_algorithm.h"
#include "merge_and_shrink_representation.h"
#include "transition_system.h"
#include "types.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

#include "../utils/markup.h"
#include "../utils/system.h"

#include <cassert>
#include <iostream>
#include <utility>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const options::Options &opts)
    : Heuristic(opts),
      verbosity(static_cast<Verbosity>(opts.get_enum("verbosity"))),
      partial_mas_method(static_cast<PartialMASMethod>(opts.get_enum("partial_mas_method"))) {
    if (opts.contains("factor_scoring_functions")) {
        factor_scoring_functions = opts.get_list<shared_ptr<FactorScoringFunction>>(
            "factor_scoring_functions");
        if (factor_scoring_functions.empty()) {
            cerr << "Got empty list of factor scoring functions." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }

    cout << "Initializing merge-and-shrink heuristic..." << endl;
    MergeAndShrinkAlgorithm algorithm(opts);
    FactoredTransitionSystem fts = algorithm.build_factored_transition_system(task_proxy);
    finalize(fts);
    cout << "Done initializing merge-and-shrink heuristic." << endl << endl;
}

vector<int> get_remaining_candidates(
    const vector<int> &merge_candidates,
    const vector<int> &scores) {
    assert(merge_candidates.size() == scores.size());
    int best_score = -1;
    for (int score : scores) {
        if (score > best_score) {
            best_score = score;
        }
    }

    vector<int> result;
    for (size_t i = 0; i < scores.size(); ++i) {
        if (scores[i] == best_score) {
            result.push_back(merge_candidates[i]);
        }
    }
    return result;
}

int MergeAndShrinkHeuristic::find_best_factor(
    const FactoredTransitionSystem &fts) const {
    vector<int> current_indices;
    for (int index : fts) {
        current_indices.push_back(index);
    }

    for (const shared_ptr<FactorScoringFunction> &fsf : factor_scoring_functions) {
        vector<int> scores = fsf->compute_scores(fts, current_indices);
        current_indices = get_remaining_candidates(current_indices, scores);
        if (current_indices.size() == 1) {
            break;
        }
    }

    if (current_indices.size() > 1) {
        cerr << "More than one factor candidate remained after computing all "
            "scores! Did you forget to include a uniquely tie-breaking "
            "factor scoring function such as fsf_random?" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    return current_indices.front();
}

void MergeAndShrinkHeuristic::finalize_factor(
    FactoredTransitionSystem &fts, int index) {
    pair<unique_ptr<MergeAndShrinkRepresentation>, unique_ptr<Distances>>
    final_entry = fts.extract_factor(index);
    if (!final_entry.second->are_goal_distances_computed()) {
        const bool compute_init = false;
        const bool compute_goal = true;
        final_entry.second->compute_distances(
            compute_init, compute_goal, verbosity);
    }
    assert(final_entry.second->are_goal_distances_computed());
    final_entry.first->set_distances(*final_entry.second);
    mas_representations.push_back(move(final_entry.first));
}

void MergeAndShrinkHeuristic::finalize(FactoredTransitionSystem &fts) {
    /*
      TODO: This method has quite a bit of fiddling with aspects of
      transition systems and the merge-and-shrink representation (checking
      whether distances have been computed; computing them) that we would
      like to have at a lower level. See also the TODO in
      factored_transition_system.h on improving the interface of that class
      (and also related classes like TransitionSystem etc).
    */

    int active_factors_count = fts.get_num_active_entries();
    if (verbosity >= Verbosity::NORMAL) {
        cout << "Number of remaining factors: " << active_factors_count << endl;
    }

    for (int index = 0; index < fts.get_size(); ++index) {
        if (fts.is_active(index) && !fts.is_factor_solvable(index)) {
            finalize_factor(fts, index);
            cout << fts.get_transition_system(index).tag()
                 << "use this unsolvable factor as heuristic."
                 << endl;
            return;
        }
    }

    if (active_factors_count == 1) {
        /*
          We regularly finished the merge-and-shrink construction, i.e., we
          merged all transition systems and are left with one solvable
          transition system. This assumes that merges are always appended at
          the end.
        */
        int last_factor_index = fts.get_size() - 1;
        for (int index = 0; index < last_factor_index; ++index) {
            assert(!fts.is_active(index));
        }
        finalize_factor(fts, last_factor_index);
        cout << fts.get_transition_system(last_factor_index).tag()
             << "use this single remaining factor as heuristic."
             << endl;
    } else {
        assert(partial_mas_method != PartialMASMethod::None);
        if (partial_mas_method == PartialMASMethod::Single) {
            if (verbosity >= Verbosity::NORMAL) {
                cout << "Need to choose a single factor to serve as a heuristic."
                     << endl;
            }
            int index = find_best_factor(fts);
            // We do not need the scoring functions anymore at this point.
            factor_scoring_functions.clear();
            finalize_factor(fts, index);
            if (verbosity >= Verbosity::NORMAL) {
                cout << fts.get_transition_system(index).tag()
                     << "chose this factor as heuristic."
                     << endl;
            }
        } else if (partial_mas_method == PartialMASMethod::Maximum) {
            for (int index : fts) {
                finalize_factor(fts, index);
            }
            if (verbosity >= Verbosity::NORMAL) {
                cout << "Use all factors in a maximum heuristic." << endl;
            }
        } else {
            cerr << "Unknown partial merge-and-shrink method!" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
        }
    }
}

int MergeAndShrinkHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    int heuristic = 0;
    for (const unique_ptr<MergeAndShrinkRepresentation> &mas_representation : mas_representations) {
        int cost = mas_representation->get_value(state);
        if (cost == PRUNED_STATE || cost == INF) {
            // If state is unreachable or irrelevant, we encountered a dead end.
            return DEAD_END;
        }
        heuristic = max(heuristic, cost);
    }
    return heuristic;
}

static shared_ptr<Heuristic> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Merge-and-shrink heuristic",
        "This heuristic implements the algorithm described in the following "
        "paper:" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "Generalized Label Reduction for Merge-and-Shrink Heuristics",
            "https://ai.dmi.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014") + "\n" +
        "For a more exhaustive description of merge-and-shrink, see the journal "
        "paper" + utils::format_paper_reference(
            {"Malte Helmert", "Patrik Haslum", "Joerg Hoffmann", "Raz Nissim"},
            "Merge-and-Shrink Abstraction: A Method for Generating Lower Bounds"
            " in Factored State Spaces",
            "https://ai.dmi.unibas.ch/papers/helmert-et-al-jacm2014.pdf",
            "Journal of the ACM 61 (3)",
            "16:1-63",
            "2014") + "\n" +
        "Please note that the journal paper describes the \"old\" theory of "
        "label reduction, which has been superseded by the above conference "
        "paper and is no longer implemented in Fast Downward.\n\n"
        "The following paper describes how to improve the DFP merge strategy "
        "with tie-breaking, and presents two new merge strategies (dyn-MIASM "
        "and SCC-DFP):" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
            "https://ai.dmi.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
            "Proceedings of the 26th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2016)",
            "294-298",
            "AAAI Press 2016"));
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported (but see note)");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes (but see note)");
    parser.document_property("consistent", "yes (but see note)");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note(
        "Note",
        "Conditional effects are supported directly. Note, however, that "
        "for tasks that are not factored (in the sense of the JACM 2014 "
        "merge-and-shrink paper), the atomic transition systems on which "
        "merge-and-shrink heuristics are based are nondeterministic, "
        "which can lead to poor heuristics even when only perfect shrinking "
        "is performed.");
    parser.document_note(
        "Note",
        "When pruning unreachable states, admissibility and consistency is "
        "only guaranteed for reachable states and transitions between "
        "reachable states. While this does not impact regular A* search which "
        "will never encounter any unreachable state, it impacts techniques "
        "like symmetry-based pruning: a reachable state which is mapped to an "
        "unreachable symmetric state (which hence is pruned) would falsely be "
        "considered a dead-end and also be pruned, thus violating optimality "
        "of the search.");
    parser.document_note(
        "Note",
        "A currently recommended good configuration uses bisimulation "
        "based shrinking, the merge strategy SCC-DFP, and the appropriate "
        "label reduction setting (max_states has been altered to be between "
        "10000 and 200000 in the literature):\n"
        "{{{\nmerge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),"
        "merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector="
        "score_based_filtering(scoring_functions=[goal_relevance,dfp,"
        "total_order])),label_reduction=exact(before_shrinking=true,"
        "before_merging=false),max_states=50000,threshold_before_merge=1)\n}}}\n"
        "Note that for versions of Fast Downward prior to 2016-08-19, the "
        "syntax differs. See the recommendation in the file "
        "merge_and_shrink_heuristic.cc for an example configuration.");

    Heuristic::add_options_to_parser(parser);
    add_merge_and_shrink_algorithm_options_to_parser(parser);

    vector<string> partial_mas_method;
        vector<string> partial_mas_method_docs;
        partial_mas_method.push_back("none");
        partial_mas_method_docs.push_back(
            "none: use an algorithm configuration that is guaranteed to return "
            "a factored transition system with a single factor. Hence do not "
            "use the option main_loop_max_time");
        partial_mas_method.push_back("single");
        partial_mas_method_docs.push_back(
            "single: choose a single factor of the remaining factors to serve as"
            "the abstraction for the heuristic. The factor is chosen according to "
            "the factor scoring functions provided via factor_scoring_functions.");
        partial_mas_method.push_back("maximum");
        partial_mas_method_docs.push_back(
            "maximum: retain all remaining factors and compute the maximum "
            "heuristic over all these abstractions.");
        parser.add_enum_option(
            "partial_mas_method",
            partial_mas_method,
            "Method to determine the final heuristic given an early abortion "
            "due to reaching the time limit.",
            "none",
            partial_mas_method_docs);
        parser.add_list_option<shared_ptr<FactorScoringFunction>>(
            "factor_scoring_functions",
            "The list of factor scoring functions used to compute scores for "
            "remaining factors if computing partial merge-and-shrink abstractions, "
            "i.e., if partial_mas_method != none.",
            options::OptionParser::NONE);

    options::Options opts = parser.parse();
    if (parser.help_mode()) {
        return nullptr;
    }

    handle_shrink_limit_options_defaults(opts);

    if (parser.dry_run()) {
        double main_loop_max_time = opts.get<double>("main_loop_max_time");
        PartialMASMethod partial_mas_method = static_cast<PartialMASMethod>(opts.get_enum("partial_mas_method"));
        if (partial_mas_method != PartialMASMethod::None
            && main_loop_max_time == numeric_limits<double>::infinity()) {
            cerr << "If setting partial_mas_method != none, you must "
                "use a finite value for main_loop_max_time."
                 << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
        if (partial_mas_method == PartialMASMethod::None
            && main_loop_max_time < INF) {
            cerr << "If using a finite value for main_loop_max_time, you must "
                "also set partial_mas_method != none."
                 << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
        if (partial_mas_method == PartialMASMethod::Single
            && !opts.contains("factor_scoring_functions")) {
            cerr << "If using partial_mas_method=single, you must also specify "
                "a least one factor scoring function via factor_scoring_functions."
                 << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
        return nullptr;
    } else {
        return make_shared<MergeAndShrinkHeuristic>(opts);
    }
}

static options::Plugin<Evaluator> _plugin("merge_and_shrink", _parse);
}
