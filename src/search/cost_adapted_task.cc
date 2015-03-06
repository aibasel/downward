#include "cost_adapted_task.h"

#include "option_parser.h"
#include "plugin.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;


CostAdaptedTask::CostAdaptedTask(const Options &opts)
    : DelegatingTask(shared_ptr<AbstractTask>(opts.get<AbstractTask *>("transform"))),
      cost_type(OperatorCost(opts.get<int>("cost_type"))) {
}

CostAdaptedTask::~CostAdaptedTask() {
}

int CostAdaptedTask::get_operator_cost(int index, bool is_axiom) const {
    return get_adjusted_action_cost(parent->get_operator_cost(index, is_axiom), cost_type);
}


static AbstractTask *_parse(OptionParser &parser) {
    parser.add_option<AbstractTask *>(
        "transform",
        "Parent task transformation",
        "no_transform");
    add_cost_type_option_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CostAdaptedTask(opts);
}

static Plugin<AbstractTask> _plugin("adapt_costs", _parse);
