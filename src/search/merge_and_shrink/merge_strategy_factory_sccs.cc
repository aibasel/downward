#include "merge_strategy_factory_sccs.h"

#include "merge_strategy_sccs.h"
#include "merge_selector.h"
#include "merge_tree_factory.h"
#include "transition_system.h"

#include "../task_proxy.h"

#include "../algorithms/sccs.h"
#include "../plugins/plugin.h"
#include "../task_utils/causal_graph.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
static bool compare_sccs_increasing(const vector<int> &lhs, const vector<int> &rhs) {
    return lhs.size() < rhs.size();
}

static bool compare_sccs_decreasing(const vector<int> &lhs, const vector<int> &rhs) {
    return lhs.size() > rhs.size();
}

MergeStrategyFactorySCCs::MergeStrategyFactorySCCs(
    const OrderOfSCCs &order_of_sccs,
    const shared_ptr<MergeSelector> &merge_selector,
    utils::Verbosity verbosity)
    : MergeStrategyFactory(verbosity),
      order_of_sccs(order_of_sccs),
      merge_selector(merge_selector) {
}

unique_ptr<MergeStrategy> MergeStrategyFactorySCCs::compute_merge_strategy(
    const TaskProxy &task_proxy, const FactoredTransitionSystem &fts) {
    VariablesProxy vars = task_proxy.get_variables();
    int num_vars = vars.size();

    // Compute SCCs of the causal graph.
    vector<vector<int>> cg;
    cg.reserve(num_vars);
    for (VariableProxy var : vars) {
        const vector<int> &successors =
            task_proxy.get_causal_graph().get_successors(var.get_id());
        cg.push_back(successors);
    }
    vector<vector<int>> sccs(sccs::compute_maximal_sccs(cg));

    // Put the SCCs in the desired order.
    switch (order_of_sccs) {
    case OrderOfSCCs::TOPOLOGICAL:
        // SCCs are computed in topological order.
        break;
    case OrderOfSCCs::REVERSE_TOPOLOGICAL:
        // SCCs are computed in topological order.
        reverse(sccs.begin(), sccs.end());
        break;
    case OrderOfSCCs::DECREASING:
        sort(sccs.begin(), sccs.end(), compare_sccs_decreasing);
        break;
    case OrderOfSCCs::INCREASING:
        sort(sccs.begin(), sccs.end(), compare_sccs_increasing);
        break;
    }

    if (log.is_at_least_normal()) {
        log << "SCCs of the causal graph:" << endl;
    }
    vector<vector<int>> non_singleton_cg_sccs;
    for (const vector<int> &scc : sccs) {
        if (log.is_at_least_normal()) {
            log << scc << endl;
        }
        int scc_size = scc.size();
        if (scc_size != 1) {
            non_singleton_cg_sccs.push_back(scc);
        }
    }
    if (log.is_at_least_normal() && sccs.size() == 1) {
        log << "Only one single SCC" << endl;
    }
    if (log.is_at_least_normal() && static_cast<int>(sccs.size()) == num_vars) {
        log << "Only singleton SCCs" << endl;
        assert(non_singleton_cg_sccs.empty());
    }

    if (merge_selector) {
        merge_selector->initialize(task_proxy);
    }

    return make_unique<MergeStrategySCCs>(
        fts,
        merge_selector,
        move(non_singleton_cg_sccs));
}

bool MergeStrategyFactorySCCs::requires_init_distances() const {
    return merge_selector->requires_init_distances();
}

bool MergeStrategyFactorySCCs::requires_goal_distances() const {
    return merge_selector->requires_goal_distances();
}

void MergeStrategyFactorySCCs::dump_strategy_specific_options() const {
    if (log.is_at_least_normal()) {
        log << "Merge order of sccs: ";
        switch (order_of_sccs) {
        case OrderOfSCCs::TOPOLOGICAL:
            log << "topological";
            break;
        case OrderOfSCCs::REVERSE_TOPOLOGICAL:
            log << "reverse topological";
            break;
        case OrderOfSCCs::DECREASING:
            log << "decreasing";
            break;
        case OrderOfSCCs::INCREASING:
            log << "increasing";
            break;
        }
        log << endl;

        log << "Merge strategy for merging within sccs: " << endl;
        merge_selector->dump_options(log);
    }
}

string MergeStrategyFactorySCCs::name() const {
    return "sccs";
}

class MergeStrategyFactorySCCsFeature
    : public plugins::TypedFeature<MergeStrategyFactory, MergeStrategyFactorySCCs> {
public:
    MergeStrategyFactorySCCsFeature() : TypedFeature("merge_sccs") {
        document_title("Merge strategy SSCs");
        document_synopsis(
            "This merge strategy implements the algorithm described in the paper "
            + utils::format_conference_reference(
                {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
                "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
                "https://ai.dmi.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
                "Proceedings of the 26th International Conference on Planning and "
                "Scheduling (ICAPS 2016)",
                "2358-2366",
                "AAAI Press",
                "2016") +
            "In a nutshell, it computes the maximal SCCs of the causal graph, "
            "obtaining a partitioning of the task's variables. Every such "
            "partition is then merged individually, using the specified fallback "
            "merge strategy, considering the SCCs in a configurable order. "
            "Afterwards, all resulting composite abstractions are merged to form "
            "the final abstraction, again using the specified fallback merge "
            "strategy and the configurable order of the SCCs.");

        add_option<OrderOfSCCs>(
            "order_of_sccs",
            "how the SCCs should be ordered",
            "topological");
        add_option<shared_ptr<MergeSelector>>(
            "merge_selector",
            "the fallback merge strategy to use");
        add_merge_strategy_options_to_feature(*this);
    }

    virtual shared_ptr<MergeStrategyFactorySCCs>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<MergeStrategyFactorySCCs>(
            opts.get<OrderOfSCCs>("order_of_sccs"),
            opts.get<shared_ptr<MergeSelector>> ("merge_selector"),
            get_merge_strategy_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<MergeStrategyFactorySCCsFeature> _plugin;

static plugins::TypedEnumPlugin<OrderOfSCCs> _enum_plugin({
        {"topological",
         "according to the topological ordering of the directed graph "
         "where each obtained SCC is a 'supervertex'"},
        {"reverse_topological",
         "according to the reverse topological ordering of the directed "
         "graph where each obtained SCC is a 'supervertex'"},
        {"decreasing",
         "biggest SCCs first, using 'topological' as tie-breaker"},
        {"increasing",
         "smallest SCCs first, using 'topological' as tie-breaker"}
    });
}
