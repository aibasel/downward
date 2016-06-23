#include "merge_tree_factory_linear.h"

#include "merge_tree.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/markup.h"

using namespace std;

namespace merge_and_shrink {
MergeTreeFactoryLinear::MergeTreeFactoryLinear(const options::Options &options)
    : MergeTreeFactory(),
      variable_order_type(static_cast<VariableOrderType>(
          options.get_enum("variable_order"))){
}

MergeTree *MergeTreeFactoryLinear::compute_merge_tree(
    shared_ptr<AbstractTask> task,
    FactoredTransitionSystem &) {
    VariableOrderFinder vof(task, variable_order_type);
    MergeTreeNode *root = new MergeTreeNode(vof.next());
    while (!vof.done()) {
        MergeTreeNode *right_child = new MergeTreeNode(vof.next());
        root = new MergeTreeNode(root, right_child);
    }
    return new MergeTree(root);
}

void MergeTreeFactoryLinear::dump_options() const {
    dump_variable_order_type(variable_order_type);
}

static shared_ptr<MergeTreeFactory>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Linear merge trees",
        "These merge trees implement several linear merge orders, which "
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
        return make_shared<MergeTreeFactoryLinear>(opts);
}

static options::PluginShared<MergeTreeFactory> _plugin("linear", _parse);
}
