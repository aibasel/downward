#include "merge_tree_factory_linear.h"

#include "factored_transition_system.h"
#include "merge_tree.h"
#include "transition_system.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/system.h"

#include <algorithm>

using namespace std;

namespace merge_and_shrink {
MergeTreeFactoryLinear::MergeTreeFactoryLinear(const plugins::Options &options)
    : MergeTreeFactory(options),
      variable_order_type(
          options.get<variable_order_finder::VariableOrderType>("variable_order")),
      rng(utils::parse_rng_from_options(options)) {
}

unique_ptr<MergeTree> MergeTreeFactoryLinear::compute_merge_tree(
    const TaskProxy &task_proxy) {
    variable_order_finder::VariableOrderFinder vof(task_proxy, variable_order_type, rng);
    MergeTreeNode *root = new MergeTreeNode(vof.next());
    while (!vof.done()) {
        MergeTreeNode *right_child = new MergeTreeNode(vof.next());
        root = new MergeTreeNode(root, right_child);
    }
    return utils::make_unique_ptr<MergeTree>(
        root, rng, update_option);
}

unique_ptr<MergeTree> MergeTreeFactoryLinear::compute_merge_tree(
    const TaskProxy &task_proxy,
    const FactoredTransitionSystem &fts,
    const vector<int> &indices_subset) {
    /*
      Compute a mapping from state variables to transition system indices
      that contain those variables. Also set all indices not contained in
      indices_subset to "used".
    */
    int num_vars = task_proxy.get_variables().size();
    int num_ts = fts.get_size();
    vector<int> var_to_ts_index(num_vars, -1);
    vector<bool> used_ts_indices(num_ts, true);
    for (int ts_index : fts) {
        bool use_ts_index =
            find(indices_subset.begin(), indices_subset.end(),
                 ts_index) != indices_subset.end();
        if (use_ts_index) {
            used_ts_indices[ts_index] = false;
        }
        const vector<int> &vars =
            fts.get_transition_system(ts_index).get_incorporated_variables();
        for (int var : vars) {
            var_to_ts_index[var] = ts_index;
        }
    }

    /*
     Compute the merge tree, using transition systems corresponding to
     variables in order given by the variable order finder, implicitly
     skipping all indices not in indices_subset, because these have been set
     to "used" above.
    */
    variable_order_finder::VariableOrderFinder vof(task_proxy, variable_order_type, rng);

    int next_var = vof.next();
    int ts_index = var_to_ts_index[next_var];
    assert(ts_index != -1);
    // find the first valid ts index
    while (used_ts_indices[ts_index]) {
        assert(!vof.done());
        next_var = vof.next();
        ts_index = var_to_ts_index[next_var];
        assert(ts_index != -1);
    }
    used_ts_indices[ts_index] = true;
    MergeTreeNode *root = new MergeTreeNode(ts_index);

    while (!vof.done()) {
        next_var = vof.next();
        ts_index = var_to_ts_index[next_var];
        assert(ts_index != -1);
        if (!used_ts_indices[ts_index]) {
            used_ts_indices[ts_index] = true;
            MergeTreeNode *right_child = new MergeTreeNode(ts_index);
            root = new MergeTreeNode(root, right_child);
        }
    }
    return utils::make_unique_ptr<MergeTree>(
        root, rng, update_option);
}

string MergeTreeFactoryLinear::name() const {
    return "linear";
}

void MergeTreeFactoryLinear::dump_tree_specific_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        dump_variable_order_type(variable_order_type, log);
    }
}

void MergeTreeFactoryLinear::add_options_to_feature(plugins::Feature &feature) {
    MergeTreeFactory::add_options_to_feature(feature);
    feature.add_option<variable_order_finder::VariableOrderType>(
        "variable_order",
        "the order in which atomic transition systems are merged",
        "cg_goal_level");
}

class MergeTreeFactoryLinearFeature : public plugins::TypedFeature<MergeTreeFactory, MergeTreeFactoryLinear> {
public:
    MergeTreeFactoryLinearFeature() : TypedFeature("linear") {
        document_title("Linear merge trees");
        document_synopsis(
            "These merge trees implement several linear merge orders, which "
            "are described in the paper:" + utils::format_conference_reference(
                {"Malte Helmert", "Patrik Haslum", "Joerg Hoffmann"},
                "Flexible Abstraction Heuristics for Optimal Sequential Planning",
                "https://ai.dmi.unibas.ch/papers/helmert-et-al-icaps2007.pdf",
                "Proceedings of the Seventeenth International Conference on"
                " Automated Planning and Scheduling (ICAPS 2007)",
                "176-183",
                "AAAI Press",
                "2007"));

        MergeTreeFactoryLinear::add_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<MergeTreeFactoryLinearFeature> _plugin;

static plugins::TypedEnumPlugin<variable_order_finder::VariableOrderType> _enum_plugin({
        {"cg_goal_level",
         "variables are prioritized first if they have an arc to a previously "
         "added variable, second if their goal value is defined "
         "and third according to their level in the causal graph"},
        {"cg_goal_random",
         "variables are prioritized first if they have an arc to a previously "
         "added variable, second if their goal value is defined "
         "and third randomly"},
        {"goal_cg_level",
         "variables are prioritized first if their goal value is defined, "
         "second if they have an arc to a previously added variable, "
         "and third according to their level in the causal graph"},
        {"random",
         "variables are ordered randomly"},
        {"level",
         "variables are ordered according to their level in the causal graph"},
        {"reverse_level",
         "variables are ordered reverse to their level in the causal graph"}
    });
}
