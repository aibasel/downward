#include "merge_and_shrink_heuristic.h"

#include "abstraction.h"
#include "labels.h"
#include "shrink_fh.h"
#include "variable_order_finder.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../timer.h"

#include <cassert>
#include <vector>
using namespace std;


MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : Heuristic(opts),
      merge_strategy(MergeStrategy(opts.get_enum("merge_strategy"))),
      shrink_strategy(opts.get<ShrinkStrategy *>("shrink_strategy")),
      use_label_reduction(opts.get<bool>("reduce_labels")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")) {
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
    delete labels;
}

void MergeAndShrinkHeuristic::dump_options() const {
    cout << "Merge strategy: ";
    switch (merge_strategy) {
    case MERGE_LINEAR_CG_GOAL_LEVEL:
        cout << "linear CG/GOAL, tie breaking on level (main)";
        break;
    case MERGE_LINEAR_CG_GOAL_RANDOM:
        cout << "linear CG/GOAL, tie breaking random";
        break;
    case MERGE_LINEAR_GOAL_CG_LEVEL:
        cout << "linear GOAL/CG, tie breaking on level";
        break;
    case MERGE_LINEAR_RANDOM:
        cout << "linear random";
        break;
    case MERGE_DFP:
        cout << "Draeger/Finkbeiner/Podelski" << endl;
        cerr << "DFP merge strategy not implemented." << endl;
        exit_with(EXIT_UNSUPPORTED);
        break;
    case MERGE_LINEAR_LEVEL:
        cout << "linear by level";
        break;
    case MERGE_LINEAR_REVERSE_LEVEL:
        cout << "linear by reverse level";
        break;
    default:
        ABORT("Unknown merge strategy.");
    }
    cout << endl;
    shrink_strategy->dump_options();
    cout << "Label reduction: "
         << (use_label_reduction ? "enabled" : "disabled") << endl
         << "Expensive statistics: "
         << (use_expensive_statistics ? "enabled" : "disabled") << endl;
}

void MergeAndShrinkHeuristic::warn_on_unusual_options() const {
    if (use_expensive_statistics) {
        string dashes(79, '=');
        cerr << dashes << endl
             << ("WARNING! You have enabled extra statistics for "
            "merge-and-shrink heuristics.\n"
            "These statistics require a lot of time and memory.\n"
            "When last tested (around revision 3011), enabling the "
            "extra statistics\nincreased heuristic generation time by "
            "76%. This figure may be significantly\nworse with more "
            "recent code or for particular domains and instances.\n"
            "You have been warned. Don't use this for benchmarking!")
        << endl << dashes << endl;
    }
}

EquivalenceRelation MergeAndShrinkHeuristic::compute_outside_equivalence(const Abstraction *abstraction,
                                                                         const vector<Abstraction *> &all_abstractions) const {
    /*Returns an equivalence relation over operators s.t. o ~ o'
    iff o and o' are locally equivalent in all transition systems
    T' \neq T. (They may or may not be locally equivalent in T.) */
    cout << "compute outside equivalence for " << abstraction->tag() << endl;

    vector<pair<int, int> > labeled_label_nos;
    labeled_label_nos.reserve(labels->get_size());
    for (int label_no = 0; label_no < labels->get_size(); ++label_no) {
        labeled_label_nos.push_back(make_pair(0, label_no));
    }
    // start with the relation where all labels are equivalent
    EquivalenceRelation relation = EquivalenceRelation::from_labels<int>(labeled_label_nos.size(), labeled_label_nos);
    for (size_t i = 0; i < all_abstractions.size(); ++i) {
        if (!all_abstractions[i])
            continue;
        Abstraction *abs = all_abstractions[i];
        if (abs != abstraction) {
            cout << "computing local equivalence for " << abs->tag() << endl;
            if (!abs->is_normalized()) {
                cout << "need to normalize" << endl;
                abs->normalize();
            }
            assert(abs->is_normalized());
            assert(abs->sorted_unique());
            //vector<pair<int, int> > labeled_label_nos;
            EquivalenceRelation next_relation = abs->compute_local_equivalence_relation(/*labeled_label_nos*/);
            //vector<pair<int, int> > labeled_label_nos2;
            EquivalenceRelation next_relation2 = abs->compute_local_equivalence_relation2(/*labeled_label_nos2*/);
            //assert(labeled_label_nos == labeled_label_nos2);
            assert(next_relation.get_num_elements() == next_relation2.get_num_elements());
            assert(next_relation.get_num_blocks() == next_relation2.get_num_blocks());
            for (BlockListConstIter it = next_relation.begin(); it != next_relation.end(); ++it) {
                const Block &block = *it;
                ElementListConstIter jt = block.begin();
                if (!block.empty()) {
                    int label = *jt;
                    if (labels->is_label_reduced(label)) {
                        // ignore already reduced labels
                        continue;
                    }
                    // find label and its block in next_relation2
                    const Block *block_of_label_in_next_relation2 = 0;
                    for (BlockListConstIter it2 = next_relation2.begin(); it2 != next_relation2.end(); ++it2) {
                        const Block &block2 = *it2;
                        for (ElementListConstIter jt2 = block2.begin(); jt2 != block2.end(); ++jt2) {
                            int label2 = *jt2;
                            if (labels->is_label_reduced(label2)) {
                                // ignore already reduced labels
                                continue;
                            }
                            if (label2 == label) {
                                block_of_label_in_next_relation2 = &block2;
                                break;
                            }
                        }
                    }
                    assert(block_of_label_in_next_relation2);
                    for (ElementListConstIter block1it = block.begin(); block1it != block.end(); ++block1it) {
                        int label_no = *block1it;
                        bool found_label = false;
                        for (ElementListConstIter block2it = block_of_label_in_next_relation2->begin();
                             block2it != block_of_label_in_next_relation2->end(); ++block2it) {
                            if (label_no == *block2it) {
                                found_label = true;
                                break;
                            }
                        }
                        assert(found_label);
                    }
                }
            }
            relation.refine(next_relation);
        }
    }
    return relation;
}

Abstraction *MergeAndShrinkHeuristic::build_abstraction() {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_abstractions also
    //       allocates memory.

    vector<Abstraction *> atomic_abstractions;
    Abstraction::build_atomic_abstractions(
        is_unit_cost_problem(), atomic_abstractions, labels);
    vector<Abstraction *> all_abstractions(atomic_abstractions);

    cout << "Shrinking atomic abstractions..." << endl;
    for (size_t i = 0; i < atomic_abstractions.size(); ++i) {
        assert(atomic_abstractions[i]->sorted_unique());
        atomic_abstractions[i]->compute_distances();
        if (!atomic_abstractions[i]->is_solvable())
            return atomic_abstractions[i];
        shrink_strategy->shrink_atomic(*atomic_abstractions[i]);
    }

    cout << "Merging abstractions..." << endl;

    VariableOrderFinder order(merge_strategy);

    int var_first = order.next();
    cout << "First variable: #" << var_first << endl;
    Abstraction *abstraction = atomic_abstractions[var_first];
    abstraction->statistics(use_expensive_statistics);

    while (!order.done()) {
        int var_no = order.next();
        cout << "Next variable: #" << var_no << endl;
        Abstraction *other_abstraction = atomic_abstractions[var_no];

        // TODO: When using nonlinear merge strategies, make sure not
        // to normalize multiple parts of a composite. See issue68.
        // TODO: do not reduce labels several times for the same abstraction!
        bool reduced_labels = false;
        EquivalenceRelation relation = compute_outside_equivalence(abstraction, all_abstractions);
        if (shrink_strategy->reduce_labels_before_shrinking()) {
            labels->reduce_labels(abstraction->get_relevant_labels(), abstraction->get_varset(), &relation);
            reduced_labels = true;
            abstraction->normalize();
            assert(abstraction->sorted_unique());
            // no label reduction for other_abstraction now
            other_abstraction->normalize();
            assert(other_abstraction->sorted_unique());
        }

        abstraction->compute_distances();
        if (!abstraction->is_solvable())
            return abstraction;

        other_abstraction->compute_distances();
        shrink_strategy->shrink_before_merge(*abstraction, *other_abstraction);
        // TODO: Make shrink_before_merge return a pair of bools
        //       that tells us whether they have actually changed,
        //       and use that to decide whether to dump statistics?
        // (The old code would print statistics on abstraction iff it was
        // shrunk. This is not so easy any more since this method is not
        // in control, and the shrink strategy doesn't know whether we want
        // expensive statistics. As a temporary aid, we just print the
        // statistics always now, whether or not we shrunk.)
        abstraction->statistics(use_expensive_statistics);
        other_abstraction->statistics(use_expensive_statistics);

        if (!reduced_labels) {
            labels->reduce_labels(abstraction->get_relevant_labels(), abstraction->get_varset(), &relation);
        }
        abstraction->normalize();
        assert(abstraction->sorted_unique());
        abstraction->statistics(use_expensive_statistics);

        // Don't label-reduce the atomic abstraction -- see issue68.
        other_abstraction->normalize();
        assert(abstraction->sorted_unique());
        other_abstraction->statistics(use_expensive_statistics);

        Abstraction *new_abstraction = new CompositeAbstraction(
            is_unit_cost_problem(),
            labels,
            abstraction, other_abstraction);

        abstraction->release_memory();
        other_abstraction->release_memory();

        abstraction = new_abstraction;
        abstraction->statistics(use_expensive_statistics);

        all_abstractions[var_first] = abstraction;
        all_abstractions[var_no] = 0;
    }

    abstraction->compute_distances();
    if (!abstraction->is_solvable())
        return abstraction;

    ShrinkStrategy *def_shrink = ShrinkFH::create_default(abstraction->size());
    def_shrink->shrink(*abstraction, abstraction->size(), true);
    abstraction->compute_distances();

    abstraction->statistics(use_expensive_statistics);
    abstraction->release_memory();
    return abstraction;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms_no_cond_effects();

    cout << "Building abstraction..." << endl;
    labels = new Labels(cost_type);
    final_abstraction = build_abstraction();
    if (!final_abstraction->is_solvable()) {
        cout << "Abstract problem is unsolvable!" << endl;
    }

    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl << "initial h value: " << compute_heuristic(g_initial_state()) << endl;
    cout << "Estimated peak memory for abstraction: " << final_abstraction->get_peak_memory_estimate() << " bytes" << endl;
}

int MergeAndShrinkHeuristic::compute_heuristic(const State &state) {
    int cost = final_abstraction->get_cost(state);
    if (cost == -1)
        return DEAD_END;
    return cost;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Merge-and-shrink heuristic",
        "Note: The parameter space and syntax for the merge-and-shrink "
        "heuristic has changed significantly in August 2011.");
    parser.document_language_support(
        "action costs",
        "supported");
    parser.document_language_support("conditional_effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    // TODO: better documentation what each parameter does
    vector<string> merge_strategies;
    //TODO: it's a bit dangerous that the merge strategies here
    // have to be specified exactly in the same order
    // as in the enum definition. Try to find a way around this,
    // or at least raise an error when the order is wrong.
    merge_strategies.push_back("MERGE_LINEAR_CG_GOAL_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_CG_GOAL_RANDOM");
    merge_strategies.push_back("MERGE_LINEAR_GOAL_CG_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_RANDOM");
    merge_strategies.push_back("MERGE_DFP");
    merge_strategies.push_back("MERGE_LINEAR_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_REVERSE_LEVEL");
    parser.add_enum_option("merge_strategy", merge_strategies,
                           "merge strategy",
                           "MERGE_LINEAR_CG_GOAL_LEVEL");

    parser.add_option<ShrinkStrategy *>(
        "shrink_strategy",
        "shrink strategy; these are not fully documented yet; "
        "try one of the following:",
        "shrink_fh(max_states=50000, max_states_before_merge=50000, shrink_f=high, shrink_h=low)");
    ValueExplanations shrink_value_explanations;
    shrink_value_explanations.push_back(
        make_pair("shrink_fh(max_states=N)",
                  "f-preserving abstractions from the "
                  "Helmert/Haslum/Hoffmann ICAPS 2007 paper "
                  "(called HHH in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Here, N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the default merge strategy "
                  "MERGE_LINEAR_CG_GOAL_LEVEL to match the heuristic "
                  "in the paper."));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=infinity, threshold=1, greedy=true, initialize_by_h=false, group_by_h=false)",
                  "Greedy bisimulation without size bound "
                  "(called M&S-gop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Combine this with the merge strategy "
                  "MERGE_LINEAR_REVERSE_LEVEL to match "
                  "the heuristic in the paper. "));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=N, greedy=false, initialize_by_h=true, group_by_h=true)",
                  "Exact bisimulation with a size limit "
                  "(called DFP-bop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert), "
                  "where N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the merge strategy "
                  "MERGE_LINEAR_REVERSE_LEVEL to match "
                  "the heuristic in the paper."));
    parser.document_values("shrink_strategy", shrink_value_explanations);

    // TODO: Rename option name to "use_label_reduction" to be
    //       consistent with the papers?
    parser.add_option<bool>("reduce_labels", "enable label reduction", "true");
    parser.add_option<bool>("expensive_statistics", "show statistics on \"unique unlabeled edges\" (WARNING: "
                            "these are *very* slow -- check the warning in the output)", "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (parser.dry_run()) {
        return 0;
    } else {
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(opts);
        return result;
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
