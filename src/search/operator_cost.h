#ifndef OPERATOR_COST_H
#define OPERATOR_COST_H

#include <tuple>

class OperatorProxy;

namespace plugins {
class Feature;
class Options;
}

enum OperatorCost {NORMAL = 0, ONE = 1, PLUSONE = 2, MAX_OPERATOR_COST};

int get_adjusted_action_cost(const OperatorProxy &op, OperatorCost cost_type, bool is_unit_cost);
extern void add_cost_type_options_to_feature(plugins::Feature &feature);
extern std::tuple<OperatorCost> get_cost_type_arguments_from_options(
    const plugins::Options &opts);
#endif
