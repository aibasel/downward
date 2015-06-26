#ifndef POTENTIALS_POTENTIAL_OPTIMIZER_H
#define POTENTIALS_POTENTIAL_OPTIMIZER_H

#include "../lp_solver.h"

#include <memory>
#include <vector>

class GlobalState;
class Heuristic;
class Options;
class StateRegistry;

namespace potentials {

class PotentialHeuristic;

enum OptimizationFunction {
    INITIAL_STATE,
    ALL_STATES,
    SAMPLES
};

class PotentialOptimizer {
    LPSolver lp_solver;
    const int num_samples;
    const double max_potential;
    const double max_sampling_time;
    const double max_filtering_time;
    const bool debug;
    int num_cols;
    std::vector<std::vector<int> > lp_var_ids;
    std::vector<std::vector<double> > fact_potentials;

    void construct_lp();
    void filter_dead_ends(std::vector<GlobalState> &samples);
    void extract_lp_solution();
    bool solve_lp();

public:
    explicit PotentialOptimizer(const Options &options);
    ~PotentialOptimizer() = default;

    void set_objective(const std::vector<double> &coefficients);

    bool optimize_for_state(const GlobalState &state);
    bool optimize_for_all_states();
    bool optimize_for_samples(const std::vector<GlobalState> &samples);

    bool has_optimal_solution() const;
    std::shared_ptr<Heuristic> get_heuristic() const;
};

void add_common_potential_options_to_parser(OptionParser &parser);

}
#endif
