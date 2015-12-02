#include "label_reduction.h"

#include "factored_transition_system.h"
#include "labels.h"
#include "transition_system.h"

#include "../equivalence_relation.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../task_proxy.h"
#include "../utilities.h"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

LabelReduction::LabelReduction(const Options &options)
    : lr_before_shrinking(options.get<bool>("before_shrinking")),
      lr_before_merging(options.get<bool>("before_merging")),
      lr_method(LabelReductionMethod(options.get_enum("method"))),
      lr_system_order(LabelReductionSystemOrder(options.get_enum("system_order"))) {
}

bool LabelReduction::initialized() const {
    return !transition_system_order.empty();
}

void LabelReduction::initialize(const TaskProxy &task_proxy) {
    assert(!initialized());

    // Compute the transition system order
    size_t max_transition_system_count = task_proxy.get_variables().size() * 2 - 1;
    transition_system_order.reserve(max_transition_system_count);
    if (lr_system_order == REGULAR
        || lr_system_order == RANDOM) {
        for (size_t i = 0; i < max_transition_system_count; ++i)
            transition_system_order.push_back(i);
        if (lr_system_order == RANDOM) {
            g_rng.shuffle(transition_system_order);
        }
    } else {
        assert(lr_system_order == REVERSE);
        for (size_t i = 0; i < max_transition_system_count; ++i)
            transition_system_order.push_back(max_transition_system_count - 1 - i);
    }
}

void LabelReduction::compute_label_mapping(
    const EquivalenceRelation *relation,
    const FactoredTransitionSystem &fts,
    vector<pair<int, vector<int>>> &label_mapping) {
    const Labels &labels = fts.get_labels();
    int next_new_label_no = labels.get_size();
    int num_labels = 0;
    int num_labels_after_reduction = 0;
    for (BlockListConstIter group_it = relation->begin();
         group_it != relation->end(); ++group_it) {
        const Block &block = *group_it;
        unordered_map<int, vector<int>> equivalent_label_nos;
        for (ElementListConstIter label_it = block.begin();
             label_it != block.end(); ++label_it) {
            assert(*label_it < next_new_label_no);
            int label_no = *label_it;
            if (labels.is_current_label(label_no)) {
                // only consider non-reduced labels
                int cost = labels.get_label_cost(label_no);
                equivalent_label_nos[cost].push_back(label_no);
                ++num_labels;
            }
        }
        for (auto it = equivalent_label_nos.begin();
             it != equivalent_label_nos.end(); ++it) {
            const vector<int> &label_nos = it->second;
            if (label_nos.size() > 1) {
                label_mapping.push_back(make_pair(next_new_label_no, label_nos));
                ++next_new_label_no;
            }
            if (!label_nos.empty()) {
                ++num_labels_after_reduction;
            }
        }
    }
    int number_reduced_labels = num_labels - num_labels_after_reduction;
    if (number_reduced_labels > 0) {
        cout << "Label reduction: "
             << num_labels << " labels, "
             << num_labels_after_reduction << " after reduction"
             << endl;
    }
}

EquivalenceRelation *LabelReduction::compute_combinable_equivalence_relation(
    int ts_index,
    const FactoredTransitionSystem &fts) const {
    /*
      Returns an equivalence relation over labels s.t. l ~ l'
      iff l and l' are locally equivalent in all transition systems
      T' \neq T. (They may or may not be locally equivalent in T.)
    */
    //cout << transition_system.tag() << "compute combinable labels" << endl;

    // create the equivalence relation where all labels are equivalent
    int num_labels = fts.get_num_labels();
    vector<pair<int, int>> annotated_labels;
    annotated_labels.reserve(num_labels);
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        if (fts.get_labels().is_current_label(label_no)) {
            annotated_labels.push_back(make_pair(0, label_no));
        }
    }
    EquivalenceRelation *relation =
        EquivalenceRelation::from_annotated_elements<int>(num_labels, annotated_labels);

    for (int i = 0; i < fts.get_size(); ++i) {
        if (!fts.is_active(i) || i == ts_index) {
            continue;
        }
        const TransitionSystem &ts = fts.get_ts(i);
        for (TSConstIterator group_it = ts.begin();
             group_it != ts.end(); ++group_it) {
            relation->refine(group_it.begin(), group_it.end());
        }
    }
    return relation;
}

void LabelReduction::reduce(pair<int, int> next_merge,
                            FactoredTransitionSystem &fts) {
    assert(initialized());
    assert(reduce_before_shrinking() || reduce_before_merging());
    int num_transition_systems = fts.get_size();

    if (lr_method == TWO_TRANSITION_SYSTEMS) {
        /* Note:
           We compute the combinable relation for labels for the two transition systems
           in the order given by the merge strategy. We conducted experiments
           testing the impact of always starting with the larger transitions system
           (in terms of variables) or with the smaller transition system and found
           no significant differences.
         */
        assert(fts.is_active(next_merge.first));
        assert(fts.is_active(next_merge.second));

        EquivalenceRelation *relation = compute_combinable_equivalence_relation(
            next_merge.first,
            fts);
        vector<pair<int, vector<int>>> label_mapping;
        compute_label_mapping(relation, fts, label_mapping);
        if (!label_mapping.empty()) {
            fts.apply_label_reduction(label_mapping,
                                       next_merge.first);
        }
        delete relation;
        relation = 0;
        release_vector_memory(label_mapping);

        relation = compute_combinable_equivalence_relation(
            next_merge.second,
            fts);
        compute_label_mapping(relation, fts, label_mapping);
        if (!label_mapping.empty()) {
            fts.apply_label_reduction(label_mapping,
                                       next_merge.second);
        }
        delete relation;
        return;
    }

    // Make sure that we start with an index not ouf of range for
    // all_transition_systems
    size_t tso_index = 0;
    assert(!transition_system_order.empty());
    while (transition_system_order[tso_index] >= num_transition_systems) {
        ++tso_index;
        assert(in_bounds(tso_index, transition_system_order));
    }

    int max_iterations;
    if (lr_method == ALL_TRANSITION_SYSTEMS) {
        max_iterations = num_transition_systems;
    } else if (lr_method == ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT) {
        max_iterations = numeric_limits<int>::max();
    } else {
        ABORT("unknown label reduction method");
    }

    int num_unsuccessful_iterations = 0;

    for (int i = 0; i < max_iterations; ++i) {
        int ts_index = transition_system_order[tso_index];

        vector<pair<int, vector<int>>> label_mapping;
        if (fts.is_active(ts_index)) {
            EquivalenceRelation *relation =
                compute_combinable_equivalence_relation(ts_index,
                                                        fts);
            compute_label_mapping(relation, fts, label_mapping);
            delete relation;
        }

        if (label_mapping.empty()) {
            // Even if the transition system has been removed, we need to count
            // it as unsuccessful iterations (the size of the vector matters).
            ++num_unsuccessful_iterations;
        } else {
            num_unsuccessful_iterations = 0;
            fts.apply_label_reduction(label_mapping, ts_index);
        }
        if (num_unsuccessful_iterations == num_transition_systems - 1)
            break;

        ++tso_index;
        if (tso_index == transition_system_order.size()) {
            tso_index = 0;
        }
        while (transition_system_order[tso_index] >= num_transition_systems) {
            ++tso_index;
            if (tso_index == transition_system_order.size()) {
                tso_index = 0;
            }
        }
    }
}

void LabelReduction::dump_options() const {
    assert(initialized());
    cout << "Label reduction options:" << endl;
    cout << "Before merging: "
         << (lr_before_merging ? "enabled" : "disabled") << endl;
    cout << "Before shrinking: "
         << (lr_before_shrinking ? "enabled" : "disabled") << endl;
    cout << "Method: ";
    switch (lr_method) {
    case TWO_TRANSITION_SYSTEMS:
        cout << "two transition systems (which will be merged next)";
        break;
    case ALL_TRANSITION_SYSTEMS:
        cout << "all transition systems";
        break;
    case ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT:
        cout << "all transition systems with fixpoint computation";
        break;
    }
    cout << endl;
    if (lr_method == ALL_TRANSITION_SYSTEMS ||
        lr_method == ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT) {
        cout << "System order: ";
        switch (lr_system_order) {
        case REGULAR:
            cout << "regular";
            break;
        case REVERSE:
            cout << "reversed";
            break;
        case RANDOM:
            cout << "random";
            break;
        }
        cout << endl;
    }
}

static shared_ptr<LabelReduction>_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Generalized label reduction",
        "This class implements the generalized label reduction described "
        "in the following paper:\n\n"
        " * Silvan Sievers, Martin Wehrle, and Malte Helmert.<<BR>>\n"
        " [Generalized Label Reduction for Merge-and-Shrink Heuristics "
        "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf].<<BR>>\n "
        "In //Proceedings of the 28th AAAI Conference on Artificial "
        "Intelligence (AAAI 2014)//, pp. 2358-2366. AAAI Press 2014.");
    parser.add_option<bool>("before_shrinking",
                            "apply label reduction before shrinking");
    parser.add_option<bool>("before_merging",
                            "apply label reduction before merging");

    vector<string> label_reduction_method;
    vector<string> label_reduction_method_doc;
    label_reduction_method.push_back("TWO_TRANSITION_SYSTEMS");
    label_reduction_method_doc.push_back(
        "compute the 'combinable relation' only for the two transition "
        "systems being merged next");
    label_reduction_method.push_back("ALL_TRANSITION_SYSTEMS");
    label_reduction_method_doc.push_back(
        "compute the 'combinable relation' for labels once for every "
        "transition  system and reduce labels");
    label_reduction_method.push_back("ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT");
    label_reduction_method_doc.push_back(
        "keep computing the 'combinable relation' for labels iteratively "
        "for all transition systems until no more labels can be reduced");
    parser.add_enum_option("method",
                           label_reduction_method,
                           "Label reduction method. See the AAAI14 paper by "
                           "Sievers et al. for explanation of the default label "
                           "reduction method and the 'combinable relation' ."
                           "Also note that you must set at least one of the "
                           "options reduce_labels_before_shrinking or "
                           "reduce_labels_before_merging in order to use "
                           "the chosen label reduction configuration.",
                           "ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT",
                           label_reduction_method_doc);

    vector<string> label_reduction_system_order;
    vector<string> label_reduction_system_order_doc;
    label_reduction_system_order.push_back("REGULAR");
    label_reduction_system_order_doc.push_back(
        "transition systems are considered in the FD given order for "
        "atomic transition systems and in the order of their creation "
        "for composite transition system.");
    label_reduction_system_order.push_back("REVERSE");
    label_reduction_system_order_doc.push_back(
        "inverse of REGULAR");
    label_reduction_system_order.push_back("RANDOM");
    label_reduction_system_order_doc.push_back(
        "random order");
    parser.add_enum_option("system_order",
                           label_reduction_system_order,
                           "Order of transition systems for the label reduction "
                           "methods that iterate over the set of all transition "
                           "systems. Only useful for the choices "
                           "all_transition_systems and "
                           "all_transition_systems_with_fixpoint for the option "
                           "label_reduction_method.",
                           "RANDOM",
                           label_reduction_system_order_doc);

    Options opts = parser.parse();

    if (parser.dry_run()) {
        bool lr_before_shrinking = opts.get<bool>("before_shrinking");
        bool lr_before_merging = opts.get<bool>("before_merging");
        if (!lr_before_shrinking && !lr_before_merging) {
            cerr << "Please turn on at least one of the options "
                 << "before_shrinking or before_merging!" << endl;
            exit_with(EXIT_INPUT_ERROR);
        }
        return 0;
    } else {
        return make_shared<LabelReduction>(opts);
    }
}

static PluginTypePlugin<LabelReduction> _type_plugin(
    "Label reduction",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");

static PluginShared<LabelReduction> _plugin("exact", _parse);

