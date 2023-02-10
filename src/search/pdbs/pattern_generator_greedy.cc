#include "pattern_generator_greedy.h"

#include "pattern_information.h"
#include "utils.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../task_utils/variable_order_finder.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/timer.h"

#include <iostream>

using namespace std;

namespace pdbs {
PatternGeneratorGreedy::PatternGeneratorGreedy(const plugins::Options &opts)
    : PatternGenerator(opts), max_states(opts.get<int>("max_states")) {
}

string PatternGeneratorGreedy::name() const {
    return "greedy pattern generator";
}

PatternInformation PatternGeneratorGreedy::compute_pattern(const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    Pattern pattern;
    variable_order_finder::VariableOrderFinder order(
        task_proxy, variable_order_finder::GOAL_CG_LEVEL);
    VariablesProxy variables = task_proxy.get_variables();

    int size = 1;
    while (true) {
        if (order.done())
            break;
        int next_var_id = order.next();
        VariableProxy next_var = variables[next_var_id];
        int next_var_size = next_var.get_domain_size();

        if (!utils::is_product_within_limit(size, next_var_size, max_states))
            break;

        pattern.push_back(next_var_id);
        size *= next_var_size;
    }

    return PatternInformation(task_proxy, move(pattern), log);
}

class PatternGeneratorGreedyFeature : public plugins::TypedFeature<PatternGenerator, PatternGeneratorGreedy> {
public:
    PatternGeneratorGreedyFeature() : TypedFeature("greedy") {
        add_option<int>(
            "max_states",
            "maximal number of abstract states in the pattern database.",
            "1000000",
            plugins::Bounds("1", "infinity"));
        add_generator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PatternGeneratorGreedyFeature> _plugin;
}
