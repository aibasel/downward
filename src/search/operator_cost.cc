#include "globals.h"
#include "operator_cost.h"
#include "operator.h"
#include "utilities.h"

#include <cstdlib>
using namespace std;

int get_adjusted_action_cost(const Operator &op, OperatorCost cost_type) {
    if (op.is_axiom())
        return 0;
    switch (cost_type) {
    case NORMAL:
        return op.get_cost();
    case ONE:
        return 1;
    case PLUSONE:
        if (g_min_action_cost == 1 && g_max_action_cost == 1)
            return 1;
        else
            return op.get_cost() + 1;
    default:
        ABORT("Unknown cost type");
    }
}
