#include "operator_cost.h"

#include "plugins/plugin.h"
#include "task_proxy.h"

#include "utils/system.h"

#include <cstdlib>
#include <vector>
using namespace std;

static int get_adjusted_action_cost(int cost, OperatorCost cost_type, bool is_unit_cost) {
    switch (cost_type) {
    case NORMAL:
        return cost;
    case ONE:
        return 1;
    case PLUSONE:
        if (is_unit_cost)
            return 1;
        else
            return cost + 1;
    default:
        ABORT("Unknown cost type");
    }
}

int get_adjusted_action_cost(const OperatorProxy &op, OperatorCost cost_type, bool is_unit_cost) {
    if (op.is_axiom())
        return 0;
    else
        return get_adjusted_action_cost(op.get_cost(), cost_type, is_unit_cost);
}

void add_cost_type_option_to_feature(plugins::Feature &feature) {
    feature.add_option<OperatorCost>(
        "cost_type",
        "Operator cost adjustment type. "
        "No matter what this setting is, axioms will always be considered "
        "as actions of cost 0 by the heuristics that treat axioms as actions.",
        "normal");
}

static plugins::TypedEnumPlugin<OperatorCost> _enum_plugin({
    {"normal", "all actions are accounted for with their real cost"},
    {"one", "all actions are accounted for as unit cost"},
    {"plusone", "all actions are accounted for as their real cost + 1 "
     "(except if all actions have original cost 1, "
     "in which case cost 1 is used). "
     "This is the behaviour known for the heuristics of the LAMA planner. "
     "This is intended to be used by the heuristics, not search engines, "
     "but is supported for both."}
});
