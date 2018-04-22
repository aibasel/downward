#ifndef COST_SATURATION_SCORING_FUNCTIONS_H
#define COST_SATURATION_SCORING_FUNCTIONS_H

#include <vector>

namespace options {
class OptionParser;
}

namespace cost_saturation {
enum class ScoringFunction {
    RANDOM,
    MAX_HEURISTIC,
    MIN_COSTS,
    MAX_HEURISTIC_PER_COSTS,
    MIN_STOLEN_COSTS,
    MAX_HEURISTIC_PER_STOLEN_COSTS
};

extern double compute_score(
    int h, int used_costs, ScoringFunction scoring_function, bool use_exp);

extern int compute_stolen_costs(int wanted_by_abs, int surplus_cost);

extern std::vector<int> compute_all_surplus_costs(
    const std::vector<int> &costs,
    const std::vector<std::vector<int>> &saturated_costs_by_abstraction);

extern int compute_costs_stolen_by_heuristic(
    const std::vector<int> &saturated_costs,
    const std::vector<int> &surplus_costs);

extern void add_scoring_function_to_parser(options::OptionParser &parser);
}

#endif
