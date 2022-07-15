#include "pattern_collection_generator_combo.h"

#include "pattern_generator_greedy.h"
#include "utils.h"
#include "validation.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/timer.h"

#include <iostream>
#include <memory>
#include <set>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorCombo::PatternCollectionGeneratorCombo(
    const Options &opts)
    : PatternCollectionGenerator(opts), opts(opts) {
}

string PatternCollectionGeneratorCombo::name() const {
    return "combo pattern collection generator";
}

PatternCollectionInformation PatternCollectionGeneratorCombo::compute_patterns(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    shared_ptr<PatternCollection> patterns = make_shared<PatternCollection>();

    PatternGeneratorGreedy large_pattern_generator(opts);
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

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.add_option<int>(
        "max_states",
        "maximum abstraction size for combo strategy",
        "1000000",
        Bounds("1", "infinity"));
    add_generator_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorCombo>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("combo", _parse);
}
