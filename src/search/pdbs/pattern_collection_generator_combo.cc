#include "pattern_collection_generator_combo.h"

#include "pattern_generator_greedy.h"
#include "utils.h"
#include "validation.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/timer.h"

#include <iostream>
#include <memory>
#include <set>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorCombo::PatternCollectionGeneratorCombo(const Options &opts)
    : max_states(opts.get<int>("max_states")),
      rng(utils::parse_rng_from_options(opts)) {
}

PatternCollectionInformation PatternCollectionGeneratorCombo::generate(
    const shared_ptr<AbstractTask> &task) {
    utils::Timer timer;
    utils::g_log << "Generating patterns using the combo generator..." << endl;
    TaskProxy task_proxy(*task);
    shared_ptr<PatternCollection> patterns = make_shared<PatternCollection>();

    PatternGeneratorGreedy large_pattern_generator(max_states, rng);
    const Pattern &large_pattern = large_pattern_generator.generate(task).get_pattern();
    patterns->push_back(large_pattern);

    set<int> used_vars(large_pattern.begin(), large_pattern.end());
    for (FactProxy goal : task_proxy.get_goals()) {
        int goal_var_id = goal.get_variable().get_id();
        if (!used_vars.count(goal_var_id))
            patterns->emplace_back(1, goal_var_id);
    }

    PatternCollectionInformation pci(task_proxy, patterns);
    dump_pattern_collection_generation_statistics(
        "Combo generator", timer(), pci);
    return pci;
}

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.add_option<int>(
        "max_states",
        "maximum abstraction size for combo strategy",
        "1000000",
        Bounds("1", "infinity"));
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorCombo>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("combo", _parse);
}
