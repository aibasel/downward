#include "pattern_collection_generator_combo.h"

#include "pattern_generator_greedy.h"
#include "utils.h"
#include "validation.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/timer.h"

#include <iostream>
#include <memory>
#include <set>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorCombo::PatternCollectionGeneratorCombo(
    int max_states, utils::Verbosity verbosity)
    : PatternCollectionGenerator(verbosity),
      max_states(max_states),
      verbosity(verbosity) {
}

string PatternCollectionGeneratorCombo::name() const {
    return "combo pattern collection generator";
}

PatternCollectionInformation PatternCollectionGeneratorCombo::compute_patterns(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    shared_ptr<PatternCollection> patterns = make_shared<PatternCollection>();

    PatternGeneratorGreedy large_pattern_generator(
        max_states, verbosity);
    Pattern large_pattern = large_pattern_generator.generate(task).get_pattern();
    set<int> used_vars(large_pattern.begin(), large_pattern.end());
    patterns->push_back(move(large_pattern));

    for (FactProxy goal : task_proxy.get_goals()) {
        int goal_var_id = goal.get_variable().get_id();
        if (!used_vars.count(goal_var_id)) {
            patterns->emplace_back(1, goal_var_id);
        }
    }

    PatternCollectionInformation pci(task_proxy, patterns, log);
    return pci;
}

class PatternCollectionGeneratorComboFeature
    : public plugins::TypedFeature<PatternCollectionGenerator, PatternCollectionGeneratorCombo> {
public:
    PatternCollectionGeneratorComboFeature() : TypedFeature("combo") {
        add_option<int>(
            "max_states",
            "maximum abstraction size for combo strategy",
            "1000000",
            plugins::Bounds("1", "infinity"));
        add_generator_options_to_feature(*this);
    }

    virtual shared_ptr<PatternCollectionGeneratorCombo> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<PatternCollectionGeneratorCombo>(
            opts.get<int>("max_states"),
            get_generator_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<PatternCollectionGeneratorComboFeature> _plugin;
}
