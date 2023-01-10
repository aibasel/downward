#ifndef OPERATOR_COST_H
#define OPERATOR_COST_H

class OperatorProxy;

namespace plugins {
class OptionParser;
}

enum OperatorCost {NORMAL = 0, ONE = 1, PLUSONE = 2, MAX_OPERATOR_COST};

int get_adjusted_action_cost(const OperatorProxy &op, OperatorCost cost_type, bool is_unit_cost);
void add_cost_type_option_to_parser(plugins::OptionParser &parser);

#endif
