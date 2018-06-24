#include "merge_and_shrink_heuristic.h"

#include "distances.h"
#include "factor_scoring_functions.h"
#include "factored_transition_system.h"
#include "fts_factory.h"
#include "label_reduction.h"
#include "labels.h"
#include "merge_and_shrink_representation.h"
#include "merge_strategy.h"
#include "merge_strategy_factory.h"
#include "shrink_strategy.h"
#include "transition_system.h"
#include "types.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/markup.h"
#include "../utils/math.h"
#include "../utils/system.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : Heuristic(opts),
      merge_strategy_factory(opts.get<shared_ptr<MergeStrategyFactory>>("merge_strategy")),
      shrink_strategy(opts.get<shared_ptr<ShrinkStrategy>>("shrink_strategy")),
      label_reduction(nullptr),
      max_states(opts.get<int>("max_states")),
      max_states_before_merge(opts.get<int>("max_states_before_merge")),
      shrink_threshold_before_merge(opts.get<int>("threshold_before_merge")),
      prune_unreachable_states(opts.get<bool>("prune_unreachable_states")),
      prune_irrelevant_states(opts.get<bool>("prune_irrelevant_states")),
      verbosity(static_cast<Verbosity>(opts.get_enum("verbosity"))),
      max_time(opts.get<double>("max_time")),
      num_transitions_to_abort(opts.get<int>("num_transitions_to_abort")),
      partial_mas_method(static_cast<PartialMASMethod>(opts.get_enum("partial_mas_method"))),
      starting_peak_memory(-1) {
    assert(max_states_before_merge > 0);
    assert(max_states >= max_states_before_merge);
    assert(shrink_threshold_before_merge <= max_states_before_merge);

    if (opts.contains("label_reduction")) {
        label_reduction = opts.get<shared_ptr<LabelReduction>>("label_reduction");
        label_reduction->initialize(task_proxy);
    }

    if (opts.contains("factor_scoring_functions")) {
        factor_scoring_functions = opts.get_list<shared_ptr<FactorScoringFunction>>(
            "factor_scoring_functions");
    }

    utils::Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    starting_peak_memory = utils::get_peak_memory_in_kb();
    task_properties::verify_no_axioms(task_proxy);
    dump_options();
    warn_on_unusual_options();
    cout << endl;

    build(timer);
    const bool final = true;
    report_peak_memory_delta(final);
    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl;
    cout << endl;
}

void MergeAndShrinkHeuristic::report_peak_memory_delta(bool final) const {
    if (final)
        cout << "Final";
    else
        cout << "Current";
    cout << " peak memory increase of merge-and-shrink computation: "
         << utils::get_peak_memory_in_kb() - starting_peak_memory << " KB"
         << endl;
}

void MergeAndShrinkHeuristic::dump_options() const {
    if (merge_strategy_factory) { // deleted after merge strategy extraction
        merge_strategy_factory->dump_options();
        cout << endl;
    }

    cout << "Options related to size limits and shrinking: " << endl;
    cout << "Transition system size limit: " << max_states << endl
         << "Transition system size limit right before merge: "
         << max_states_before_merge << endl;
    cout << "Threshold to trigger shrinking right before merge: "
         << shrink_threshold_before_merge << endl;
    cout << endl;

    shrink_strategy->dump_options();
    cout << endl;

    if (label_reduction) {
        label_reduction->dump_options();
    } else {
        cout << "Label reduction disabled" << endl;
    }
    cout << endl;

    cout << "Verbosity: ";
    switch (verbosity) {
    case Verbosity::SILENT:
        cout << "silent";
        break;
    case Verbosity::NORMAL:
        cout << "normal";
        break;
    case Verbosity::VERBOSE:
        cout << "verbose";
        break;
    }
    cout << endl;
}

void MergeAndShrinkHeuristic::warn_on_unusual_options() const {
    string dashes(79, '=');
    if (!label_reduction) {
        cout << dashes << endl
             << "WARNING! You did not enable label reduction.\nThis may "
            "drastically reduce the performance of merge-and-shrink!"
             << endl << dashes << endl;
    } else if (label_reduction->reduce_before_merging() && label_reduction->reduce_before_shrinking()) {
        cout << dashes << endl
             << "WARNING! You set label reduction to be applied twice in each merge-and-shrink\n"
            "iteration, both before shrinking and merging. This double computation effort\n"
            "does not pay off for most configurations!"
             << endl << dashes << endl;
    } else {
        if (label_reduction->reduce_before_shrinking() &&
            (shrink_strategy->get_name() == "f-preserving"
             || shrink_strategy->get_name() == "random")) {
            cout << dashes << endl
                 << "WARNING! Bucket-based shrink strategies such as f-preserving random perform\n"
                "best if used with label reduction before merging, not before shrinking!"
                 << endl << dashes << endl;
        }
        if (label_reduction->reduce_before_merging() &&
            shrink_strategy->get_name() == "bisimulation") {
            cout << dashes << endl
                 << "WARNING! Shrinking based on bisimulation performs best if used with label\n"
                "reduction before shrinking, not before merging!"
                 << endl << dashes << endl;
        }
    }

    if (!prune_unreachable_states || !prune_irrelevant_states) {
        cout << dashes << endl
             << "WARNING! Pruning is (partially) turned off!\nThis may "
            "drastically reduce the performance of merge-and-shrink!"
             << endl << dashes << endl;
    }
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

bool MergeAndShrinkHeuristic::ran_out_of_time(
    const utils::Timer &timer) const {
    if (timer() > max_time) {
        if (verbosity >= Verbosity::NORMAL) {
            cout << "Ran out of time, stopping computation." << endl;
            cout << endl;
        }
        return true;
    }
    return false;
}

bool MergeAndShrinkHeuristic::too_many_transitions(int num_transitions) const {
    if (num_transitions > num_transitions_to_abort) {
        if (verbosity >= Verbosity::NORMAL) {
            cout << "Factor has too many transitions, stopping computation."
                 << endl;
            cout << endl;
        }
        return true;
    }
    return false;
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

    assert(current_indices.size() == 1);
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
    assert(partial_mas_method != PartialMASMethod::None);
    int active_factors_count = 0;
    for (int index = 0; index < fts.get_size(); ++index) {
        if (fts.is_active(index)) {
            ++active_factors_count;
        }
    }
    if (verbosity >= Verbosity::NORMAL) {
        cout << "Number of remaining factors: " << active_factors_count << endl;
    }
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
            cout << "Chose single factor as heuristic: " << fts.get_ts(index).tag()
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
        utils::exit_with(utils::ExitCode::UNSUPPORTED);
    }
    if (verbosity >= Verbosity::NORMAL) {
        cout << endl;
    }
}

int MergeAndShrinkHeuristic::prune_fts(
    FactoredTransitionSystem &fts, const utils::Timer &timer) const {
    /*
      Prune all factors according to the chosen options. Stop early if one
      factor is unsolvable and return its index.
    */
    int unsolvable_index = -1;
    bool pruned = false;
    for (int index = 0; index < fts.get_size(); ++index) {
        if (prune_unreachable_states || prune_irrelevant_states) {
            bool pruned_factor = prune_step(
                fts,
                index,
                prune_unreachable_states,
                prune_irrelevant_states,
                verbosity);
            pruned = pruned || pruned_factor;
        }
        if (!fts.is_factor_solvable(index)) {
            unsolvable_index = index;
            break;
        }
    }
    if (verbosity >= Verbosity::NORMAL && pruned) {
        print_time(timer, "after pruning atomic factors");
    }
    return unsolvable_index;
}

int MergeAndShrinkHeuristic::main_loop(
    FactoredTransitionSystem &fts, const utils::Timer &timer) {
    int maximum_intermediate_size = 0;
    for (int i = 0; i < fts.get_size(); ++i) {
        int size = fts.get_ts(i).get_size();
        if (size > maximum_intermediate_size) {
            maximum_intermediate_size = size;
        }
    }

    unique_ptr<MergeStrategy> merge_strategy =
        merge_strategy_factory->compute_merge_strategy(task_proxy, fts);
    merge_strategy_factory = nullptr;

    int iteration_counter = 0;
    int final_index = -1;
    while (fts.get_num_active_entries() > 1) {
        // Choose next transition systems to merge
        pair<int, int> merge_indices = merge_strategy->get_next();
        if (ran_out_of_time(timer)) {
            final_index = -2;
            break;
        }
        int merge_index1 = merge_indices.first;
        int merge_index2 = merge_indices.second;
        assert(merge_index1 != merge_index2);
        if (verbosity >= Verbosity::NORMAL) {
            cout << "Next pair of indices: ("
                 << merge_index1 << ", " << merge_index2 << ")" << endl;
            if (verbosity >= Verbosity::VERBOSE) {
                fts.statistics(merge_index1);
                fts.statistics(merge_index2);
            }
            print_time(timer, "after computation of next merge");
        }

        // Label reduction (before shrinking)
        if (label_reduction && label_reduction->reduce_before_shrinking()) {
            bool reduced = label_reduction->reduce(merge_indices, fts, verbosity);
            if (verbosity >= Verbosity::NORMAL && reduced) {
                print_time(timer, "after label reduction");
            }
        }

        if (ran_out_of_time(timer)) {
            final_index = -2;
            break;
        }

        // Shrinking
        bool shrunk = shrink_before_merge_step(
            fts,
            merge_index1,
            merge_index2,
            max_states,
            max_states_before_merge,
            shrink_threshold_before_merge,
            *shrink_strategy,
            verbosity);
        if (verbosity >= Verbosity::NORMAL && shrunk) {
            print_time(timer, "after shrinking");
        }

        if (ran_out_of_time(timer)) {
            final_index = -2;
            break;
        }

        // Label reduction (before merging)
        if (label_reduction && label_reduction->reduce_before_merging()) {
            bool reduced = label_reduction->reduce(merge_indices, fts, verbosity);
            if (verbosity >= Verbosity::NORMAL && reduced) {
                print_time(timer, "after label reduction");
            }
        }

        if (ran_out_of_time(timer)) {
            final_index = -2;
            break;
        }

        // Merging
        int merged_index = fts.merge(merge_index1, merge_index2, verbosity);
        int abs_size = fts.get_ts(merged_index).get_size();
        if (abs_size > maximum_intermediate_size) {
            maximum_intermediate_size = abs_size;
        }

        if (verbosity >= Verbosity::NORMAL) {
            if (verbosity >= Verbosity::VERBOSE) {
                fts.statistics(merged_index);
            }
            print_time(timer, "after merging");
        }

        // We do not check for num transitions here but only after pruning
        // to allow recovering a too large product.
        if (ran_out_of_time(timer)) {
            final_index = -2;
            break;
        }

        // Pruning
        if (prune_unreachable_states || prune_irrelevant_states) {
            bool pruned = prune_step(
                fts,
                merged_index,
                prune_unreachable_states,
                prune_irrelevant_states,
                verbosity);
            if (verbosity >= Verbosity::NORMAL && pruned) {
                if (verbosity >= Verbosity::VERBOSE) {
                    fts.statistics(merged_index);
                }
                print_time(timer, "after pruning");
            }
        }

        /*
          NOTE: both the shrink strategy classes and the construction
          of the composite transition system require the input
          transition systems to be non-empty, i.e. the initial state
          not to be pruned/not to be evaluated as infinity.
        */
        if (!fts.is_factor_solvable(merged_index)) {
            if (verbosity >= Verbosity::NORMAL) {
                cout << "Abstract problem is unsolvable, stopping computation."
                     << endl << endl;
            }
            final_index = merged_index;
            break;
        }

        int num_trans = fts.get_ts(merged_index).compute_total_transitions();
        if (ran_out_of_time(timer) || too_many_transitions(num_trans)) {
            final_index = -2;
            break;
        }

        // End-of-iteration output.
        if (verbosity >= Verbosity::VERBOSE) {
            report_peak_memory_delta();
        }
        if (verbosity >= Verbosity::NORMAL) {
            cout << endl;
        }

        ++iteration_counter;
    }

    if (final_index == -1) {
        /*
          We regularly finished the merge-and-shrink construction, i.e., we
          merged all transition systems and are left with one solvable
          transition system. This assumes that merges are always appended at
          the end.
        */
        for (int index = 0; index < fts.get_size() - 1; ++index) {
            assert(!fts.is_active(index));
        }
        final_index = fts.get_size() - 1;
        assert(fts.is_factor_solvable(final_index));
        cout << "Main loop terminated regularly. Statistics:" << endl;
        cout << "Final transition system size: "
             << fts.get_ts(final_index).get_size() << endl;
    } else {
        cout << "Main loop terminated early (either because a factor is "
            "unsolvable or the time or transition limit was reached). "
            "Statistics:" << endl;
    }

    cout << "Maximum intermediate abstraction size: "
         << maximum_intermediate_size << endl;
    cout << endl;

    shrink_strategy = nullptr;
    label_reduction = nullptr;

    return final_index;
}

void MergeAndShrinkHeuristic::build(const utils::Timer &timer) {
    const bool compute_init_distances =
        shrink_strategy->requires_init_distances() ||
        merge_strategy_factory->requires_init_distances() ||
        prune_unreachable_states;
    const bool compute_goal_distances =
        shrink_strategy->requires_goal_distances() ||
        merge_strategy_factory->requires_goal_distances() ||
        prune_irrelevant_states;
    FactoredTransitionSystem fts =
        create_factored_transition_system(
            task_proxy,
            compute_init_distances,
            compute_goal_distances,
            verbosity,
            max_time,
            timer);
    if (verbosity >= Verbosity::NORMAL) {
        print_time(timer, "after computation of atomic transition systems");
    }
    int unsolvable_index = prune_fts(fts, timer);
    if (unsolvable_index != -1) {
        cout << "Atomic factor is unsolvable, stopping computation."
             << endl << endl;
        // Use the unsolvable factor as the final abstraction.
        finalize_factor(fts, unsolvable_index);
        return;
    }
    if (verbosity >= Verbosity::NORMAL) {
        cout << endl;
    }

    if (ran_out_of_time(timer)) {
        // Ran out of time, do not proceed with main loop.
        finalize(fts);
        return;
    }

    for (int index = 0; index < fts.get_size(); ++index) {
        const TransitionSystem &ts = fts.get_ts(index);
        int num_transitions = ts.compute_total_transitions();
        if (too_many_transitions(num_transitions)) {
            // A factor grew too many transitions, do not proceed with main loop.
            finalize(fts);
            return;
        }
    }

    int final_index = main_loop(fts, timer);
    if (final_index == -2) {
        // Main loop terminated early due to reaching the time or transition limit.
        finalize(fts);
    } else {
        /*
          Main loop terminated regularly and final_index points to the last
          factor, or it terminated early and final_index points to an unsolvable
          factor. In both cases, we use this factor as the final abstraction.
        */
        finalize_factor(fts, final_index);
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

void MergeAndShrinkHeuristic::add_shrink_limit_options_to_parser(OptionParser &parser) {
    parser.add_option<int>(
        "max_states",
        "maximum transition system size allowed at any time point.",
        "-1",
        Bounds("-1", "infinity"));
    parser.add_option<int>(
        "max_states_before_merge",
        "maximum transition system size allowed for two transition systems "
        "before being merged to form the synchronized product.",
        "-1",
        Bounds("-1", "infinity"));
    parser.add_option<int>(
        "threshold_before_merge",
        "If a transition system, before being merged, surpasses this soft "
        "transition system size limit, the shrink strategy is called to "
        "possibly shrink the transition system.",
        "-1",
        Bounds("-1", "infinity"));
}

void MergeAndShrinkHeuristic::handle_shrink_limit_options_defaults(Options &opts) {
    int max_states = opts.get<int>("max_states");
    int max_states_before_merge = opts.get<int>("max_states_before_merge");
    int threshold = opts.get<int>("threshold_before_merge");

    // If none of the two state limits has been set: set default limit.
    if (max_states == -1 && max_states_before_merge == -1) {
        max_states = 50000;
    }

    // If exactly one of the max_states options has been set, set the other
    // so that it imposes no further limits.
    if (max_states_before_merge == -1) {
        max_states_before_merge = max_states;
    } else if (max_states == -1) {
        int n = max_states_before_merge;
        if (utils::is_product_within_limit(n, n, INF)) {
            max_states = n * n;
        } else {
            max_states = INF;
        }
    }

    if (max_states_before_merge > max_states) {
        cout << "warning: max_states_before_merge exceeds max_states, "
             << "correcting." << endl;
        max_states_before_merge = max_states;
    }

    if (max_states < 1) {
        cerr << "error: transition system size must be at least 1" << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }

    if (max_states_before_merge < 1) {
        cerr << "error: transition system size before merge must be at least 1"
             << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }

    if (threshold == -1) {
        threshold = max_states;
    }
    if (threshold < 1) {
        cerr << "error: threshold must be at least 1" << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }
    if (threshold > max_states) {
        cout << "warning: threshold exceeds max_states, correcting" << endl;
        threshold = max_states;
    }

    opts.set<int>("max_states", max_states);
    opts.set<int>("max_states_before_merge", max_states_before_merge);
    opts.set<int>("threshold_before_merge", threshold);
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Merge-and-shrink heuristic",
        "This heuristic implements the algorithm described in the following "
        "paper:" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "Generalized Label Reduction for Merge-and-Shrink Heuristics",
            "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014") + "\n" +
        "For a more exhaustive description of merge-and-shrink, see the journal "
        "paper" + utils::format_paper_reference(
            {"Malte Helmert", "Patrik Haslum", "Joerg Hoffmann", "Raz Nissim"},
            "Merge-and-Shrink Abstraction: A Method for Generating Lower Bounds"
            " in Factored State Spaces",
            "http://ai.cs.unibas.ch/papers/helmert-et-al-jacm2014.pdf",
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
            "http://ai.cs.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
            "Proceedings of the 26th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2016)",
            "294-298",
            "AAAI Press 2016"));
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported (but see note)");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
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

    // Merge strategy option.
    parser.add_option<shared_ptr<MergeStrategyFactory>>(
        "merge_strategy",
        "See detailed documentation for merge strategies. "
        "We currently recommend SCC-DFP, which can be achieved using "
        "{{{merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector="
        "score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order"
        "]))}}}");

    // Shrink strategy option.
    parser.add_option<shared_ptr<ShrinkStrategy>>(
        "shrink_strategy",
        "See detailed documentation for shrink strategies. "
        "We currently recommend non-greedy shrink_bisimulation, which can be "
        "achieved using {{{shrink_strategy=shrink_bisimulation(greedy=false)}}}");

    // Label reduction option.
    parser.add_option<shared_ptr<LabelReduction>>(
        "label_reduction",
        "See detailed documentation for labels. There is currently only "
        "one 'option' to use label_reduction, which is {{{label_reduction=exact}}} "
        "Also note the interaction with shrink strategies.",
        OptionParser::NONE);

    // Pruning options.
    parser.add_option<bool>(
        "prune_unreachable_states",
        "If true, prune abstract states unreachable from the initial state.",
        "true");
    parser.add_option<bool>(
        "prune_irrelevant_states",
        "If true, prune abstract states from which no goal state can be "
        "reached.",
        "true");

    MergeAndShrinkHeuristic::add_shrink_limit_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);

    vector<string> verbosity_levels;
    vector<string> verbosity_level_docs;
    verbosity_levels.push_back("silent");
    verbosity_level_docs.push_back(
        "silent: no output during construction, only starting and final "
        "statistics");
    verbosity_levels.push_back("normal");
    verbosity_level_docs.push_back(
        "normal: basic output during construction, starting and final "
        "statistics");
    verbosity_levels.push_back("verbose");
    verbosity_level_docs.push_back(
        "verbose: full output during construction, starting and final "
        "statistics");
    parser.add_enum_option(
        "verbosity",
        verbosity_levels,
        "Option to specify the level of verbosity.",
        "verbose",
        verbosity_level_docs);

    parser.add_option<double>(
        "max_time",
        "A limit in seconds on the computation time of the heuristic. "
        "If the limit is surpassed, the algorithm terminates, leaving the "
        "chosen partial_mas_method to compute a heuristic from the set of "
        "remaining factors.",
        "infinity",
        Bounds("0.0", "infinity"));
    parser.add_option<int>(
        "num_transitions_to_abort",
        "A limit on the number of transitions of any factor during the "
        "computation. Once this limit is reached, the algorithm terminates, "
        "leaving the chosen partial_mas_method to compute a heuristic from the "
        "set of remaining factors.",
        "infinity",
        Bounds("0", "infinity"));
    vector<string> partial_mas_method;
    vector<string> partial_mas_method_docs;
    partial_mas_method.push_back("none");
    partial_mas_method_docs.push_back(
        "none: attempt to compute a merge-and-shrink abstraction over all "
        "variables of the planning task. Do not set a finite value for any of"
        "the options max_time or num_transitions_to_abort");
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
        "Method to determine the final heuristic given an early abortion, "
        "such as due to reaching the time or transitions limit.",
        "none",
        partial_mas_method_docs);
    parser.add_list_option<shared_ptr<FactorScoringFunction>>(
        "factor_scoring_functions",
        "The list of factor scoring functions used to compute scores for "
        "remaining factors if computing partial merge-and-shrink abstractions, "
        "i.e., if partial_mas_method != none.",
        options::OptionParser::NONE);

    Options opts = parser.parse();
    if (parser.help_mode()) {
        return nullptr;
    }

    MergeAndShrinkHeuristic::handle_shrink_limit_options_defaults(opts);

    if (parser.dry_run()) {
        double max_time = opts.get<double>("max_time");
        int num_transitions_to_abort = opts.get<int>("num_transitions_to_abort");
        PartialMASMethod partial_mas_method = static_cast<PartialMASMethod>(opts.get_enum("partial_mas_method"));
        if (partial_mas_method != PartialMASMethod::None
            && (max_time == numeric_limits<double>::infinity()
                && num_transitions_to_abort == INF)) {
            cerr << "If using a partial merge-and-shrink method, you must "
                "use a finite value for at least one of max_time and "
                "num_transitions_to_abort. "
                 << endl;
            utils::exit_with(utils::ExitCode::INPUT_ERROR);
        }
        if (partial_mas_method == PartialMASMethod::None
            && (max_time < INF || num_transitions_to_abort < INF)) {
            cerr << "If using a finite value to any of max_time and "
                "num_transitions_to_abort, you also must use a partial "
                "merge-and-shrink method."
                 << endl;
            utils::exit_with(utils::ExitCode::INPUT_ERROR);
        }
        if (partial_mas_method == PartialMASMethod::Single
            && !opts.contains("factor_scoring_functions")) {
            cerr << "If using the partial merge-and-shrink method single, "
                "you must specify a least one factor scoring function!"
                 << endl;
            utils::exit_with(utils::ExitCode::INPUT_ERROR);
        }
        return nullptr;
    } else {
        return new MergeAndShrinkHeuristic(opts);
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
}
