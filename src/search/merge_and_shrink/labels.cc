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
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>

using namespace std;

class Label {
    /*
      This class implements labels as used by merge-and-shrink transition systems.
      Labels are opaque tokens that have an associated cost.
    */
    int cost;
public:
    explicit Label(int cost_)
        : cost(cost_) {
    }
    ~Label() {}
    int get_cost() const {
        return cost;
    }
};

Labels::Labels(const Options &options)
    : max_size(-1),
      lr_before_shrinking(options.get<bool>("before_shrinking")),
      lr_before_merging(options.get<bool>("before_merging")),
      lr_method(LabelReductionMethod(options.get_enum("method"))),
      lr_system_order(LabelReductionSystemOrder(options.get_enum("system_order"))) {
}

bool Labels::initialized() const {
    return !transition_system_order.empty();
}

void Labels::initialize(const TaskProxy &task_proxy) {
    assert(!initialized());

    // Reserve memory for labels
    size_t num_ops = task_proxy.get_operators().size();
    max_size = 0;
    if (num_ops > 0) {
        max_size = num_ops * 2 - 1;
        labels.reserve(max_size);
    }

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

void Labels::add_label(int cost) {
    assert(initialized());
    labels.push_back(new Label(cost));
}

void Labels::notify_transition_systems(
    int ts_index,
    const vector<TransitionSystem *> &all_transition_systems,
    const vector<pair<int, vector<int> > > &label_mapping) const {
    for (size_t i = 0; i < all_transition_systems.size(); ++i) {
        if (all_transition_systems[i]) {
            all_transition_systems[i]->apply_label_reduction(label_mapping,
                                                             static_cast<int>(i) != ts_index);
        }
    }
}

bool Labels::apply_label_reduction(const EquivalenceRelation *relation,
                                   vector<pair<int, vector<int> > > &label_mapping) {
    int num_labels = 0;
    int num_labels_after_reduction = 0;
    for (BlockListConstIter group_it = relation->begin();
         group_it != relation->end(); ++group_it) {
        const Block &block = *group_it;
        unordered_map<int, vector<int> > equivalent_label_nos;
        for (ElementListConstIter label_it = block.begin();
             label_it != block.end(); ++label_it) {
            assert(*label_it < static_cast<int>(labels.size()));
            int label_no = *label_it;
            Label *label = labels[label_no];
            if (label) {
                // only consider non-reduced labels
                int cost = label->get_cost();
                equivalent_label_nos[cost].push_back(label_no);
                ++num_labels;
            }
        }
        for (auto it = equivalent_label_nos.begin();
             it != equivalent_label_nos.end(); ++it) {
            const vector<int> &label_nos = it->second;
            if (label_nos.size() > 1) {
                int new_label_no = labels.size();
                Label *new_label = new Label(labels[label_nos[0]]->get_cost());
                labels.push_back(new_label);
                for (size_t i = 0; i < label_nos.size(); ++i) {
                    int old_label_no = label_nos[i];
                    delete labels[old_label_no];
                    labels[old_label_no] = 0;
                }
                label_mapping.push_back(make_pair(new_label_no, label_nos));
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
    return number_reduced_labels > 0;
}

EquivalenceRelation *Labels::compute_combinable_equivalence_relation(
    int ts_index,
    const vector<TransitionSystem *> &all_transition_systems) const {
    /*
      Returns an equivalence relation over labels s.t. l ~ l'
      iff l and l' are locally equivalent in all transition systems
      T' \neq T. (They may or may not be locally equivalent in T.)
    */
    TransitionSystem *fixed_transition_system = all_transition_systems[ts_index];
    assert(fixed_transition_system);
    //cout << transition_system->tag() << "compute combinable labels" << endl;

    // create the equivalence relation where all labels are equivalent
    int num_labels = labels.size();
    vector<pair<int, int> > annotated_labels;
    annotated_labels.reserve(num_labels);
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        if (labels[label_no]) {
            annotated_labels.push_back(make_pair(0, label_no));
        }
    }
    EquivalenceRelation *relation =
        EquivalenceRelation::from_annotated_elements<int>(num_labels, annotated_labels);

    for (size_t i = 0; i < all_transition_systems.size(); ++i) {
        TransitionSystem *ts = all_transition_systems[i];
        if (!ts || ts == fixed_transition_system) {
            continue;
        }
        for (TSConstIterator group_it = ts->begin();
             group_it != ts->end(); ++group_it) {
            relation->refine(group_it.begin(), group_it.end());
        }
    }
    return relation;
}

void Labels::reduce(pair<int, int> next_merge,
                    const vector<TransitionSystem *> &all_transition_systems) {
    assert(initialized());
    assert(reduce_before_shrinking() || reduce_before_merging());
    int num_transition_systems = all_transition_systems.size();

    if (lr_method == TWO_TRANSITION_SYSTEMS) {
        /* Note:
           We compute the combinable relation for labels for the two transition systems
           in the order given by the merge strategy. We conducted experiments
           testing the impact of always starting with the larger transitions system
           (in terms of variables) or with the smaller transition system and found
           no significant differences.
         */
        assert(all_transition_systems[next_merge.first]);
        assert(all_transition_systems[next_merge.second]);

        EquivalenceRelation *relation = compute_combinable_equivalence_relation(
            next_merge.first,
            all_transition_systems);
        vector<pair<int, vector<int> > > label_mapping;
        bool have_reduced = apply_label_reduction(relation, label_mapping);
        if (have_reduced) {
            notify_transition_systems(next_merge.first,
                                      all_transition_systems,
                                      label_mapping);
        }
        delete relation;
        relation = 0;
        release_vector_memory(label_mapping);

        relation = compute_combinable_equivalence_relation(
            next_merge.second,
            all_transition_systems);
        have_reduced = apply_label_reduction(relation, label_mapping);
        if (have_reduced) {
            notify_transition_systems(next_merge.second,
                                      all_transition_systems,
                                      label_mapping);
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
        TransitionSystem *current_transition_system = all_transition_systems[ts_index];

        bool have_reduced = false;
        vector<pair<int, vector<int> > > label_mapping;
        if (current_transition_system != 0) {
            EquivalenceRelation *relation =
                compute_combinable_equivalence_relation(ts_index,
                                                        all_transition_systems);
            have_reduced = apply_label_reduction(relation, label_mapping);
            delete relation;
        }

        if (have_reduced) {
            num_unsuccessful_iterations = 0;
            notify_transition_systems(ts_index,
                                      all_transition_systems,
                                      label_mapping);
        } else {
            // Even if the transition system has been removed, we need to count
            // it as unsuccessful iterations (the size of the vector matters).
            ++num_unsuccessful_iterations;
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

bool Labels::is_current_label(int label_no) const {
    assert(initialized());
    assert(in_bounds(label_no, labels));
    return labels[label_no];
}

int Labels::get_label_cost(int label_no) const {
    assert(initialized());
    assert(labels[label_no]);
    return labels[label_no]->get_cost();
}

void Labels::dump_labels() const {
    assert(initialized());
    cout << "active labels:" << endl;
    for (size_t label_no = 0; label_no < labels.size(); ++label_no) {
        if (labels[label_no]) {
            cout << "label " << label_no << ", cost " << labels[label_no]->get_cost() << endl;
        }
    }
}

void Labels::dump_options() const {
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

static shared_ptr<Labels>_parse(OptionParser &parser) {
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
        return 0;
    } else {
        return make_shared<Labels>(opts);
    }
}

static PluginShared<Labels> _plugin("label_reduction", _parse);
