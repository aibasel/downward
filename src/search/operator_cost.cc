#include "operator_cost.h"
#include "operator.h"

#include <cstdlib>
using namespace std;

int get_adjusted_action_cost(const Operator &op, OperatorCost cost_type) {
    switch (cost_type) {
    case NORMAL:
        return op.get_cost();
    case ONE:
        return 1;
    case PLUSONE:
        return op.get_cost() + 1;
    case MAX_OPERATOR_COST:
        cerr << "Unknown cost type" << endl;
        abort();
    }
}
