#ifndef OPERATOR_COST_H
#define OPERATOR_COST_H

class OperatorProxy;

namespace plugins {
class Feature;
}

enum OperatorCost {NORMAL = 0, ONE = 1, PLUSONE = 2, MAX_OPERATOR_COST};

int get_adjusted_action_cost(const OperatorProxy &op, OperatorCost cost_type, bool is_unit_cost);
void add_cost_type_option_to_feature(plugins::Feature &feature);

#endif
