#include "abstract_task.h"

#include "cost_adapted_task.h"
#include "globals.h"
#include "operator_cost.h"
#include "option_parser_util.h"
#include "utilities.h"

#include <iostream>

using namespace std;

const shared_ptr<AbstractTask> get_task_from_options(const Options &opts) {
    /*
      TODO: This code is only intended for the transitional period while we
      still support the "old style" of adjusting costs (via the cost_type
      parameter) in parallel with the "new style" (via task transformations).
      Once all heuristics are adapted to support task transformations and we can
      remove the "cost_type" attribute, the options should always contain an
      AbstractTask object. Then we can directly call
      opts.get<shared_ptr<AbstractTask>>("transform") where needed and this
      function can be removed.
    */
    OperatorCost cost_type = OperatorCost(opts.get_enum("cost_type"));
    if (opts.contains("transform") && cost_type != NORMAL) {
        cerr << "You may specify either the cost_type option (deprecated) or "
             << "use transform=adapt_costs(...) (recommended), but not both."
             << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    shared_ptr<AbstractTask> task;
    if (opts.contains("transform")) {
        task = opts.get<shared_ptr<AbstractTask> >("transform");
    } else if (cost_type != NORMAL) {
        Options options;
        options.set<shared_ptr<AbstractTask> >("transform", g_root_task());
        options.set<int>("cost_type", cost_type);
        task = make_shared<CostAdaptedTask>(options);
    } else {
        task = g_root_task();
    }
    return task;
}
