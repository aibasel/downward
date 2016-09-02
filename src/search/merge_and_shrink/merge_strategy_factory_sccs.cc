#include "merge_strategy_factory_sccs.h"

#include "merge_strategy_sccs.h"
#include "merge_selector.h"
#include "merge_tree_factory.h"
#include "transition_system.h"

#include "../task_proxy.h"

#include "../causal_graph.h"
#include "../scc.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/logging.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
bool compare_sccs_increasing(const vector<int> &lhs, const vector<int> &rhs) {
    return lhs.size() < rhs.size();
}

bool compare_sccs_decreasing(const vector<int> &lhs, const vector<int> &rhs) {
    return lhs.size() > rhs.size();
}

MergeStrategyFactorySCCs::MergeStrategyFactorySCCs(const options::Options &options)
    : MergeStrategyFactory(),
      order_of_sccs(static_cast<OrderOfSCCs>(options.get_enum("order_of_sccs"))),
      merge_tree_factory(nullptr),
      merge_selector(nullptr) {
    if (options.contains("merge_tree")) {
        merge_tree_factory = options.get<shared_ptr<MergeTreeFactory>>("merge_tree");
    }
    if (options.contains("merge_selector")) {
        merge_selector = options.get<shared_ptr<MergeSelector>>("merge_selector");
    }
}

unique_ptr<MergeStrategy> MergeStrategyFactorySCCs::compute_merge_strategy(
    const shared_ptr<AbstractTask> &task,
    FactoredTransitionSystem &fts) {
    TaskProxy task_proxy(*task);
    VariablesProxy vars = task_proxy.get_variables();
    int num_vars = vars.size();

    // Compute SCCs of the causal graph
    vector<vector<int>> cg;
    cg.reserve(num_vars);
    for (VariableProxy var : vars) {
        const vector<int> &successors =
            task_proxy.get_causal_graph().get_successors(var.get_id());
        cg.push_back(successors);
    }
    SCC scc(cg);
    vector<vector<int>> sccs(scc.get_result());

    // Put the SCCs in the desired order
    switch (order_of_sccs) {
    case OrderOfSCCs::TOPOLOGICAL:
        // SCCs are computed in topological order
        break;
    case OrderOfSCCs::REVERSE_TOPOLOGICAL:
        // SCCs are computed in topological order
        reverse(sccs.begin(), sccs.end());
        break;
    case OrderOfSCCs::DECREASING:
        sort(sccs.begin(), sccs.end(), compare_sccs_decreasing);
        break;
    case OrderOfSCCs::INCREASING:
        sort(sccs.begin(), sccs.end(), compare_sccs_increasing);
        break;
    }

    /*
      Compute the indices at which the merged SCCs can be found when all
      SCCs have been merged.
    */
    int index = num_vars - 1;
    cout << "found CG SCCs:" << endl;
    vector<vector<int>> non_singleton_cg_sccs;
    vector<int> indices_of_merged_sccs;
    indices_of_merged_sccs.reserve(sccs.size());
    for (const vector<int> &scc : sccs) {
        cout << scc << endl;
        int scc_size = scc.size();
        if (scc_size == 1) {
            indices_of_merged_sccs.push_back(scc.front());
        } else {
            index += scc_size - 1;
            indices_of_merged_sccs.push_back(index);
            // only store non-singleton sccs for internal merging
            non_singleton_cg_sccs.push_back(vector<int>(scc.begin(), scc.end()));
        }
    }
    if (sccs.size() == 1) {
        cout << "Only one single SCC" << endl;
    }
    if (static_cast<int>(sccs.size()) == num_vars) {
        cout << "Only singleton SCCs" << endl;
        assert(non_singleton_cg_sccs.empty());
    }

    if (merge_selector) {
        merge_selector->initialize(task);
    }

    return utils::make_unique_ptr<MergeSCCs>(
        fts,
        task,
        merge_tree_factory,
        merge_selector,
        move(non_singleton_cg_sccs),
        move(indices_of_merged_sccs));
}

void MergeStrategyFactorySCCs::dump_strategy_specific_options() const {
    cout << "Merge order of sccs: ";
    switch (order_of_sccs) {
    case OrderOfSCCs::TOPOLOGICAL:
        cout << "topological";
        break;
    case OrderOfSCCs::REVERSE_TOPOLOGICAL:
        cout << "reverse topological";
        break;
    case OrderOfSCCs::DECREASING:
        cout << "decreasing";
        break;
    case OrderOfSCCs::INCREASING:
        cout << "increasing";
        break;
    }
    cout << endl;

    cout << "Merge strategy for merging within sccs: ";
    if (merge_tree_factory) {
        merge_tree_factory->dump_options();
    }
    if (merge_selector) {
        merge_selector->dump_options();
    }
}

string MergeStrategyFactorySCCs::name() const {
    return "sccs";
}

static shared_ptr<MergeStrategyFactory>_parse(options::OptionParser &parser) {
    vector<string> order_of_sccs;
    order_of_sccs.push_back("topological");
    order_of_sccs.push_back("reverse_topological");
    order_of_sccs.push_back("decreasing");
    order_of_sccs.push_back("increasing");
    parser.add_enum_option(
        "order_of_sccs",
        order_of_sccs,
        "choose an ordering of the sccs: topological/reverse_topological or "
        "decreasing/increasing in the size of the SCCs.");
    parser.add_option<shared_ptr<MergeTreeFactory>>(
        "merge_tree",
        "the fallback merge strategy to use if a precomputed strategy should"
        "be used.",
        options::OptionParser::NONE);
    parser.add_option<shared_ptr<MergeSelector>>(
        "merge_selector",
        "the fallback merge strategy to use if a stateless strategy should"
        "be used.",
        options::OptionParser::NONE);

    options::Options options = parser.parse();
    bool merge_tree = options.contains("merge_tree");
    bool merge_selector = options.contains("merge_selector");
    if ((merge_tree && merge_selector) || (!merge_tree && !merge_selector)) {
        cerr << "You have to specify exactly one of the options merge_tree "
                "and merg_selector!" << endl;
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
    if (parser.dry_run())
        return 0;
    else
        return make_shared<MergeStrategyFactorySCCs>(options);
}

static options::PluginShared<MergeStrategyFactory> _plugin("merge_sccs", _parse);
}
