#include "label_reduction.h"

#include "factored_transition_system.h"
#include "labels.h"
#include "transition_system.h"
#include "types.h"

#include "../task_proxy.h"

#include "../algorithms/equivalence_relation.h"
#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/system.h"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
LabelReduction::LabelReduction(const plugins::Options &options)
    : lr_before_shrinking(options.get<bool>("before_shrinking")),
      lr_before_merging(options.get<bool>("before_merging")),
      lr_method(options.get<LabelReductionMethod>("method")),
      lr_system_order(options.get<LabelReductionSystemOrder>("system_order")),
      rng(utils::parse_rng_from_options(options)) {
}

bool LabelReduction::initialized() const {
    return !transition_system_order.empty();
}

void LabelReduction::initialize(const TaskProxy &task_proxy) {
    assert(!initialized());

    // Compute the transition system order.
    size_t max_transition_system_count = task_proxy.get_variables().size() * 2 - 1;
    transition_system_order.reserve(max_transition_system_count);
    if (lr_system_order == LabelReductionSystemOrder::REGULAR
        || lr_system_order == LabelReductionSystemOrder::RANDOM) {
        for (size_t i = 0; i < max_transition_system_count; ++i)
            transition_system_order.push_back(i);
        if (lr_system_order == LabelReductionSystemOrder::RANDOM) {
            rng->shuffle(transition_system_order);
        }
    } else {
        assert(lr_system_order == LabelReductionSystemOrder::REVERSE);
        for (size_t i = 0; i < max_transition_system_count; ++i)
            transition_system_order.push_back(max_transition_system_count - 1 - i);
    }
}

void LabelReduction::compute_label_mapping(
    const equivalence_relation::EquivalenceRelation &relation,
    const FactoredTransitionSystem &fts,
    vector<pair<int, vector<int>>> &label_mapping,
    utils::LogProxy &log) const {
    const Labels &labels = fts.get_labels();
    int next_new_label = labels.get_num_total_labels();
    int num_labels = 0;
    int num_labels_after_reduction = 0;
    for (const equivalence_relation::Block &block : relation) {
        unordered_map<int, vector<int>> cost_to_equivalent_labels;
        for (int label : block) {
            assert(label < next_new_label);
            int cost = labels.get_label_cost(label);
            cost_to_equivalent_labels[cost].push_back(label);
            ++num_labels;
        }
        for (auto &entry : cost_to_equivalent_labels) {
            vector<int> &equivalent_labels = entry.second;
            if (equivalent_labels.size() > 1) {
                // Labels have to be sorted for LocalLabelInfo.
                sort(equivalent_labels.begin(), equivalent_labels.end());
                if (log.is_at_least_debug()) {
                    log << "Reducing labels "
                        << equivalent_labels << " to " << next_new_label << endl;
                }
                label_mapping.push_back(make_pair(next_new_label, equivalent_labels));
                ++next_new_label;
            }
            if (!equivalent_labels.empty()) {
                ++num_labels_after_reduction;
            }
        }
    }
    int number_reduced_labels = num_labels - num_labels_after_reduction;
    if (log.is_at_least_verbose() && number_reduced_labels > 0) {
        log << "Label reduction: "
            << num_labels << " labels, "
            << num_labels_after_reduction << " after reduction"
            << endl;
    }
}

equivalence_relation::EquivalenceRelation
LabelReduction::compute_combinable_equivalence_relation(
    int ts_index,
    const FactoredTransitionSystem &fts) const {
    /*
      Returns an equivalence relation over labels s.t. l ~ l'
      iff l and l' are locally equivalent in all transition systems
      T' \neq T. (They may or may not be locally equivalent in T.)
    */

    // Create the equivalence relation where all labels are equivalent.
    const Labels &labels = fts.get_labels();
    int num_labels = labels.get_num_active_labels();
    vector<int> all_active_labels;
    all_active_labels.reserve(num_labels);
    for (int label : labels) {
        all_active_labels.push_back(label);
    }
    equivalence_relation::EquivalenceRelation relation(all_active_labels);

    for (int index : fts) {
        if (index != ts_index) {
            const TransitionSystem &ts = fts.get_transition_system(index);
            for (const LocalLabelInfo &local_label_info : ts) {
                relation.refine(local_label_info.get_label_group());
            }
        }
    }
    return relation;
}

bool LabelReduction::reduce(
    const pair<int, int> &next_merge,
    FactoredTransitionSystem &fts,
    utils::LogProxy &log) const {
    assert(initialized());
    assert(reduce_before_shrinking() || reduce_before_merging());
    int num_transition_systems = fts.get_size();

    if (lr_method == LabelReductionMethod::TWO_TRANSITION_SYSTEMS) {
        /*
           Note:
           We compute the combinable relation for labels for the two transition
           systems in the order given by the merge strategy. We conducted
           experiments testing the impact of always starting with the larger
           transitions system (in terms of variables) or with the smaller
           transition system and found no significant differences.
         */
        assert(fts.is_active(next_merge.first));
        assert(fts.is_active(next_merge.second));

        bool reduced = false;
        equivalence_relation::EquivalenceRelation relation =
            compute_combinable_equivalence_relation(next_merge.first, fts);
        vector<pair<int, vector<int>>> label_mapping;
        compute_label_mapping(relation, fts, label_mapping, log);
        if (!label_mapping.empty()) {
            fts.apply_label_mapping(label_mapping, next_merge.first);
            reduced = true;
        }
        utils::release_vector_memory(label_mapping);

        relation = compute_combinable_equivalence_relation(
            next_merge.second,
            fts);
        compute_label_mapping(relation, fts, label_mapping, log);
        if (!label_mapping.empty()) {
            fts.apply_label_mapping(label_mapping, next_merge.second);
            reduced = true;
        }
        return reduced;
    }

    /* Make sure that we start with an index not ouf of range for
       all_transition_systems. */
    size_t tso_index = 0;
    assert(!transition_system_order.empty());
    while (transition_system_order[tso_index] >= num_transition_systems) {
        ++tso_index;
        assert(utils::in_bounds(tso_index, transition_system_order));
    }

    int max_iterations;
    if (lr_method == LabelReductionMethod::ALL_TRANSITION_SYSTEMS) {
        max_iterations = num_transition_systems;
    } else if (lr_method == LabelReductionMethod::ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT) {
        max_iterations = INF;
    } else {
        ABORT("unknown label reduction method");
    }

    int num_unsuccessful_iterations = 0;

    bool reduced = false;
    /*
      If using ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT, this loop stops under
      the following conditions: if there are no combinable labels for all
      transition systems, we have num_unsuccessful_iterations =
      num_transition_systems and break the loop.

      Whenever there is a transition system for which we reduce labels, we
      reset the counter num_unsuccessful_iterations to 1 (not to 0!) because
      we only need to consider all remaining transitions systems, but not the
      one itself again.
    */
    for (int i = 0; i < max_iterations; ++i) {
        int ts_index = transition_system_order[tso_index];

        vector<pair<int, vector<int>>> label_mapping;
        if (fts.is_active(ts_index)) {
            equivalence_relation::EquivalenceRelation relation =
                compute_combinable_equivalence_relation(ts_index, fts);
            compute_label_mapping(relation, fts, label_mapping, log);
        }

        if (label_mapping.empty()) {
            /*
              Even if the index is inactive, we need to count it as
              unsuccessful iterations, because the number of indices, i.e.
              the size of the vector in the factored transition system
              matters.
            */
            ++num_unsuccessful_iterations;
        } else {
            reduced = true;
            // See comment for the loop and its exit conditions.
            num_unsuccessful_iterations = 1;
            fts.apply_label_mapping(label_mapping, ts_index);
        }
        if (num_unsuccessful_iterations == num_transition_systems) {
            // See comment for the loop and its exit conditions.
            break;
        }

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
    return reduced;
}

void LabelReduction::dump_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Label reduction options:" << endl;
        log << "Before merging: "
            << (lr_before_merging ? "enabled" : "disabled") << endl;
        log << "Before shrinking: "
            << (lr_before_shrinking ? "enabled" : "disabled") << endl;
        log << "Method: ";
        switch (lr_method) {
        case LabelReductionMethod::TWO_TRANSITION_SYSTEMS:
            log << "two transition systems (which will be merged next)";
            break;
        case LabelReductionMethod::ALL_TRANSITION_SYSTEMS:
            log << "all transition systems";
            break;
        case LabelReductionMethod::ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT:
            log << "all transition systems with fixpoint computation";
            break;
        }
        log << endl;
        if (lr_method == LabelReductionMethod::ALL_TRANSITION_SYSTEMS ||
            lr_method == LabelReductionMethod::ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT) {
            log << "System order: ";
            switch (lr_system_order) {
            case LabelReductionSystemOrder::REGULAR:
                log << "regular";
                break;
            case LabelReductionSystemOrder::REVERSE:
                log << "reversed";
                break;
            case LabelReductionSystemOrder::RANDOM:
                log << "random";
                break;
            }
            log << endl;
        }
    }
}

class LabelReductionFeature : public plugins::TypedFeature<LabelReduction, LabelReduction> {
public:
    LabelReductionFeature() : TypedFeature("exact") {
        document_title("Exact generalized label reduction");
        document_synopsis(
            "This class implements the exact generalized label reduction "
            "described in the following paper:" +
            utils::format_conference_reference(
                {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
                "Generalized Label Reduction for Merge-and-Shrink Heuristics",
                "https://ai.dmi.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
                "Proceedings of the 28th AAAI Conference on Artificial"
                " Intelligence (AAAI 2014)",
                "2358-2366",
                "AAAI Press",
                "2014"));

        add_option<bool>(
            "before_shrinking",
            "apply label reduction before shrinking");
        add_option<bool>(
            "before_merging",
            "apply label reduction before merging");

        add_option<LabelReductionMethod>(
            "method",
            "Label reduction method. See the AAAI14 paper by "
            "Sievers et al. for explanation of the default label "
            "reduction method and the 'combinable relation' ."
            "Also note that you must set at least one of the "
            "options reduce_labels_before_shrinking or "
            "reduce_labels_before_merging in order to use "
            "the chosen label reduction configuration.",
            "all_transition_systems_with_fixpoint");

        add_option<LabelReductionSystemOrder>(
            "system_order",
            "Order of transition systems for the label reduction "
            "methods that iterate over the set of all transition "
            "systems. Only useful for the choices "
            "all_transition_systems and "
            "all_transition_systems_with_fixpoint for the option "
            "label_reduction_method.",
            "random");
        // Add random_seed option.
        utils::add_rng_options(*this);
    }

    virtual shared_ptr<LabelReduction> create_component(const plugins::Options &options, const utils::Context &context) const override {
        bool lr_before_shrinking = options.get<bool>("before_shrinking");
        bool lr_before_merging = options.get<bool>("before_merging");
        if (!lr_before_shrinking && !lr_before_merging) {
            context.error(
                "Please turn on at least one of the options "
                "before_shrinking or before_merging!");
        }
        return make_shared<LabelReduction>(options);
    }
};

static plugins::FeaturePlugin<LabelReductionFeature> _plugin;

static class LabelReductionCategoryPlugin : public plugins::TypedCategoryPlugin<LabelReduction> {
public:
    LabelReductionCategoryPlugin() : TypedCategoryPlugin("LabelReduction") {
        document_synopsis("This page describes the current single 'option' for label reduction.");
    }
}
_category_plugin;

static plugins::TypedEnumPlugin<LabelReductionMethod> _label_reduction_method_enum_plugin({
        {"two_transition_systems",
         "compute the 'combinable relation' only for the two transition "
         "systems being merged next"},
        {"all_transition_systems",
         "compute the 'combinable relation' for labels once for every "
         "transition system and reduce labels"},
        {"all_transition_systems_with_fixpoint",
         "keep computing the 'combinable relation' for labels iteratively "
         "for all transition systems until no more labels can be reduced"}
    });

static plugins::TypedEnumPlugin<LabelReductionSystemOrder> _label_reduction_system_order_enum_plugin({
        {"regular",
         "transition systems are considered in the order given in the planner "
         "input if atomic and in the order of their creation if composite."},
        {"reverse",
         "inverse of regular"},
        {"random",
         "random order"}
    });
}
