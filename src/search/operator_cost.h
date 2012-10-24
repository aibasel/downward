#ifndef OPERATOR_COST_H
#define OPERATOR_COST_H

class Operator;
class OptionParser;

enum OperatorCost {NORMAL = 0, ONE = 1, PLUSONE = 2, MAX_OPERATOR_COST};

int get_adjusted_action_cost(const Operator &op, OperatorCost cost_type);
void add_cost_type_option_to_parser(OptionParser &parser);

#endif
