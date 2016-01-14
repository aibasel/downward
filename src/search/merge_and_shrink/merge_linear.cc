#include "merge_linear.h"

#include "factored_transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <cassert>
#include <iostream>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
MergeLinear::MergeLinear(const Options &opts)
    : MergeStrategy(),
      variable_order_type(VariableOrderType(opts.get_enum("variable_order"))),
      need_first_index(true) {
}

void MergeLinear::initialize(const shared_ptr<AbstractTask> task) {
    MergeStrategy::initialize(task);
    variable_order_finder = utils::make_unique_ptr<VariableOrderFinder>(
        task, variable_order_type);
}

pair<int, int> MergeLinear::get_next(FactoredTransitionSystem &fts) {
    int num_transition_systems = fts.get_size();
    assert(initialized());
    assert(!done());
    assert(!variable_order_finder->done());

    int first;
    if (need_first_index) {
        need_first_index = false;
        first = variable_order_finder->next();
        cout << "First variable: " << first << endl;
    } else {
        // The most recent composite transition system is appended at the end of
        // all_transition_systems in merge_and_shrink.cc
        first = num_transition_systems - 1;
    }
    int second = variable_order_finder->next();
    cout << "Next variable: " << second << endl;
    assert(fts.is_active(first));
    assert(fts.is_active(second));
    --remaining_merges;
    if (done() && !variable_order_finder->done()) {
        cerr << "Variable order finder not done, but no merges remaining" << endl;
        utils::exit_with(ExitCode::CRITICAL_ERROR);
    }
    return make_pair(first, second);
}

void MergeLinear::dump_strategy_specific_options() const {
    variable_order_finder->dump();
}

string MergeLinear::name() const {
    return "linear";
}

static shared_ptr<MergeStrategy>_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Linear merge strategies",
        "This merge strategy implements several linear merge orders, which "
        "are described in the paper:" + utils::format_paper_reference(
            {"Malte Helmert", "Patrik Haslum", "Joerg Hoffmann"},
            "Flexible Abstraction Heuristics for Optimal Sequential Planning",
            "http://ai.cs.unibas.ch/papers/helmert-et-al-icaps2007.pdf",
            "Proceedings of the Seventeenth International Conference on"
            " Automated Planning and Scheduling (ICAPS 2007)",
            "176-183",
            "2007"));
    vector<string> merge_strategies;
    merge_strategies.push_back("CG_GOAL_LEVEL");
    merge_strategies.push_back("CG_GOAL_RANDOM");
    merge_strategies.push_back("GOAL_CG_LEVEL");
    merge_strategies.push_back("RANDOM");
    merge_strategies.push_back("LEVEL");
    merge_strategies.push_back("REVERSE_LEVEL");
    parser.add_enum_option("variable_order", merge_strategies,
                           "the order in which atomic transition systems are merged",
                           "CG_GOAL_LEVEL");

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeLinear>(opts);
}

static PluginShared<MergeStrategy> _plugin("merge_linear", _parse);
}
