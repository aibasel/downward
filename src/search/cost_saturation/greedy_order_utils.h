#ifndef COST_SATURATION_GREEDY_ORDER_UTILS_H
#define COST_SATURATION_GREEDY_ORDER_UTILS_H

#include <vector>

namespace options {
class OptionParser;
}

namespace cost_saturation {
/*
  Scoring functions for OrderGeneratorGreedy.

  Each scoring function estimates how good it is to let the cost partitioning
  algorithm consider a heuristic early. The greedy ordering algorithm orders
  heuristics by their scores in decreasing order.
*/
enum class ScoringFunction {
    MAX_HEURISTIC,
    MIN_STOLEN_COSTS,
    MAX_HEURISTIC_PER_STOLEN_COSTS
};

/*
  Compute real-valued score for a heuristic given a heuristic estimate, the sum
  of costs needed to justify the estimate and a scoring function.
*/
extern double compute_score(
    int h, int used_costs, ScoringFunction scoring_function);

/*
  surplus(o) = sum of costs that each heuristic wants from o - cost(o).
*/
extern std::vector<int> compute_all_surplus_costs(
    const std::vector<int> &costs,
    const std::vector<std::vector<int>> &saturated_costs_by_abstraction);

/*
  Compute total amount of costs that a heuristic (with the given saturated cost
  function) "steals" from the other heuristics.
*/
extern int compute_costs_stolen_by_heuristic(
    const std::vector<int> &saturated_costs,
    const std::vector<int> &surplus_costs);

extern void add_scoring_function_to_parser(options::OptionParser &parser);
}

#endif
