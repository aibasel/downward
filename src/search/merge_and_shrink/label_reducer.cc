#include "label_reducer.h"

#include "transition_system.h"
#include "label.h"

#include "../equivalence_relation.h"
#include "../globals.h"
#include "../option_parser.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

using namespace std;

LabelReducer::LabelReducer(const Options &options)
    : label_reduction_method(LabelReductionMethod(options.get_enum("label_reduction_method"))),
      label_reduction_system_order(LabelReductionSystemOrder(options.get_enum("label_reduction_system_order"))) {
    size_t max_no_systems = g_variable_domain.size() * 2 - 1;
    transition_system_order.reserve(max_no_systems);
    if (label_reduction_system_order == REGULAR
        || label_reduction_system_order == RANDOM) {
        for (size_t i = 0; i < max_no_systems; ++i)
            transition_system_order.push_back(i);
        if (label_reduction_system_order == RANDOM) {
            random_shuffle(transition_system_order.begin(), transition_system_order.end());
        }
    } else {
        assert(label_reduction_system_order == REVERSE);
        for (size_t i = 0; i < max_no_systems; ++i)
            transition_system_order.push_back(max_no_systems - 1 - i);
    }
}

void LabelReducer::reduce_labels(pair<int, int> next_merge,
                                 const vector<TransitionSystem *> &all_transition_systems,
                                 std::vector<Label *> &labels) const {
    int num_transition_systems = all_transition_systems.size();

    if (label_reduction_method == NONE) {
        return;
    }

    if (label_reduction_method == TWO_TRANSITION_SYSTEMS) {
        /* Note:
           We compute the combinable relation for labels for the two transition systems
           in the order given by the merge strategy. We conducted experiments
           testing the impact of always starting with the larger transitions system
           (in terms of variables) or with the smaller transition system and found
           no significant differences.
         */
        assert(all_transition_systems[next_merge.first]);
        assert(all_transition_systems[next_merge.second]);

        vector<EquivalenceRelation *> local_equivalence_relations(
            all_transition_systems.size(), 0);

        EquivalenceRelation *relation = compute_outside_equivalence(
            next_merge.first, all_transition_systems,
            labels, local_equivalence_relations);
        reduce_exactly(relation, labels);
        delete relation;

        relation = compute_outside_equivalence(
            next_merge.second, all_transition_systems,
            labels, local_equivalence_relations);
        reduce_exactly(relation, labels);
        delete relation;

        for (size_t i = 0; i < local_equivalence_relations.size(); ++i)
            delete local_equivalence_relations[i];
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
    if (label_reduction_method == ALL_TRANSITION_SYSTEMS) {
        max_iterations = num_transition_systems;
    } else if (label_reduction_method == ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT) {
        max_iterations = numeric_limits<int>::max();
    } else {
        ABORT("unknown label reduction method");
    }

    int num_unsuccessful_iterations = 0;
    vector<EquivalenceRelation *> local_equivalence_relations(
        all_transition_systems.size(), 0);

    for (int i = 0; i < max_iterations; ++i) {
        int ts_index = transition_system_order[tso_index];
        TransitionSystem *current_transition_system = all_transition_systems[ts_index];

        bool have_reduced = false;
        if (current_transition_system != 0) {
            EquivalenceRelation *relation = compute_outside_equivalence(
                ts_index, all_transition_systems,
                labels, local_equivalence_relations);
            have_reduced = reduce_exactly(relation, labels);
            delete relation;
        }

        if (have_reduced) {
            num_unsuccessful_iterations = 0;
        } else {
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

    for (size_t i = 0; i < local_equivalence_relations.size(); ++i)
        delete local_equivalence_relations[i];
}

EquivalenceRelation *LabelReducer::compute_outside_equivalence(
    int ts_index,
    const vector<TransitionSystem *> &all_transition_systems,
    const vector<Label *> &labels,
    vector<EquivalenceRelation *> &local_equivalence_relations) const {
    /*Returns an equivalence relation over labels s.t. l ~ l'
    iff l and l' are locally equivalent in all transition systems
    T' \neq T. (They may or may not be locally equivalent in T.) */
    TransitionSystem *transition_system = all_transition_systems[ts_index];
    assert(transition_system);
    //cout << transition_system->tag() << "compute combinable labels" << endl;

    // We always normalize the "starting" transition system and delete the cached
    // local equivalence relation (if exists) because this does not happen
    // in the refinement loop below.
    transition_system->normalize();
    if (local_equivalence_relations[ts_index]) {
        delete local_equivalence_relations[ts_index];
        local_equivalence_relations[ts_index] = 0;
    }

    // create the equivalence relation where all labels are equivalent
    int num_labels = labels.size();
    vector<pair<int, int> > annotated_labels;
    annotated_labels.reserve(num_labels);
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        const Label *label = labels[label_no];
        assert(label->get_id() == label_no);
        if (!label->is_reduced()) {
            // only consider non-reduced labels
            annotated_labels.push_back(make_pair(0, label_no));
        }
    }
    EquivalenceRelation *relation = EquivalenceRelation::from_annotated_elements<int>(num_labels, annotated_labels);

    for (size_t i = 0; i < all_transition_systems.size(); ++i) {
        TransitionSystem *ts = all_transition_systems[i];
        if (!ts || ts == transition_system) {
            continue;
        }
        if (!ts->is_normalized()) {
            ts->normalize();
            if (local_equivalence_relations[i]) {
                delete local_equivalence_relations[i];
                local_equivalence_relations[i] = 0;
            }
        }
        //cout << transition_system->tag();
        if (!local_equivalence_relations[i]) {
            //cout << "compute local equivalence relation" << endl;
            local_equivalence_relations[i] = ts->compute_local_equivalence_relation();
        } else {
            //cout << "use cached local equivalence relation" << endl;
            assert(ts->is_normalized());
        }
        relation->refine(*local_equivalence_relations[i]);
    }
    return relation;
}

bool LabelReducer::reduce_exactly(const EquivalenceRelation *relation, std::vector<Label *> &labels) const {
    int num_labels = 0;
    int num_labels_after_reduction = 0;
    for (BlockListConstIter it = relation->begin(); it != relation->end(); ++it) {
        const Block &block = *it;
        vector<Label *> equivalent_labels;
        for (ElementListConstIter jt = block.begin(); jt != block.end(); ++jt) {
            assert(*jt < static_cast<int>(labels.size()));
            Label *label = labels[*jt];
            if (!label->is_reduced()) {
                // only consider non-reduced labels
                equivalent_labels.push_back(label);
                ++num_labels;
            }
        }
        if (equivalent_labels.size() > 1) {
            Label *new_label = new CompositeLabel(labels.size(), equivalent_labels);
            labels.push_back(new_label);
        }
        if (!equivalent_labels.empty()) {
            ++num_labels_after_reduction;
        }
    }
    int number_reduced_labels = num_labels - num_labels_after_reduction;
    if (number_reduced_labels > 0) {
        cout << "Label reduction: "
             << num_labels << " labels, "
             << num_labels_after_reduction << " after reduction"
             << endl;
    }
    return number_reduced_labels;
}

void LabelReducer::dump_options() const {
    cout << "Label reduction: ";
    switch (label_reduction_method) {
    case NONE:
        cout << "disabled";
        break;
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
    if (label_reduction_method == ALL_TRANSITION_SYSTEMS ||
        label_reduction_method == ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT) {
        cout << "System order: ";
        switch (label_reduction_system_order) {
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
