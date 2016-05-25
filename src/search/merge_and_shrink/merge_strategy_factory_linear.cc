#include "merge_strategy_factory_linear.h"

#include "merge_linear.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/markup.h"
#include "../utils/memory.h"

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactoryLinear::MergeStrategyFactoryLinear(
    options::Options &options)
    : MergeStrategyFactory(),
      variable_order_type(static_cast<VariableOrderType>(
                              options.get_enum("variable_order"))) {
}

unique_ptr<MergeStrategy> MergeStrategyFactoryLinear::compute_merge_strategy(
    shared_ptr<AbstractTask> task,
    FactoredTransitionSystem &fts) {
    return utils::make_unique_ptr<MergeLinear>(
        fts,
        utils::make_unique_ptr<VariableOrderFinder>(task, variable_order_type));
}

void MergeStrategyFactoryLinear::dump_strategy_specific_options() const {
    dump_variable_order_type(variable_order_type);
}

string MergeStrategyFactoryLinear::name() const {
    return "linear";
}

static shared_ptr<MergeStrategyFactory>_parse(options::OptionParser &parser) {
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

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeStrategyFactoryLinear>(opts);
}

static options::PluginShared<MergeStrategyFactory> _plugin("merge_linear", _parse);
}
