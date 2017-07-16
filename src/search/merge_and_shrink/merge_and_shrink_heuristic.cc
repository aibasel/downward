#include "merge_and_shrink_heuristic.h"

#include "distances.h"
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
#include "../utils/memory.h"
#include "../utils/system.h"
#include "../utils/timer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
static void print_time(const utils::Timer &timer, string text) {
    cout << "t=" << timer << " (" << text << ")" << endl;
}

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
      starting_peak_memory(-1),
      mas_representation(nullptr) {
    assert(max_states_before_merge > 0);
    assert(max_states >= max_states_before_merge);
    assert(shrink_threshold_before_merge <= max_states_before_merge);

    if (opts.contains("label_reduction")) {
        label_reduction = opts.get<shared_ptr<LabelReduction>>("label_reduction");
        label_reduction->initialize(task_proxy);
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
        cerr << dashes << endl
             << "WARNING! You did not enable label reduction.\nThis may "
            "drastically reduce the performance of merge-and-shrink!"
             << endl << dashes << endl;
    } else if (label_reduction->reduce_before_merging() && label_reduction->reduce_before_shrinking()) {
        cerr << dashes << endl
             << "WARNING! You set label reduction to be applied twice in each merge-and-shrink\n"
            "iteration, both before shrinking and merging. This double computation effort\n"
            "does not pay off for most configurations!"
             << endl << dashes << endl;
    } else {
        if (label_reduction->reduce_before_shrinking() &&
            (shrink_strategy->get_name() == "f-preserving"
             || shrink_strategy->get_name() == "random")) {
            cerr << dashes << endl
                 << "WARNING! Bucket-based shrink strategies such as f-preserving random perform\n"
                "best if used with label reduction before merging, not before shrinking!"
                 << endl << dashes << endl;
        }
        if (label_reduction->reduce_before_merging() &&
            shrink_strategy->get_name() == "bisimulation") {
            cerr << dashes << endl
                 << "WARNING! Shrinking based on bisimulation performs best if used with label\n"
                "reduction before shrinking, not before merging!"
                 << endl << dashes << endl;
        }
    }

    if (!prune_unreachable_states || !prune_irrelevant_states) {
        cerr << dashes << endl
             << "WARNING! Pruning is (partially) turned off!\nThis may "
            "drastically reduce the performance of merge-and-shrink!"
             << endl << dashes << endl;
    }
}

bool MergeAndShrinkHeuristic::shrink_before_merge(
    FactoredTransitionSystem &fts, int index1, int index2) {
    /*
      Compute the size limit for both transition systems as imposed by
      max_states and max_states_before_merge.
    */
    pair<int, int> new_sizes = compute_shrink_sizes(
        fts.get_ts(index1).get_size(),
        fts.get_ts(index2).get_size(),
        max_states_before_merge,
        max_states);

    /*
      For both transition systems, possibly compute and apply an
      abstraction.
      TODO: we could better use the given limit by increasing the size limit
      for the second shrinking if the first shrinking was larger than
      required.
    */
    bool shrunk1 = shrink_factor(
        fts,
        index1,
        new_sizes.first,
        shrink_threshold_before_merge,
        *shrink_strategy,
        verbosity);
    if (verbosity >= Verbosity::VERBOSE && shrunk1) {
        fts.statistics(index1);
    }
    bool shrunk2 = shrink_factor(
        fts,
        index2,
        new_sizes.second,
        shrink_threshold_before_merge,
        *shrink_strategy,
        verbosity);
    if (verbosity >= Verbosity::VERBOSE && shrunk2) {
        fts.statistics(index2);
    }
    return shrunk1 || shrunk2;
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
            verbosity);
    int unsolvable_index = -1;
    /*
      Go over all atomic factors and check if any is unsolvable. If so,
      we can skip the main loop and immediately terminate the heuristic
      computation.
    */
    for (int index = 0; index < fts.get_size(); ++index) {
        if (prune_unreachable_states || prune_irrelevant_states) {
            prune_factor(
                fts,
                index,
                prune_unreachable_states,
                prune_irrelevant_states,
                verbosity);
        }
        if (!fts.is_factor_solvable(index)) {
            unsolvable_index = index;
            break;
        }
    }
    print_time(timer, "after computation of atomic transition systems");
    cout << endl;

    if (unsolvable_index == -1) { // All atomic transition systems are solvable.
        unique_ptr<MergeStrategy> merge_strategy =
            merge_strategy_factory->compute_merge_strategy(task_proxy, fts);
        merge_strategy_factory = nullptr;

        while (fts.get_num_active_entries() > 1) {
            // Choose next transition systems to merge
            pair<int, int> merge_indices = merge_strategy->get_next();
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

            // Shrinking
            bool shrunk = shrink_before_merge(
                fts, merge_index1, merge_index2);
            if (verbosity >= Verbosity::NORMAL && shrunk) {
                print_time(timer, "after shrinking");
            }

            // Label reduction (before merging)
            if (label_reduction && label_reduction->reduce_before_merging()) {
                bool reduced = label_reduction->reduce(merge_indices, fts, verbosity);
                if (verbosity >= Verbosity::NORMAL && reduced) {
                    print_time(timer, "after label reduction");
                }
            }

            // Merging
            int merged_index = fts.merge(merge_index1, merge_index2, verbosity);
            if (verbosity >= Verbosity::NORMAL) {
                if (verbosity >= Verbosity::VERBOSE) {
                    fts.statistics(merged_index);
                }
                print_time(timer, "after merging");
            }

            // Pruning
            if (prune_unreachable_states || prune_irrelevant_states) {
                bool pruned = prune_factor(
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

            // End-of-iteration output.
            if (verbosity >= Verbosity::VERBOSE) {
                report_peak_memory_delta();
            }
            cout << endl;

            /*
              NOTE: both the shrink strategy classes and the construction
              of the composite transition system require the input
              transition systems to be non-empty, i.e. the initial state
              not to be pruned/not to be evaluated as infinity.
            */
            if (!fts.is_factor_solvable(merged_index)) {
                unsolvable_index = merged_index;
                break;
            }
        }
    }

    int final_index;
    if (unsolvable_index == -1) {
        /*
          If unsolvable_index == -1, we "regularly" finished the merge-and-
          shrink construction, i.e. we merged all transition systems and are
          left with one solvable transition system. This assumes that merges
          are always appended at the end.
        */
        for (int index = 0; index < fts.get_size() - 1; ++index) {
            assert(!fts.is_active(index));
        }
        final_index = fts.get_size() - 1;
        assert(fts.is_factor_solvable(final_index));
        cout << "Final transition system size: "
             << fts.get_ts(final_index).get_size() << endl;
    } else {
        /*
          unsolvable_index points to an unsolvable transition system (this
          happens if we exited the main loop prior to its regular termination).
        */
        final_index = unsolvable_index;
        cout << "Abstract problem is unsolvable!" << endl;
    }

    pair<unique_ptr<MergeAndShrinkRepresentation>, unique_ptr<Distances>>
    final_entry = fts.extract_factor(final_index);
    mas_representation = move(final_entry.first);
    mas_representation->set_distances(*final_entry.second);
    shrink_strategy = nullptr;
    label_reduction = nullptr;
}

int MergeAndShrinkHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    int cost = mas_representation->get_value(state);
    if (cost == PRUNED_STATE || cost == INF) {
        // If state is unreachable or irrelevant, we encountered a dead end.
        return DEAD_END;
    }
    return cost;
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
        cerr << "warning: max_states_before_merge exceeds max_states, "
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
        cerr << "warning: threshold exceeds max_states, correcting" << endl;
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
            "AAAI Press 2016") + "\n" +
        "Note that dyn-MIASM has not been integrated into the official code "
        "base of Fast Downward and is available on request.");
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
        "total_order])),"
        "label_reduction=exact(before_shrinking=true,"
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

    Options opts = parser.parse();
    if (parser.help_mode()) {
        return nullptr;
    }

    MergeAndShrinkHeuristic::handle_shrink_limit_options_defaults(opts);

    if (parser.dry_run()) {
        return nullptr;
    } else {
        return new MergeAndShrinkHeuristic(opts);
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
}
