#include "pattern_generator_greedy.h"

#include "validation.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"
#include "../variable_order_finder.h"

#include <iostream>

using namespace std;


namespace PDBs {
PatternGeneratorGreedy::PatternGeneratorGreedy(const Options &opts)
    : PatternGeneratorGreedy(opts.get<int>("max_states")) {
}

PatternGeneratorGreedy::PatternGeneratorGreedy(int max_states)
    : max_states(max_states) {
}

Pattern PatternGeneratorGreedy::generate(shared_ptr<AbstractTask> task) {
    TaskProxy task_proxy(*task);
    Pattern pattern;
    VariableOrderFinder order(task, GOAL_CG_LEVEL);
    VariablesProxy variables = task_proxy.get_variables();

    int size = 1;
    while (true) {
        if (order.done())
            break;
        int next_var_id = order.next();
        VariableProxy next_var = variables[next_var_id];
        int next_var_size = next_var.get_domain_size();

        if (!is_product_within_limit(size, next_var_size, max_states))
            break;

        pattern.push_back(next_var_id);
        size *= next_var_size;
    }

    validate_and_normalize_pattern(task_proxy, pattern);
    cout << "Greedy pattern: " << pattern << endl;
    return pattern;
}

static shared_ptr<PatternGenerator> _parse(OptionParser &parser) {
    parser.add_option<int>(
        "max_states",
        "maximal number of abstract states in the pattern database.",
        "1000000",
        Bounds("1", "infinity"));

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternGeneratorGreedy>(opts);
}

static PluginShared<PatternGenerator> _plugin("greedy", _parse);
}
